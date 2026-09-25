#requires -Version 7.4
<# Builds a one-second, same-domain Windows timeline plus independently timed
Android interval rates. Android is aligned by the session-ready epoch only; no
Windows-QPC minus Android-elapsedRealtime latency is ever calculated. #>
param([string]$RunName,[string]$EvidenceDirectory)
$ErrorActionPreference='Stop'
[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$run=if($EvidenceDirectory){[IO.Path]::GetFullPath($EvidenceDirectory)}elseif($RunName){Join-Path $repo ('docs\evidence\private\device-phase2cperf\'+$RunName)}else{throw 'RunName or EvidenceDirectory is required'}
if(!(Test-Path -LiteralPath $run -PathType Container)){throw 'Performance run not found'}
$hostDir=Join-Path $run 'host'
$source=Get-Content -Raw -LiteralPath (Join-Path $hostDir 'session-1-result.json')|ConvertFrom-Json
$frequency=[double]$source.frequency;$t0=[double]$source.first_qpc;$seconds=[int][Math]::Floor([double]$source.seconds)
if($seconds-lt60){throw 'Run is too short for sustained-cadence analysis'}
$timeline=@(for($i=0;$i-lt$seconds;$i++){[ordered]@{Second=$i;ContentPresent=0;IddCxSourceFps=$null;HostReceived=0;UniqueSource=0;HostDrops=0;ConversionCompleted=0;ConversionMsAvg=$null;EncoderSubmitted=0;EncoderDropped=0;EncoderOutput=0;EncoderLatencyMsAvg=$null;SenderAdmitted=0;SenderDropped=0;WireComplete=0;Acked=0;TransportOutstanding=$null;HostQueueDepth=$null;HostQueueHighWater=$null;EncoderPending=$null;EncoderPoolBusy=$null;AndroidFramesFps=$null;AndroidBytesPerSecond=$null;DecoderInputFps=$null;DecoderOutputFps=$null;RenderCallbackFps=$null;AndroidQueueDepth=$null;AndroidQueuePeak=$null;AndroidCpuPercentOneCore=$null;AndroidPssKb=$null;AndroidPrivateDirtyKb=$null;ThermalStatus=$null;BatteryTemperatureDeciC=$null;RefreshHz=$null;HostCpuPercentOneCore=$null;HostPrivateBytes=$null;HostWorkingSetBytes=$null;HostHandles=$null;HostThreads=$null;GpuEngineUtilizationSum=$null}})
function Bin-Qpc([double]$value){return [int][Math]::Floor(($value-$t0)/$frequency)}
function Bin-Ns([double]$value){return [int][Math]::Floor($value/1e9-$t0/$frequency)}
function In-Range([int]$bin){return $bin-ge0-and$bin-lt$timeline.Count}
foreach($r in Import-Csv -LiteralPath (Join-Path $run 'pattern.csv')){$b=Bin-Qpc ([double]$r.qpc);if(In-Range $b){$timeline[$b].ContentPresent++}}
$frames=@(Import-Csv -LiteralPath (Join-Path $hostDir 'session-1-frames.csv'))
foreach($r in $frames){$b=Bin-Qpc ([double]$r.receive_qpc);if(In-Range $b){$timeline[$b].HostReceived++}}
foreach($r in Import-Csv -LiteralPath (Join-Path $hostDir 'classifications.csv')){$b=Bin-Qpc ([double]$r.source_qpc);if((In-Range $b)-and$r.class-in@('1','3')){$timeline[$b].UniqueSource++}}
$telemetry=@(Import-Csv -LiteralPath (Join-Path $hostDir 'session-1-telemetry.csv'))
$prevQ=$t0;$prevSource=0.0;$prevProducer=0.0;$prevStale=0.0;$prevContention=0.0
foreach($r in $telemetry){$q=[double]$r.qpc;$dt=($q-$prevQ)/$frequency;$b=Bin-Qpc $q;if($dt-gt0-and(In-Range $b)){$timeline[$b].IddCxSourceFps=([double]$r.source_frames-$prevSource)/$dt;$timeline[$b].HostDrops=([double]$r.producer_drops-$prevProducer)+([double]$r.host_stale_drops-$prevStale)+([double]$r.contention-$prevContention);$timeline[$b].HostQueueDepth=[int]$r.ready+[int]$r.held;$timeline[$b].HostQueueHighWater=[int]$r.high_water};$prevQ=$q;$prevSource=[double]$r.source_frames;$prevProducer=[double]$r.producer_drops;$prevStale=[double]$r.host_stale_drops;$prevContention=[double]$r.contention}
$conversion=@{};$latency=@{}
foreach($r in Import-Csv -LiteralPath (Join-Path $hostDir 'encode-input.csv')){if($r.decision-eq'accepted'){$b=Bin-Qpc ([double]$r.submit_qpc);if(In-Range $b){$timeline[$b].ConversionCompleted++;$timeline[$b].EncoderSubmitted++;$timeline[$b].EncoderPending=[Math]::Max([int]$timeline[$b].EncoderPending,[int]$r.pending);$timeline[$b].EncoderPoolBusy=[Math]::Max([int]$timeline[$b].EncoderPoolBusy,[int]$r.pool_busy);if(!$conversion.ContainsKey($b)){$conversion[$b]=[Collections.Generic.List[double]]::new()};$conversion[$b].Add([double]$r.conversion_ms)}}else{$b=Bin-Qpc ([double]$r.source_qpc);if(In-Range $b){$timeline[$b].EncoderDropped++}}}
foreach($b in $conversion.Keys){$timeline[$b].ConversionMsAvg=($conversion[$b]|Measure-Object -Average).Average}
foreach($r in Import-Csv -LiteralPath (Join-Path $hostDir 'encode-output.csv')){$b=Bin-Qpc ([double]$r.output_qpc);if(In-Range $b){$timeline[$b].EncoderOutput++;if(!$latency.ContainsKey($b)){$latency[$b]=[Collections.Generic.List[double]]::new()};$latency[$b].Add([double]$r.latency_ms)}}
foreach($b in $latency.Keys){$timeline[$b].EncoderLatencyMsAvg=($latency[$b]|Measure-Object -Average).Average}
$admittedCumulative=0;$ackedCumulative=0
foreach($r in Import-Csv -LiteralPath (Join-Path $hostDir 'transport-frames.csv')){if($r.stage-eq'input'){$b=Bin-Ns ([double]$r.enqueue_ns);if(In-Range $b){if($r.decision-eq'admitted'){$timeline[$b].SenderAdmitted++}else{$timeline[$b].SenderDropped++}}}elseif($r.stage-eq'wire'){$b=Bin-Ns ([double]$r.send_ns);if(In-Range $b){$timeline[$b].WireComplete++}}elseif($r.stage-eq'terminal'-and$r.decision-eq'acked'){$b=Bin-Ns ([double]$r.ack_ns);if(In-Range $b){$timeline[$b].Acked++}}}
foreach($row in $timeline){$admittedCumulative+=$row.SenderAdmitted;$ackedCumulative+=$row.Acked;$row.TransportOutstanding=$admittedCumulative-$ackedCumulative}
$sessions=@(Import-Csv -LiteralPath (Join-Path $hostDir 'transport-sessions.csv'));$hostReady=@($sessions|Where-Object event-eq'READY'|Select-Object -First 1);$hostReadyElapsed=if($hostReady){[double]$hostReady.time_ns/1e9-$t0/$frequency}else{0}
$log=Get-Content -LiteralPath (Join-Path $run 'android-swdprx.log');$androidReadyEpoch=$null;$metrics=[Collections.Generic.List[object]]::new()
foreach($line in $log){if($line-match'^\s*([0-9]+\.[0-9]+)\s+\d+\s+\d+\s+[A-Z]\s+SWDPRX\s+:\s+(.*)$'){$epoch=[double]$Matches[1];$message=$Matches[2];if($null-eq$androidReadyEpoch-and$message.StartsWith('SESSION_READY')){$androidReadyEpoch=$epoch};if($message.StartsWith('METRICS event=PERIODIC')){$values=@{};foreach($m in [regex]::Matches($message,'([A-Za-z][A-Za-z0-9]*)=([^\s]+)')){$values[$m.Groups[1].Value]=$m.Groups[2].Value};$metrics.Add([pscustomobject]@{Epoch=$epoch;Values=$values})}}}
if($null-ne$androidReadyEpoch-and$metrics.Count-ge2){for($i=1;$i-lt$metrics.Count;$i++){$a=$metrics[$i-1];$b=$metrics[$i];$dt=([double]$b.Values.elapsedRealtimeNs-[double]$a.Values.elapsedRealtimeNs)/1e9;if($dt-le0){continue};$elapsed=$hostReadyElapsed+($b.Epoch-$androidReadyEpoch);$bin=[int][Math]::Floor($elapsed);if(!(In-Range $bin)){continue};$row=$timeline[$bin];$row.AndroidFramesFps=([double]$b.Values.frames-[double]$a.Values.frames)/$dt;$row.AndroidBytesPerSecond=([double]$b.Values.frameBytes-[double]$a.Values.frameBytes)/$dt;$row.DecoderInputFps=([double]$b.Values.decoderInputs-[double]$a.Values.decoderInputs)/$dt;$row.DecoderOutputFps=([double]$b.Values.decoderOutputs-[double]$a.Values.decoderOutputs)/$dt;$row.RenderCallbackFps=([double]$b.Values.renderCallbacks-[double]$a.Values.renderCallbacks)/$dt;$row.AndroidCpuPercentOneCore=([double]$b.Values.processCpuMs-[double]$a.Values.processCpuMs)/1000/$dt*100;$row.AndroidQueueDepth=[int]$b.Values.queueDepth;$row.AndroidQueuePeak=[int]$b.Values.queuePeak;$row.AndroidPssKb=[int]$b.Values.pssKb;$row.AndroidPrivateDirtyKb=[int]$b.Values.privateDirtyKb;$row.ThermalStatus=[int]$b.Values.thermalStatus;$row.BatteryTemperatureDeciC=[int]$b.Values.batteryTemperatureDeciC;$row.RefreshHz=[double]$b.Values.refreshHz}}
$resources=@(Import-Csv -LiteralPath (Join-Path $run 'windows-resources.csv')|Where-Object Role -eq 'Host'|Sort-Object {[double]$_.ElapsedSeconds});for($i=0;$i-lt$resources.Count;$i++){$r=$resources[$i];$bin=[int][Math]::Floor([double]$r.ElapsedSeconds);if(!(In-Range $bin)){continue};$row=$timeline[$bin];$row.HostPrivateBytes=[long]$r.PrivateBytes;$row.HostWorkingSetBytes=[long]$r.WorkingSetBytes;$row.HostHandles=[int]$r.Handles;$row.HostThreads=[int]$r.Threads;if($i){$dt=[double]$r.ElapsedSeconds-[double]$resources[$i-1].ElapsedSeconds;if($dt-gt0){$row.HostCpuPercentOneCore=([double]$r.CpuSeconds-[double]$resources[$i-1].CpuSeconds)/$dt*100}}}
$gpuPath=Join-Path $run 'gpu-engines.csv';if(Test-Path $gpuPath){foreach($group in (Import-Csv $gpuPath|Group-Object {[int][Math]::Floor([double]$_.ElapsedSeconds)})){$bin=[int]$group.Name;if(In-Range $bin){$timeline[$bin].GpuEngineUtilizationSum=($group.Group|Measure-Object UtilizationPercentage -Sum).Sum}}}
$objects=@($timeline|ForEach-Object{[pscustomobject]$_});$objects|Export-Csv -LiteralPath (Join-Path $run 'performance-timeline.csv') -NoTypeInformation -Encoding utf8
function Median([double[]]$v){$a=@($v|Sort-Object);if(!$a.Count){return $null};if($a.Count%2){return $a[[int]($a.Count/2)]};return($a[$a.Count/2-1]+$a[$a.Count/2])/2}
function Rolling([string]$field,[int]$end,[int]$width=10){$start=[Math]::Max(0,$end-$width+1);$values=@();for($i=$start;$i-le$end;$i++){$v=$objects[$i].$field;if($null-ne$v){$values+=[double]$v}};if(!$values.Count){return $null};return($values|Measure-Object -Average).Average}
$stages=[ordered]@{ContentPresent='Display content Present';IddCxSourceFps='IddCx source availability';HostReceived='Host capture/admission';ConversionCompleted='GPU conversion completion';EncoderSubmitted='AMD encoder submission';EncoderOutput='AMD encoder output';SenderAdmitted='sender admission';WireComplete='TCP/ADB wire completion';Acked='Android ACK';AndroidFramesFps='Android FRAME receive';DecoderInputFps='MediaCodec input';DecoderOutputFps='MediaCodec output';RenderCallbackFps='Surface render callback'}
$references=[ordered]@{};$degradation=[ordered]@{};foreach($field in $stages.Keys){$samples=@();for($s=15;$s-le[Math]::Min(75,$seconds-1);$s++){$v=Rolling $field $s;if($null-ne$v){$samples+=$v}};$reference=Median $samples;$references[$field]=$reference;$first=$null;if($null-ne$reference-and$reference-gt0){for($s=90;$s-lt$seconds-15;$s++){$all=$true;for($j=0;$j-lt15;$j++){$v=Rolling $field ($s+$j);if($null-eq$v-or$v-ge0.8*$reference){$all=$false;break}};if($all){$first=$s;break}}};$degradation[$field]=$first}
$times=@($degradation.GetEnumerator()|Where-Object{$null-ne$_.Value}|ForEach-Object{[int]$_.Value});$firstTime=if($times.Count){($times|Measure-Object -Minimum).Minimum}else{$null};$firstField=$null;if($null-ne$firstTime){foreach($field in $stages.Keys){if($null-ne$degradation[$field]-and[int]$degradation[$field]-le$firstTime+2){$firstField=$field;break}}}
$withinStageFirstTime=$firstTime;$withinStageFirstField=$firstField
# A run can enter the slow state before the seconds-15..75 self-reference window.
# Detect that case by comparing adjacent pipeline stages in the same controller
# timeline. A boundary is material when its 10-second downstream/upstream ratio
# is below 80% for 15 consecutive samples. This does not compare device clocks.
$boundaries=[ordered]@{
 'IddCxSourceFps|HostReceived'='IddCx source -> Host capture/admission'
 'HostReceived|ConversionCompleted'='Host admission -> GPU conversion completion'
 'ConversionCompleted|EncoderSubmitted'='GPU conversion -> AMD encoder submission'
 'EncoderSubmitted|EncoderOutput'='AMD encoder submission -> output'
 'EncoderOutput|SenderAdmitted'='AMD encoder output -> sender admission'
 'SenderAdmitted|WireComplete'='sender admission -> wire completion'
 'WireComplete|Acked'='wire completion -> Android ACK'
 'AndroidFramesFps|DecoderInputFps'='Android FRAME receive -> MediaCodec input'
 'DecoderInputFps|DecoderOutputFps'='MediaCodec input -> output'
 'DecoderOutputFps|RenderCallbackFps'='MediaCodec output -> Surface render callback'
}
$boundaryResults=[ordered]@{};$firstBoundary=$null;$firstBoundaryDownstream=$null;$firstBoundaryConfirmed=$null;$firstBoundaryInstant=$null
foreach($pair in $boundaries.Keys){
 $parts=$pair.Split('|');$upstream=$parts[0];$downstream=$parts[1];$start=$null;$confirmed=$null
 for($s=9;$s-le$seconds-15;$s++){
  $all=$true
  for($j=0;$j-lt15;$j++){$u=Rolling $upstream ($s+$j);$d=Rolling $downstream ($s+$j);if($null-eq$u-or$null-eq$d-or$u-le0-or$d-ge0.8*$u){$all=$false;break}}
  if($all){$start=$s;$confirmed=$s+14;break}
 }
 $instant=$null
 for($s=0;$s-lt$seconds;$s++){$u=$objects[$s].$upstream;$d=$objects[$s].$downstream;if($null-ne$u-and$null-ne$d-and[double]$u-gt0-and[double]$d-lt0.8*[double]$u){$instant=$s;break}}
 $boundaryResults[$pair]=[ordered]@{Label=$boundaries[$pair];FirstInstantaneousSecond=$instant;FirstRollingWindowSecond=$start;ConfirmedSecond=$confirmed}
 if($null-ne$start-and($null-eq$firstBoundary-or$start-lt$firstBoundary)){$firstBoundary=$start;$firstBoundaryDownstream=$downstream;$firstBoundaryConfirmed=$confirmed;$firstBoundaryInstant=$instant}
}
if($null-ne$firstBoundary){$firstTime=$firstBoundary;$firstField=$firstBoundaryDownstream}
function WindowAverage([string]$field,[int]$from,[int]$to){$v=@();for($i=[Math]::Max(0,$from);$i-le[Math]::Min($seconds-1,$to);$i++){$x=$objects[$i].$field;if($null-ne$x){$v+=[double]$x}};if(!$v.Count){return $null};return($v|Measure-Object -Average).Average}
$windows=[ordered]@{};if($null-ne$firstTime){foreach($field in $stages.Keys){$windows[$field]=[ordered]@{Before30s=WindowAverage $field ($firstTime-30) ($firstTime-1);Transition10s=WindowAverage $field $firstTime ($firstTime+9);After60s=WindowAverage $field ($firstTime+10) ($firstTime+69)}}}
$hostRows=@($objects|Where-Object{$null-ne$_.HostPrivateBytes});$androidRows=@($objects|Where-Object{$null-ne$_.AndroidPssKb});$summary=[ordered]@{Outcome=$(if($null-ne$firstBoundary){'BOUNDARY_DIVERGENCE_LOCALIZED'}elseif($null-ne$firstField){'DEGRADATION_LOCALIZED_BY_SELF_REFERENCE'}else{'NO_PERSISTENT_20_PERCENT_DEGRADATION'});Definition='Adjacent-stage 10-second rolling downstream/upstream ratio below 80% for 15 consecutive samples; self-reference detector retained separately';DurationSeconds=$seconds;FirstDegradationSecond=$firstTime;FirstInstantaneousBoundarySecond=$firstBoundaryInstant;BoundaryConfirmedSecond=$firstBoundaryConfirmed;FirstStage=$firstField;FirstStageLabel=$(if($firstField){$stages[$firstField]}else{$null});BoundaryResults=$boundaryResults;WithinStageReferenceDefinition='10-second rolling rate below 80% of its seconds-15..75 median for 15 consecutive samples';WithinStageFirstDegradationSecond=$withinStageFirstTime;WithinStageFirstStage=$withinStageFirstField;ReferenceRates=$references;PerStageFirstDegradationSecond=$degradation;TransitionWindows=$windows;WindowsHost=$(if($hostRows.Count){[ordered]@{PrivateFirst=$hostRows[0].HostPrivateBytes;PrivateLast=$hostRows[-1].HostPrivateBytes;PrivatePeak=($hostRows.HostPrivateBytes|Measure-Object -Maximum).Maximum;HandlesFirst=$hostRows[0].HostHandles;HandlesLast=$hostRows[-1].HostHandles;HandlesPeak=($hostRows.HostHandles|Measure-Object -Maximum).Maximum;ThreadsFirst=$hostRows[0].HostThreads;ThreadsLast=$hostRows[-1].HostThreads;ThreadsPeak=($hostRows.HostThreads|Measure-Object -Maximum).Maximum}}else{$null});Android=$(if($androidRows.Count){[ordered]@{PssFirstKb=$androidRows[0].AndroidPssKb;PssLastKb=$androidRows[-1].AndroidPssKb;PssPeakKb=($androidRows.AndroidPssKb|Measure-Object -Maximum).Maximum;ThermalMin=($androidRows.ThermalStatus|Measure-Object -Minimum).Minimum;ThermalMax=($androidRows.ThermalStatus|Measure-Object -Maximum).Maximum;BatteryTemperatureMinDeciC=($androidRows.BatteryTemperatureDeciC|Measure-Object -Minimum).Minimum;BatteryTemperatureMaxDeciC=($androidRows.BatteryTemperatureDeciC|Measure-Object -Maximum).Maximum}}else{$null});CrossDeviceLatencyCalculated=$false}
$summary|ConvertTo-Json -Depth 12|Set-Content -LiteralPath (Join-Path $run 'performance-summary.json') -Encoding utf8
Write-Output ($summary.Outcome+': first='+$summary.FirstStageLabel+' second='+$summary.FirstDegradationSecond)
