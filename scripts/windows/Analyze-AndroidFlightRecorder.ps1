#requires -Version 7.4
param([Parameter(Mandatory=$true)][string]$RunName,[ValidateSet('device-phase2cperf2','device-phase2cperf3')][string]$EvidencePhase='device-phase2cperf2')
$ErrorActionPreference='Stop'
[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
if($RunName-notmatch'^[a-zA-Z0-9-]+$'){throw 'Invalid run name'}
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$run=Join-Path $repo ('docs\evidence\private\'+$EvidencePhase+'\'+$RunName)
$hostDir=Join-Path $run 'host';$metaPath=Join-Path $hostDir 'flight-recorder-meta.json';$binary=Join-Path $hostDir 'flight-recorder.bin'
foreach($required in @($metaPath,(Join-Path $hostDir 'session-1-result.json'),(Join-Path $hostDir 'encode-result.json'),(Join-Path $hostDir 'transport-result.json'))){if(!(Test-Path -LiteralPath $required -PathType Leaf)){throw("Missing evidence: "+$required)}}
$meta=Get-Content -Raw -LiteralPath $metaPath|ConvertFrom-Json
$source=Get-Content -Raw -LiteralPath (Join-Path $hostDir 'session-1-result.json')|ConvertFrom-Json
$encode=Get-Content -Raw -LiteralPath (Join-Path $hostDir 'encode-result.json')|ConvertFrom-Json
$transport=Get-Content -Raw -LiteralPath (Join-Path $hostDir 'transport-result.json')|ConvertFrom-Json
$ratio=if([double]$source.source_fps){[double]$source.fps/[double]$source.source_fps}else{0}
if(!$meta.triggered){
 if(Test-Path -LiteralPath $binary){throw 'NO TRIGGER run unexpectedly contains a raw trace'}
 if([long]$meta.records-ne0-or[long]$meta.dropped-ne0){throw 'NO TRIGGER metadata is inconsistent'}
 $summary=[ordered]@{version=2;run=$RunName;outcome='NO_TRIGGER';source_fps=$source.source_fps;host_fps=$source.fps;host_source_ratio=$ratio;encoder_output_fps=$encode.output_fps;sender_fps=$transport.sender_fps;flight_recorder=$meta}
 $summary|ConvertTo-Json -Depth 12|Set-Content -LiteralPath (Join-Path $run 'flight-recorder-summary.json') -Encoding utf8
 Write-Output ('ANALYZED_NO_TRIGGER: '+$RunName+' source='+('{0:F3}'-f$source.source_fps)+' host='+('{0:F3}'-f$source.fps)+' ratio='+('{0:F6}'-f$ratio))
 exit 0
}
if(!(Test-Path -LiteralPath $binary -PathType Leaf)){throw 'TRIGGERED run is missing its raw trace'}
if([long]$meta.dropped-ne0){throw 'Flight-recorder trace loss is nonzero'}
if([double]$meta.pre_coverage_seconds-lt15){throw 'Pre-trigger coverage is below 15 seconds'}
if([double]$meta.post_coverage_seconds-lt59){throw 'Post-trigger coverage is below 59 seconds'}
$names=@{
 1='HostIteration';2='HostFrameAcquire';3='HostAdmission';4='KeyedMutexWait';5='HostSampleDiagnostic';6='DriverAck';7='HostEvidence';8='HostReturn'
 10='EncoderPump';11='EncoderEventService';12='EncoderOutputTotal';13='EncoderProcessOutput';14='EncoderOutputParse';15='EncoderEvidenceWrite';16='SinkConsume';17='ConversionSubmit';18='ConversionWait';19='EncoderProcessInput';20='EncoderSubmission'
 21='ConsumeTotal';22='ConsumeMutexWait';23='ConsumeMutexHold';24='FrameConstruction';25='TransportEvidence';26='QueueAdmission'
 30='WorkerQueueMutexWait';31='WorkerQueueMutexHold';32='WorkerConditionWait';33='LogicalFrameSend';34='SocketWaitWrite';35='SocketWriteCall';36='ChannelTxEvidence';37='AckReceive';38='SocketWaitRead';39='SocketReadCall';40='ChannelRxEvidence';41='SentMutexWait';42='SentMutexHold';43='AckMutexWait';44='AckMutexHold';45='AckProcess';46='SocketConfiguration';47='ChannelSend';48='ChannelReceive';49='SocketWriteResult';50='SocketReadResult';51='TransportFlush';52='ChannelFlush';60='DetectorTrigger'
}
function Distribution([double[]]$Values){
 if(!$Values-or!$Values.Count){return [ordered]@{count=0;mean_ms=0;p50_ms=0;p95_ms=0;p99_ms=0;max_ms=0;total_ms=0}}
 [array]::Sort($Values);$sum=($Values|Measure-Object -Sum).Sum
 function Pick([double]$q){$index=[Math]::Max(0,[Math]::Min($Values.Count-1,[Math]::Ceiling($Values.Count*$q)-1));return $Values[$index]}
 return [ordered]@{count=$Values.Count;mean_ms=$sum/$Values.Count;p50_ms=(Pick .50);p95_ms=(Pick .95);p99_ms=(Pick .99);max_ms=$Values[-1];total_ms=$sum}
}
function Add-Value([hashtable]$Table,[string]$Key,[double]$Value){if(!$Table.ContainsKey($Key)){$Table[$Key]=[Collections.Generic.List[double]]::new()};$Table[$Key].Add($Value)}
$all=@{};$healthy=@{};$degraded=@{};$onset=@{};$healthyFrame=[Collections.Generic.List[double]]::new();$healthyIdle=[Collections.Generic.List[double]]::new();$degradedFrame=[Collections.Generic.List[double]]::new();$degradedIdle=[Collections.Generic.List[double]]::new();$pumpRecords=[Collections.Generic.List[object]]::new();$submissionRecords=[Collections.Generic.List[object]]::new();$send=@{};$ack=@{}
$frequency=[uint64]$meta.frequency;$trigger=[uint64]$meta.trigger_qpc;$healthyEnd=$trigger-[uint64]([double]$meta.detector_window_seconds*$frequency)
$stream=[IO.File]::OpenRead($binary);$reader=[IO.BinaryReader]::new($stream)
try{
 $magic=$reader.ReadUInt32();$version=$reader.ReadUInt32();$fileFrequency=$reader.ReadUInt64();$started=$reader.ReadUInt64();$count=$reader.ReadUInt64();$dropped=$reader.ReadUInt64();$recordSize=$reader.ReadUInt32();$reduced=$reader.ReadUInt32()
 if($magic-ne0x31544d53-or$version-ne2-or$recordSize-ne48-or$fileFrequency-ne$frequency){throw 'Unsupported/corrupt flight-recorder header'}
 if($stream.Length-ne48L+[int64]$count*48L-or$count-ne[uint64]$meta.records-or$dropped-ne0){throw 'Flight-recorder length/count mismatch'}
 for([uint64]$i=0;$i-lt$count;$i++){
  $start=$reader.ReadUInt64();$duration=$reader.ReadUInt64();$frame=$reader.ReadUInt64();$session=$reader.ReadUInt64();$event=$reader.ReadUInt32();$thread=$reader.ReadUInt32();$value0=$reader.ReadUInt32();$value1=$reader.ReadUInt32()
  if(!$names.ContainsKey([int]$event)){throw("Unknown flight-recorder event "+$event)}
  $name=$names[[int]$event];$milliseconds=[double]$duration*1000.0/$frequency;Add-Value $all $name $milliseconds
  $eventServiceName=if($event-eq11){switch([uint32]$value0){0{'EncoderEventServiceNoEvent'}601{'EncoderEventServiceNeedInput'}602{'EncoderEventServiceHaveOutput'}603{'EncoderEventServiceDrainComplete'}default{'EncoderEventServiceType'+[string]$value0}}}else{$null}
  if($eventServiceName){Add-Value $all $eventServiceName $milliseconds}
  if($event-eq10){$pumpRecords.Add([pscustomobject]@{start=$start;end=$start+$duration;milliseconds=$milliseconds})}
  elseif($event-eq20){$submissionRecords.Add([pscustomobject]@{start=$start;end=$start+$duration})}
  if($start-lt$healthyEnd){
   Add-Value $healthy $name $milliseconds;if($eventServiceName){Add-Value $healthy $eventServiceName $milliseconds}
   if($event-eq1){if($frame){$healthyFrame.Add($milliseconds)}else{$healthyIdle.Add($milliseconds)}}
  }
  elseif($start-ge$trigger){
   Add-Value $degraded $name $milliseconds;if($eventServiceName){Add-Value $degraded $eventServiceName $milliseconds}
   if($event-eq1){if($frame){$degradedFrame.Add($milliseconds)}else{$degradedIdle.Add($milliseconds)}}
  }
  $relativeSecond=[int][Math]::Floor(([double]$start-[double]$trigger)/[double]$frequency)
  if($relativeSecond-ge-30-and$relativeSecond-le10){
   $onsetName=switch([int]$event){
    1 {if($frame){'HostFrameIteration'}else{'HostIdleIteration'}}
    2 {'HostFrameAcquire'}
    3 {'HostAdmission'}
    11 {$eventServiceName}
    12 {'EncoderOutputTotal'}
    13 {'EncoderProcessOutput'}
    18 {'ConversionWait'}
    19 {'EncoderProcessInput'}
    default {$null}
   }
   if($onsetName){Add-Value $onset ([string]$relativeSecond+'|'+$onsetName) $milliseconds}
  }
  if($event-eq33-and$frame){$send[[string]$frame]=$start}
  if($event-eq45-and$frame){$ack[[string]$frame]=$start+$duration}
 }
}finally{$reader.Dispose();$stream.Dispose()}
$submissions=@($submissionRecords|Sort-Object start);$submissionIndex=0
foreach($pump in @($pumpRecords|Sort-Object start)){
 while($submissionIndex-lt$submissions.Count-and[uint64]$submissions[$submissionIndex].end-lt[uint64]$pump.start){$submissionIndex++}
 $inside=$submissionIndex-lt$submissions.Count-and[uint64]$pump.start-ge[uint64]$submissions[$submissionIndex].start-and[uint64]$pump.end-le[uint64]$submissions[$submissionIndex].end
 $splitName=if($inside){'EncoderPumpSubmit'}else{'EncoderPumpTopLevel'}
 Add-Value $all $splitName $pump.milliseconds
 if([uint64]$pump.start-lt$healthyEnd){Add-Value $healthy $splitName $pump.milliseconds}
 elseif([uint64]$pump.start-ge$trigger){Add-Value $degraded $splitName $pump.milliseconds}
 $relativeSecond=[int][Math]::Floor(([double]$pump.start-[double]$trigger)/[double]$frequency)
 if($relativeSecond-ge-30-and$relativeSecond-le10){Add-Value $onset ([string]$relativeSecond+'|'+$splitName) $pump.milliseconds}
}
$healthy['HostFrameIteration']=$healthyFrame;$healthy['HostIdleIteration']=$healthyIdle;$degraded['HostFrameIteration']=$degradedFrame;$degraded['HostIdleIteration']=$degradedIdle
$rows=[Collections.Generic.List[object]]::new();$healthySummary=[ordered]@{};$degradedSummary=[ordered]@{}
foreach($name in @($names.Values+'HostFrameIteration'+'HostIdleIteration'+'EncoderPumpTopLevel'+'EncoderPumpSubmit'+'EncoderEventServiceNoEvent'+'EncoderEventServiceNeedInput'+'EncoderEventServiceHaveOutput'+'EncoderEventServiceDrainComplete'|Sort-Object -Unique)){
 $h=Distribution $(if($healthy.ContainsKey($name)){$healthy[$name].ToArray()}else{@()});$d=Distribution $(if($degraded.ContainsKey($name)){$degraded[$name].ToArray()}else{@()});$healthySummary[$name]=$h;$degradedSummary[$name]=$d
 $rows.Add([pscustomobject]@{stage=$name;segment='healthy-pre';count=$h.count;mean_ms=$h.mean_ms;p50_ms=$h.p50_ms;p95_ms=$h.p95_ms;p99_ms=$h.p99_ms;max_ms=$h.max_ms})
 $rows.Add([pscustomobject]@{stage=$name;segment='degraded-post';count=$d.count;mean_ms=$d.mean_ms;p50_ms=$d.p50_ms;p95_ms=$d.p95_ms;p99_ms=$d.p99_ms;max_ms=$d.max_ms})
}
$onsetRows=[Collections.Generic.List[object]]::new()
foreach($relativeSecond in -30..10){
 foreach($name in @('HostFrameIteration','HostIdleIteration','HostFrameAcquire','HostAdmission','EncoderPumpTopLevel','EncoderPumpSubmit','EncoderEventServiceNoEvent','EncoderEventServiceNeedInput','EncoderEventServiceHaveOutput','ConversionWait','EncoderProcessInput','EncoderProcessOutput','EncoderOutputTotal')){
  $key=[string]$relativeSecond+'|'+$name;$distribution=Distribution $(if($onset.ContainsKey($key)){$onset[$key].ToArray()}else{@()})
  $onsetRows.Add([pscustomobject]@{relative_second=$relativeSecond;stage=$name;count=$distribution.count;mean_ms=$distribution.mean_ms;p50_ms=$distribution.p50_ms;p95_ms=$distribution.p95_ms;p99_ms=$distribution.p99_ms;max_ms=$distribution.max_ms})
 }
}
$sendAckHealthy=[Collections.Generic.List[double]]::new();$sendAckDegraded=[Collections.Generic.List[double]]::new();foreach($key in $send.Keys){if($ack.ContainsKey($key)-and$ack[$key]-ge$send[$key]){$value=[double]($ack[$key]-$send[$key])*1000.0/$frequency;if($send[$key]-lt$healthyEnd){$sendAckHealthy.Add($value)}elseif($send[$key]-ge$trigger){$sendAckDegraded.Add($value)}}}
$summary=[ordered]@{version=2;run=$RunName;outcome='TRIGGERED';source_fps=$source.source_fps;host_fps=$source.fps;host_source_ratio=$ratio;encoder_output_fps=$encode.output_fps;sender_fps=$transport.sender_fps;flight_recorder=$meta;healthy_segment_end_qpc=$healthyEnd;degraded_segment_start_qpc=$trigger;healthy_send_to_ack=Distribution $sendAckHealthy.ToArray();degraded_send_to_ack=Distribution $sendAckDegraded.ToArray();outstanding_ack_policy='one synchronous FRAME followed by its exact ACK';outstanding_ack_peak=1;healthy=$healthySummary;degraded=$degradedSummary}
$rows|Export-Csv -LiteralPath (Join-Path $run 'flight-recorder-stages.csv') -NoTypeInformation -Encoding utf8
$onsetRows|Export-Csv -LiteralPath (Join-Path $run 'flight-recorder-onset.csv') -NoTypeInformation -Encoding utf8
$summary|ConvertTo-Json -Depth 15|Set-Content -LiteralPath (Join-Path $run 'flight-recorder-summary.json') -Encoding utf8
Write-Output ('ANALYZED_TRIGGERED: '+$RunName+' records='+$count+' pre='+('{0:F3}'-f$meta.pre_coverage_seconds)+'s post='+('{0:F3}'-f$meta.post_coverage_seconds)+'s')
