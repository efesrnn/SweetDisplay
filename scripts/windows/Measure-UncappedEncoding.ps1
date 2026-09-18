# Offline summary only. Never creates a live session or upgrades resource acceptance.
param([Parameter(Mandatory=$true)][string]$EvidenceDirectory)
$ErrorActionPreference='Stop'
$dir=(Resolve-Path -LiteralPath $EvidenceDirectory).Path
function Json($p){[IO.File]::ReadAllText($p)|ConvertFrom-Json}
$h=Json (Join-Path $dir 'host/session-1-result.json')
$e=Json (Join-Path $dir 'host/encode-result.json')
$v=Json (Join-Path $dir 'encoding-verification.json')
$execution=Json (Join-Path $dir 'execution.json')
$calls=[IO.File]::ReadAllText((Join-Path $dir 'host/encoder-calls.txt'))
$reportPath=Join-Path $dir 'uncapped-acceptance.json'
$measurePath=Join-Path $dir 'sustained-measurements.json'
if((Test-Path $reportPath) -or (Test-Path $measurePath)){throw 'Preserve existing offline results'}
$allHost=$v.Outcome -eq 'PASS_ENCODE_DECODE' -and $execution.EncodeMode.UncappedSubmission -and $calls.Contains('submission_rate_limiter=disabled') -and $h.received -eq $e.input -and $e.input -eq $e.accepted -and $e.accepted -eq $e.outputs -and $e.outputs -eq $v.Sessions[0].Decode.decoded -and !$e.rate_drops -and !$e.backpressure_drops
$acceptance=@{Outcome=$(if($allHost){'PASS_ALL_UNIQUE_HOST_FRAMES'}else{'FAIL_ALL_UNIQUE_HOST_FRAMES'});Seconds=$h.seconds;Source=$h.source_frames;Host=$h.received;Submitted=$e.accepted;Encoded=$e.outputs;Decoded=$v.Sessions[0].Decode.decoded;SourceFps=$h.source_fps;HostFps=$h.fps;SubmittedFps=$e.accepted/$h.seconds;EncodedFps=$e.outputs/$h.seconds;SourceDrops=@{Producer=$h.dropped_before_host;Stale=$h.dropped_host_queue;Contention=$h.contention};RateDrops=$e.rate_drops;BackpressureDrops=$e.backpressure_drops;Scope='All unique Host-received frames only; upstream drops remain explicit. Resource/latency review is separate.'}
$acceptance|ConvertTo-Json -Depth 5|Set-Content $reportPath -Encoding utf8
function Percentile($sorted,[double]$q){if(!$sorted.Count){return $null};$sorted[[Math]::Max(0,[int][Math]::Ceiling($q*$sorted.Count)-1)]}
$outputs=@(Import-Csv (Join-Path $dir 'host/encode-output.csv'))
$inputs=@(Import-Csv (Join-Path $dir 'host/encode-input.csv'))
$periods=@();$periodSeconds=300
for($start=0;$start -lt $h.seconds;$start+=$periodSeconds){
 $end=[Math]::Min([double]($start+$periodSeconds),[double]$h.seconds)
 $o=@($outputs|Where-Object{[double]$_.pts/1e7 -ge $start -and [double]$_.pts/1e7 -lt $end})
 $lat=@($o|ForEach-Object{[double]$_.latency_ms}|Sort-Object)
 $i=@($inputs|Where-Object{([double]$_.source_qpc-[double]$inputs[0].source_qpc)/$h.frequency -ge $start -and ([double]$_.source_qpc-[double]$inputs[0].source_qpc)/$h.frequency -lt $end})
 $conv=@($i|Where-Object decision -eq 'accepted'|ForEach-Object{[double]$_.conversion_ms}|Sort-Object)
 $periods+=@{StartSeconds=$start;EndSeconds=$end;HostRecords=$i.Count;Outputs=$o.Count;OutputFps=$o.Count/($end-$start);LatencyMeanMs=($lat|Measure-Object -Average).Average;LatencyP50Ms=(Percentile $lat .5);LatencyP95Ms=(Percentile $lat .95);LatencyP99Ms=(Percentile $lat .99);LatencyMaxMs=($lat|Measure-Object -Maximum).Maximum;ConversionMeanMs=($conv|Measure-Object -Average).Average;ConversionP95Ms=(Percentile $conv .95);ConversionMaxMs=($conv|Measure-Object -Maximum).Maximum;EncoderQueuePeak=($i|Measure-Object pending -Maximum).Maximum;Backpressure=@($i|Where-Object decision -eq 'backpressure').Count;RateDrops=@($i|Where-Object decision -eq 'rate').Count}
}
$telemetry=@(Import-Csv (Join-Path $dir 'host/session-1-telemetry.csv'))
$dropWindows=@();$previous=$null
foreach($t in $telemetry){$elapsed=([double]$t.qpc-[double]$h.first_qpc)/$h.frequency;$s=[long]$t.host_stale_drops;$p=[long]$t.producer_drops;$c=[long]$t.contention
 if($previous){$ds=$s-[long]$previous.host_stale_drops;$dp=$p-[long]$previous.producer_drops;$dc=$c-[long]$previous.contention}else{$ds=$s;$dp=$p;$dc=$c}
 if($ds -or $dp -or $dc){$dropWindows+=@{AtSeconds=$elapsed;Stale=$ds;Producer=$dp;Contention=$dc}}
 $previous=$t
}
$tail=@{Stale=$h.dropped_host_queue-[long]$previous.host_stale_drops;Producer=$h.dropped_before_host-[long]$previous.producer_drops;Contention=$h.contention-[long]$previous.contention}
$idr=@();$lastIdrIndex=-1;$lastIdrPts=0L
for($i=0;$i -lt $outputs.Count;$i++){if($outputs[$i].idr -eq '1'){if($lastIdrIndex -ge 0){$idr+=@{Frames=$i-$lastIdrIndex;Seconds=([long]$outputs[$i].pts-$lastIdrPts)/1e7}};$lastIdrIndex=$i;$lastIdrPts=[long]$outputs[$i].pts}}
$gpu=@(Import-Csv (Join-Path $dir 'gpu-engines.csv')|Group-Object {($_.Name -split 'engtype_')[-1]}|ForEach-Object{[pscustomobject]@{Engine=$_.Name;Samples=$_.Count;Mean=($_.Group|Measure-Object UtilizationPercentage -Average).Average;Peak=($_.Group|Measure-Object UtilizationPercentage -Maximum).Maximum}})
$summary=@{Outcome='MEASURED_REVIEW_REQUIRED';Periods=$periods;DropWindows=$dropWindows;UnsampledTailDrops=$tail;IdrGaps=$idr;Gpu=$gpu;Acceptance=$acceptance;Scope='Coarse CPU/GPU telemetry and observed wall latencies are not isolated GPU execution or maximum encoder capacity measurements'}
$summary|ConvertTo-Json -Depth 8|Set-Content $measurePath -Encoding utf8
$acceptance|ConvertTo-Json -Depth 5
if(!$allHost){throw 'All-Host-frame acceptance failed; evidence retained; do not run recovery'}
