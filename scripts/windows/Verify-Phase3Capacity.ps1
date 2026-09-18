param([Parameter(Mandatory=$true)][string]$EvidenceDirectory,[ValidateRange(0,7200)][int]$MinimumStreamSeconds=0)
$ErrorActionPreference='Stop'
[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
function Require([bool]$Condition,[string]$Message){if(!$Condition){throw $Message}}
function Number([string]$Value){[double]::Parse($Value.Replace(',','.'),[Globalization.CultureInfo]::InvariantCulture)}
function Distribution($Values){
 [double[]]$a=@($Values);Require ($a.Count -gt 0) 'Empty distribution';[Array]::Sort($a)
 @{Count=$a.Count;Min=$a[0];Mean=($a|Measure-Object -Average).Average;P50=$a[[int][math]::Ceiling($a.Count*.50)-1];P95=$a[[int][math]::Ceiling($a.Count*.95)-1];P99=$a[[int][math]::Ceiling($a.Count*.99)-1];Max=$a[-1]}
}
$execution=Get-Content (Join-Path $EvidenceDirectory 'execution.json') -Raw | ConvertFrom-Json
Require ($execution.Outcome -eq 'PASS') 'Execution did not pass'
Require (!$execution.PatternForcedStop) 'Pattern did not shut down cleanly'
$patternCommand=@($execution.Commands|Where-Object Command -eq 'Start D3D11 pattern on SWT0001')[0].Result
Require ($patternCommand.Arguments -match '\s([0-9A-F]{8})\s+\d+(?:\s+\d+\s+[01])?$') 'Missing test nonce'
$expectedNonce=[Convert]::ToUInt32($Matches[1],16)
$pattern=@{};$patternFrames=@(Import-Csv (Join-Path $EvidenceDirectory 'pattern.csv'))
foreach($f in $patternFrames){$pattern[[uint32]$f.paint_tick]=[uint64]$f.qpc}
$patternReport=Get-Content (Join-Path $EvidenceDirectory 'pattern-result.json') -Raw | ConvertFrom-Json
Require ($patternFrames.Count -eq $patternReport.presented) 'Pattern log count'
$reports=[Collections.Generic.List[object]]::new();$prior=$null
foreach($name in @('main','reconnect')){
 $prefix=Join-Path $EvidenceDirectory ($name+'/session-1')
 $sampled=@($execution.Commands|Where-Object {$_.Command -eq 'Start bounded Host measurement' -and $_.Result.Arguments -like ('*'+$name+'"*')})[0].Result.Arguments -match '--sample'
 $r=Get-Content ($prefix+'-result.json') -Raw | ConvertFrom-Json
 $frames=@(Import-Csv ($prefix+'-frames.csv'))
 Require ($frames.Count -eq $r.received -and $r.received -gt 1) "$name frame count"
 Require ($r.frequency -eq $patternReport.frequency) "$name QPC frequency"
 if($name -eq 'main' -and $MinimumStreamSeconds){Require ($r.seconds -ge $MinimumStreamSeconds -and ($r.last_qpc-$r.first_qpc)/$r.frequency -ge $MinimumStreamSeconds) 'Insufficient actual soak frame span'}
 Require ($r.high_water -le 3 -and $r.held_end -eq 0 -and $r.clean_disconnect) "$name bounded lifetime"
 Require ($r.acknowledged -eq $r.received -and $r.invalid -eq 0) "$name acknowledgment"
 $accounted=$r.received+$r.dropped_before_host+$r.dropped_host_queue+$r.busy+$r.invalid+$r.contention+$r.queue_depth_end+$r.held_end
 Require ($accounted -eq $r.source_frames) "$name source/drop/outstanding accounting"
 $timings=@{};foreach($key in @('SourceInterval','ReceiveInterval','ResourceInterval','SampleCompleteInterval','MutexWait','SampleMapAndCopy','SourceToSampleComplete','RenderToSampleComplete')){$timings[$key]=[Collections.Generic.List[double]]::new()}
 [uint64]$lastId=0;[uint64]$lastQpc=0;[uint64]$lastReceive=0;[uint64]$lastAcquire=0;[uint64]$lastSample=0;[uint64]$lastPresent=0;[uint64]$gap=0;[uint32]$lastCounter=0
 $distinct=[Collections.Generic.HashSet[uint32]]::new();$presentations=[Collections.Generic.HashSet[uint64]]::new()
 foreach($f in $frames){
  [uint64]$id=$f.frame_id;[uint64]$qpc=$f.source_qpc;[uint64]$receive=$f.receive_qpc;[uint64]$acquire=$f.resource_acquired_qpc;[uint64]$sample=$f.sample_complete_qpc;[uint64]$present=$f.presentation_id;[uint32]$counter=$f.counter
  Require ($id -gt $lastId -and $qpc -gt $lastQpc -and $receive -gt $lastReceive -and $receive -ge $qpc -and $acquire -ge $receive -and $acquire -gt $lastAcquire -and (!$sampled -or ($sample -ge $acquire -and $sample -gt $lastSample -and $receive -ge $lastSample))) "$name clocks/ID ordering"
  Require ([uint64]$f.id_gap -eq $(if($lastId){$id-$lastId-1}else{0})) "$name row gap count"
  Require ($f.width -eq '2400' -and $f.height -eq '1080' -and $f.format -eq '87' -and $f.flags -eq '0') "$name per-frame geometry"
  if($sampled){Require ([uint32]$f.nonce -eq $expectedNonce -and $pattern.ContainsKey($counter) -and $pattern[$counter] -le $qpc -and $counter -ge $lastCounter) "$name real SWT0001 pattern correspondence"}
  Require ($present -ge $lastPresent) "$name presentation ordering"
  if($sampled){$null=$distinct.Add($counter)};$null=$presentations.Add($present)
  if($lastId){
   $gap+=$id-$lastId-1
   $timings.SourceInterval.Add(($qpc-$lastQpc)*1000.0/$r.frequency)
   $timings.ReceiveInterval.Add(($receive-$lastReceive)*1000.0/$r.frequency)
   $timings.ResourceInterval.Add(($acquire-$lastAcquire)*1000.0/$r.frequency)
   if($sampled){$timings.SampleCompleteInterval.Add(($sample-$lastSample)*1000.0/$r.frequency)}
  }
  $timings.MutexWait.Add((Number $f.mutex_wait_ms))
  if($sampled){
   $timings.SampleMapAndCopy.Add(($sample-$acquire)*1000.0/$r.frequency)
   $timings.SourceToSampleComplete.Add(($sample-$qpc)*1000.0/$r.frequency)
   $timings.RenderToSampleComplete.Add(($sample-$pattern[$counter])*1000.0/$r.frequency)
  }
  $lastId=$id;$lastQpc=$qpc;$lastReceive=$receive;$lastAcquire=$acquire;$lastSample=$sample;$lastPresent=$present;$lastCounter=$counter
 }
 Require ($gap -eq $r.id_gaps) "$name gap accounting"
 if($sampled){Require ($r.nonce_matches -eq $r.received -and $distinct.Count -gt 1) "$name content verification count"}
 $telemetry=@(Import-Csv ($prefix+'-telemetry.csv'))
 foreach($t in $telemetry){Require ([int]$t.ready+[int]$t.held -le 3 -and [int]$t.high_water -le 3 -and $t.connected -eq '1' -and [uint64]$t.epoch -eq $r.epoch) "$name telemetry connection/queue/epoch"}
 $distributions=@{};foreach($key in $timings.Keys){if($timings[$key].Count){$distributions[$key]=Distribution $timings[$key]}}
 if($prior){Require ($r.epoch -eq $prior.epoch -and $r.first_id -gt $prior.last_id) 'Clean reconnect changed source epoch/IDs'};$prior=$r
 $histogram=[ordered]@{'0-12ms'=0;'12-20ms'=0;'20-30ms'=0;'30-50ms'=0;'50-100ms'=0;'100ms+'=0}
 foreach($v in $timings.SourceInterval){$key=if($v -lt 12){'0-12ms'}elseif($v -lt 20){'12-20ms'}elseif($v -lt 30){'20-30ms'}elseif($v -lt 50){'30-50ms'}elseif($v -lt 100){'50-100ms'}else{'100ms+'};$histogram[$key]++}
 $reports.Add(@{Run=$name;Host=$r;Sampled=$sampled;UniquePatternCounters=$(if($sampled){$distinct.Count}else{$null});DistinctContentFPS=$(if($sampled){$distinct.Count/$r.seconds}else{$null});UniquePresentationIds=$presentations.Count;DuplicateContentFrames=$(if($sampled){$r.received-$distinct.Count}else{$null});DistributionsMs=$distributions;SourceIntervalHistogram=$histogram;TelemetrySamples=$telemetry.Count})
}
$resources=@(Import-Csv (Join-Path $EvidenceDirectory 'resources.csv'))
if($MinimumStreamSeconds){Require (@($resources|Where-Object {$_.Run -eq 'main' -and $_.Role -eq 'Driver'}).Count -ge 10) 'Missing driver process resource profile'}
$profiles=[Collections.Generic.List[object]]::new()
foreach($group in ($resources|Group-Object Run,Role)){
 $all=@($group.Group);$end=Number $all[-1].ElapsedSeconds;$warmup=if($end -gt 180){120}else{10}
 $samples=@($all|Where-Object {(Number $_.ElapsedSeconds) -ge $warmup})
 if($samples.Count -lt 2){$samples=$all};if($samples.Count -lt 2){continue}
 $a=$samples[0];$b=$samples[-1];$duration=(Number $b.ElapsedSeconds)-(Number $a.ElapsedSeconds)
 $cpu=(Number $b.CpuSeconds)-(Number $a.CpuSeconds)
 $profiles.Add(@{Run=$a.Run;Role=$a.Role;Samples=$samples.Count;StartSeconds=(Number $a.ElapsedSeconds);EndSeconds=(Number $b.ElapsedSeconds);CpuPercentOneCore=100*$cpu/$duration;CpuPercentWholeMachine=100*$cpu/$duration/$execution.LogicalProcessors;PrivateBytesFirst=[long]$a.PrivateBytes;PrivateBytesLast=[long]$b.PrivateBytes;PrivateBytesDelta=[long]$b.PrivateBytes-[long]$a.PrivateBytes;PrivateBytesRange=(@($samples|ForEach-Object {[long]$_.PrivateBytes})|Measure-Object -Minimum -Maximum);WorkingSetDelta=[long]$b.WorkingSetBytes-[long]$a.WorkingSetBytes;HandlesFirst=[int]$a.Handles;HandlesLast=[int]$b.Handles;HandleDelta=[int]$b.Handles-[int]$a.Handles;HandlesRange=(@($samples|ForEach-Object {[int]$_.Handles})|Measure-Object -Minimum -Maximum)})
}
$before=Get-Content (Join-Path $EvidenceDirectory 'before-health.json') -Raw|ConvertFrom-Json
foreach($file in Get-ChildItem $EvidenceDirectory -Filter '*-health.json'){
 $h=Get-Content $file.FullName -Raw|ConvertFrom-Json
 Require ($h.Boot -eq $before.Boot -and $h.CiStatus -eq 0 -and $h.CiFlags -eq $before.CiFlags -and ($h.CiFlags -band 1) -ne 0 -and ($h.CiFlags -band 2) -eq 0 -and ($h.CiFlags -band 0x400) -ne 0 -and $h.SecureBoot -and $h.Hvci -eq 1 -and $h.Devices.Count -eq 2 -and @($h.Devices|Where-Object {$_.Problem -ne 0 -or $_.Status -ne 'OK'}).Count -eq 0) 'Periodic security/boot/PnP health'
 Require ((@($h.Devices.Inf|Sort-Object)-join ',') -eq (@($before.Devices.Inf|Sort-Object)-join ',')) 'Installed package changed'
}
Require ($execution.FailureEventCount -eq 0) 'Failure events recorded'
$sourceMetadata=$null
$traceDir=Join-Path $EvidenceDirectory 'source-metadata'
if(Test-Path $traceDir){
 $trace=Get-Content (Join-Path $traceDir 'result.json') -Raw|ConvertFrom-Json
 Require ($trace.metadata_only -and $trace.status -eq 'VERIFIED' -and $trace.events_lost -eq 0 -and $trace.buffers_lost -eq 0 -and $trace.dump_frame_id -eq 0 -and !(Test-Path (Join-Path $traceDir 'diagnostic-frame.bmp'))) 'Metadata-only ETW result/loss/capture validation'
 $entries=@(Import-Csv (Join-Path $traceDir 'frames.csv'))
 Require ($entries.Count -eq $trace.frames -and $entries.Count -gt 1) 'Metadata trace count'
 $targets=[Collections.Generic.List[double]]::new();$leads=[Collections.Generic.List[double]]::new();$acquisitions=[Collections.Generic.List[double]]::new()
 $previous=$null;[long]$presentationGaps=0
 foreach($entry in $entries){
  Require ($entry.width -eq '2400' -and $entry.height -eq '1080' -and $entry.dxgi_format -eq '87' -and [uint64]$entry.qpc_frequency -eq $patternReport.frequency) 'Metadata-only frame geometry/clock'
  $leads.Add(([double]$entry.present_qpc-[double]$entry.acquired_qpc)*1000/[double]$entry.qpc_frequency)
  if($previous){
   Require ([uint64]$entry.frame_id -eq [uint64]$previous.frame_id+1 -and [uint64]$entry.acquired_qpc -gt [uint64]$previous.acquired_qpc -and [uint64]$entry.present_qpc -gt [uint64]$previous.present_qpc -and [uint64]$entry.presentation_frame_number -gt [uint64]$previous.presentation_frame_number -and $entry.producer_pid -eq $previous.producer_pid) 'Metadata-only trace ID/order/producer'
   $presentationGaps+=[long]$entry.presentation_frame_number-[long]$previous.presentation_frame_number-1
   $targets.Add(([double]$entry.present_qpc-[double]$previous.present_qpc)*1000/[double]$entry.qpc_frequency)
   $acquisitions.Add(([double]$entry.acquired_qpc-[double]$previous.acquired_qpc)*1000/[double]$entry.qpc_frequency)
  };$previous=$entry
 }
 $sourceMetadata=@{Frames=$trace.frames;FPS=$trace.fps;Seconds=$trace.elapsed_seconds;PresentationGaps=$presentationGaps;TargetPresentationIntervalsMs=(Distribution $targets);AcquisitionIntervalsMs=(Distribution $acquisitions);TargetTimeMinusAcquiredMs=(Distribution $leads);Meaning='Target presentation time is not an arrival timestamp. This trace was collected without a connected Host.'}
}
$gpuSummary=@()
$gpuFile=Join-Path $EvidenceDirectory 'gpu-engines.csv'
if(Test-Path $gpuFile){
 $gpuRows=@(Import-Csv $gpuFile|Where-Object {(Number $_.ElapsedSeconds) -ge 10})
 foreach($group in ($gpuRows|Group-Object Run,Engine)){
  $a=$group.Group[0]
  if($a.Engine -notmatch '^pid_(\d+)_.+_engtype_(.+)$'){continue}
  $enginePid=$Matches[1];$engineType=$Matches[2]
  $role=@($resources|Where-Object {$_.Run -eq $a.Run -and $_.ProcessId -eq $enginePid}|Select-Object -First 1).Role
  if(!$role){continue}
  $measure=@($group.Group|ForEach-Object {Number $_.UtilizationPercent})|Measure-Object -Average -Maximum
  if($measure.Maximum -gt 0){$gpuSummary+=@{Run=$a.Run;Role=$role;EngineType=$engineType;MeanPercent=$measure.Average;PeakPercent=$measure.Maximum;Samples=$measure.Count}}
 }
}
$summary=@{Outcome='PASS';Scope='Evidence consistency, bounded queue, real content, IDs/QPC, clean reconnect, health; capacity/soak acceptance requires review of measured rates and resource trends';Pattern=$patternReport;Runs=$reports.ToArray();Profiles=$profiles.ToArray();GpuEngines=$gpuSummary;SourceMetadata=$sourceMetadata;GpuCopyTiming='Not isolated. Mutex acquisition and diagnostic 1280-pixel sample completion are separate measurements, not full-frame GPU copy timestamps.'}
$summary|ConvertTo-Json -Depth 12|Set-Content (Join-Path $EvidenceDirectory 'verification.json') -Encoding utf8
$reports|ForEach-Object {[pscustomobject]@{Run=$_.Run;Seconds=$_.Host.seconds;SourceFPS=$_.Host.source_fps;HostFPS=$_.Host.fps;DistinctFPS=$_.DistinctContentFPS;QueuePeak=$_.Host.high_water;P50ms=$_.DistributionsMs.SourceInterval.P50;P95ms=$_.DistributionsMs.SourceInterval.P95;P99ms=$_.DistributionsMs.SourceInterval.P99;MaxGapMs=$_.DistributionsMs.SourceInterval.Max}}|Format-Table -AutoSize
