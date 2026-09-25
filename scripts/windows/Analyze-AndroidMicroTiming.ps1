#requires -Version 7.4
param([Parameter(Mandatory=$true)][string]$RunName)
$ErrorActionPreference='Stop'
[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
if($RunName-notmatch'^[a-zA-Z0-9-]+$'){throw'Invalid run name'}
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$run=Join-Path $repo ('docs\evidence\private\device-phase2cperf1\'+$RunName)
$hostDir=Join-Path $run 'host';$binary=Join-Path $hostDir 'micro-timing.bin';$metaPath=Join-Path $hostDir 'micro-timing-meta.json'
foreach($required in @($binary,$metaPath,(Join-Path $hostDir 'session-1-result.json'),(Join-Path $hostDir 'encode-result.json'),(Join-Path $hostDir 'transport-result.json'))){if(!(Test-Path -LiteralPath $required -PathType Leaf)){throw("Missing evidence: "+$required)}}
$names=@{
 1='HostIteration';2='HostFrameAcquire';3='HostAdmission';4='KeyedMutexWait';5='HostSampleDiagnostic';6='DriverAck';7='HostEvidence';8='HostReturn'
 10='EncoderPump';11='EncoderEventService';12='EncoderOutputTotal';13='EncoderProcessOutput';14='EncoderOutputParse';15='EncoderEvidenceWrite';16='SinkConsume';17='ConversionSubmit';18='ConversionWait';19='EncoderProcessInput';20='EncoderSubmission'
 21='ConsumeTotal';22='ConsumeMutexWait';23='ConsumeMutexHold';24='FrameConstruction';25='TransportEvidence';26='QueueAdmission'
 30='WorkerQueueMutexWait';31='WorkerQueueMutexHold';32='WorkerConditionWait';33='LogicalFrameSend';34='SocketWaitWrite';35='SocketWriteCall';36='ChannelTxEvidence';37='AckReceive';38='SocketWaitRead';39='SocketReadCall';40='ChannelRxEvidence';41='SentMutexWait';42='SentMutexHold';43='AckMutexWait';44='AckMutexHold';45='AckProcess';46='SocketConfiguration';47='ChannelSend';48='ChannelReceive';49='SocketWriteResult';50='SocketReadResult';51='TransportFlush';52='ChannelFlush'
}
function Distribution([double[]]$Values){
 if(!$Values-or!$Values.Count){return [ordered]@{count=0;mean_ms=0;p50_ms=0;p95_ms=0;p99_ms=0;max_ms=0;total_ms=0}}
 [array]::Sort($Values);$sum=($Values|Measure-Object -Sum).Sum
 function Pick([double]$q){$index=[Math]::Max(0,[Math]::Min($Values.Count-1,[Math]::Ceiling($Values.Count*$q)-1));return $Values[$index]}
 return [ordered]@{count=$Values.Count;mean_ms=$sum/$Values.Count;p50_ms=(Pick .50);p95_ms=(Pick .95);p99_ms=(Pick .99);max_ms=$Values[-1];total_ms=$sum}
}
$stream=[IO.File]::OpenRead($binary);$reader=[IO.BinaryReader]::new($stream)
try{
 $magic=$reader.ReadUInt32();$version=$reader.ReadUInt32();$frequency=$reader.ReadUInt64();$started=$reader.ReadUInt64();$count=$reader.ReadUInt64();$dropped=$reader.ReadUInt64();$recordSize=$reader.ReadUInt32();$reduced=$reader.ReadUInt32()
 if($magic-ne0x31544d53-or$version-ne1-or$recordSize-ne48-or!$frequency){throw'Unsupported/corrupt micro-timing header'}
 if($stream.Length-ne48L+[int64]$count*48L){throw'Micro-timing length mismatch'}
 $durations=@{};$threads=@{};$sendStart=@{};$ackEnd=@{};$blocked=@{};$ackTicks=[Collections.Generic.List[uint64]]::new();$admissionTicks=[Collections.Generic.List[uint64]]::new();$frameIterations=[Collections.Generic.List[double]]::new();$idleIterations=[Collections.Generic.List[double]]::new();$socket=[ordered]@{send_buffer=0;receive_buffer=0;write_calls=0;write_requested_bytes=0;write_written_bytes=0;write_requested_max=0;write_written_max=0;partial_writes=0;partial_write_shortfall_bytes=0;write_retry_events=0;write_would_block=0;read_calls=0;read_requested_bytes=0;read_received_bytes=0;read_retry_events=0;read_would_block=0;write_wait_timeouts=0;read_wait_timeouts=0;queue_depth_peak=0}
 for([uint64]$i=0;$i-lt$count;$i++){
  $start=$reader.ReadUInt64();$duration=$reader.ReadUInt64();$frame=$reader.ReadUInt64();$session=$reader.ReadUInt64();$event=$reader.ReadUInt32();$thread=$reader.ReadUInt32();$value0=$reader.ReadUInt32();$value1=$reader.ReadUInt32()
  if(!$names.ContainsKey([int]$event)){throw("Unknown micro-timing event "+$event)}
  if(!$durations.ContainsKey($event)){$durations[$event]=[Collections.Generic.List[double]]::new();$threads[$event]=[Collections.Generic.HashSet[uint32]]::new()}
  $durations[$event].Add([double]$duration*1000.0/$frequency);$null=$threads[$event].Add($thread)
  if($event-eq33-and$frame){$sendStart[[string]$frame]=$start}
  if($event-eq45-and$frame){$ackEnd[[string]$frame]=$start+$duration;$ackTicks.Add($start+$duration)}
  if($event-eq2-and$frame){$admissionTicks.Add($start+$duration)}
  if($event-eq1){if($frame){$frameIterations.Add([double]$duration*1000.0/$frequency)}else{$idleIterations.Add([double]$duration*1000.0/$frequency)}}
  if($event-in@(4,22,30,41,43)-and$value0){$key=[string]$event;if(!$blocked.ContainsKey($key)){$blocked[$key]=[Collections.Generic.List[double]]::new()};$blocked[$key].Add([double]$duration*1000.0/$frequency)}
  if($event-eq46){$socket.send_buffer=$value0;$socket.receive_buffer=$value1}
  if($event-eq35){$socket.write_calls++;$socket.write_requested_bytes+=$value0;$socket.write_written_bytes+=$value1;$socket.write_requested_max=[Math]::Max($socket.write_requested_max,$value0);$socket.write_written_max=[Math]::Max($socket.write_written_max,$value1);if($value1-lt$value0){$socket.partial_writes++;$socket.partial_write_shortfall_bytes+=($value0-$value1)}}
  if($event-eq49){if($value0-gt1){$socket.write_retry_events++};$socket.write_would_block+=$value1}
  if($event-eq39){$socket.read_calls++;$socket.read_requested_bytes+=$value0;$socket.read_received_bytes+=$value1}
  if($event-eq50){if($value0-gt1){$socket.read_retry_events++};$socket.read_would_block+=$value1}
  if($event-eq34){$socket.write_wait_timeouts+=$value1}
  if($event-eq38){$socket.read_wait_timeouts+=$value1}
  if($event-eq26){$socket.queue_depth_peak=[Math]::Max($socket.queue_depth_peak,$value0)}
 }
}finally{$reader.Dispose();$stream.Dispose()}
$rows=[Collections.Generic.List[object]]::new();$stage=[ordered]@{}
foreach($event in @($durations.Keys|Sort-Object)){$distribution=Distribution $durations[$event].ToArray();$distribution['threads']=$threads[$event].Count;$stage[$names[[int]$event]]=$distribution;$rows.Add([pscustomobject]@{event=$event;name=$names[[int]$event];count=$distribution.count;threads=$distribution.threads;mean_ms=$distribution.mean_ms;p50_ms=$distribution.p50_ms;p95_ms=$distribution.p95_ms;p99_ms=$distribution.p99_ms;max_ms=$distribution.max_ms;total_ms=$distribution.total_ms})}
$sendToAck=[Collections.Generic.List[double]]::new();foreach($key in $sendStart.Keys){if($ackEnd.ContainsKey($key)-and$ackEnd[$key]-ge$sendStart[$key]){$sendToAck.Add([double]($ackEnd[$key]-$sendStart[$key])*1000.0/$frequency)}}
$ackTicks.Sort();$ackIntervals=[Collections.Generic.List[double]]::new();for($i=1;$i-lt$ackTicks.Count;$i++){$ackIntervals.Add([double]($ackTicks[$i]-$ackTicks[$i-1])*1000.0/$frequency)}
$admissionTicks.Sort();$admissionIntervals=[Collections.Generic.List[double]]::new();for($i=1;$i-lt$admissionTicks.Count;$i++){$admissionIntervals.Add([double]($admissionTicks[$i]-$admissionTicks[$i-1])*1000.0/$frequency)}
$blockedSummary=[ordered]@{};foreach($key in $blocked.Keys){$blockedSummary[$names[[int]$key]]=Distribution $blocked[$key].ToArray()}
$source=Get-Content -Raw -LiteralPath (Join-Path $hostDir 'session-1-result.json')|ConvertFrom-Json
$encode=Get-Content -Raw -LiteralPath (Join-Path $hostDir 'encode-result.json')|ConvertFrom-Json
$transport=Get-Content -Raw -LiteralPath (Join-Path $hostDir 'transport-result.json')|ConvertFrom-Json
$meta=Get-Content -Raw -LiteralPath $metaPath|ConvertFrom-Json
$summary=[ordered]@{version=1;run=$RunName;frequency=$frequency;records=$count;dropped=$dropped;reduced_evidence=[bool]$reduced;instrumentation=$meta;source_fps=$source.source_fps;host_fps=$source.fps;encoder_output_fps=$encode.output_fps;sender_fps=$transport.sender_fps;host_admission_interval=Distribution $admissionIntervals.ToArray();host_frame_iteration=Distribution $frameIterations.ToArray();host_idle_iteration=Distribution $idleIterations.ToArray();ack_interval=Distribution $ackIntervals.ToArray();send_to_ack=Distribution $sendToAck.ToArray();outstanding_ack_policy='one synchronous FRAME followed by its exact ACK';outstanding_ack_peak=$(if($sendStart.Count){1}else{0});socket=$socket;blocked_waits=$blockedSummary;stages=$stage}
$rows|Export-Csv -LiteralPath (Join-Path $run 'micro-timing-stages.csv') -NoTypeInformation -Encoding utf8
$summary|ConvertTo-Json -Depth 12|Set-Content -LiteralPath (Join-Path $run 'micro-timing-summary.json') -Encoding utf8
if($dropped-ne0){throw'Micro-timing capacity exhausted'}
Write-Output ('ANALYZED: '+$RunName+' records='+$count+' sendToAckP95ms='+('{0:F3}'-f$summary.send_to_ack.p95_ms))
