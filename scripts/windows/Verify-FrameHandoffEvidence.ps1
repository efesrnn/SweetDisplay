param([Parameter(Mandatory=$true)][string]$EvidenceDirectory)
$ErrorActionPreference='Stop'
function Require([bool]$Condition,[string]$Message){if(!$Condition){throw $Message}}
$execution=Get-Content (Join-Path $EvidenceDirectory 'execution.json') -Raw | ConvertFrom-Json
Require ($execution.Outcome -eq 'PASS') 'Integration execution did not pass'
$pattern=@{}
Import-Csv (Join-Path $EvidenceDirectory 'pattern.csv') | ForEach-Object {$pattern[[uint32]$_.paint_tick]=[uint64]$_.qpc}
$reports=[Collections.Generic.List[object]]::new()
foreach($relative in @('continuous/session-1','continuous/session-2','slow/session-1','after-crash/session-1')){
 $report=Get-Content (Join-Path $EvidenceDirectory ($relative+'-result.json')) -Raw | ConvertFrom-Json
 $frames=@(Import-Csv (Join-Path $EvidenceDirectory ($relative+'-frames.csv')))
 Require ($frames.Count -eq $report.received -and $report.received -gt 1) "$relative frame count"
 Require ($report.width -eq 2400 -and $report.height -eq 1080 -and $report.format -eq 87) "$relative geometry"
 Require ($report.high_water -le 3 -and $report.held_end -eq 0 -and $report.clean_disconnect) "$relative bounded lifetime"
 Require ($report.acknowledged -eq $report.received -and $report.invalid -eq 0) "$relative acknowledgment/validation"
 Require ($report.nonce_matches -eq $report.received -and $report.sample_changes -gt 0) "$relative pattern content"
 $accounted=$report.received+$report.dropped_before_host+$report.dropped_host_queue+$report.busy+$report.invalid+$report.contention+$report.queue_depth_end+$report.held_end
 Require ($accounted -eq $report.source_frames) "$relative source/drop/outstanding accounting"
 [uint64]$lastId=0;[uint64]$lastQpc=0;[uint64]$lastReceive=0;[uint64]$gap=0;[uint64]$lastCounter=0
 $intervals=[Collections.Generic.List[double]]::new()
 foreach($frame in $frames){
  [uint64]$id=$frame.frame_id;[uint64]$qpc=$frame.source_qpc;[uint64]$receive=$frame.receive_qpc;[uint32]$counter=$frame.counter
  Require ($id -gt $lastId -and $qpc -gt $lastQpc -and $receive -gt $lastReceive -and $receive -ge $qpc) "$relative clock/ID monotonicity"
  Require ($frame.width -eq '2400' -and $frame.height -eq '1080' -and $frame.format -eq '87' -and $frame.flags -eq '0') "$relative per-frame metadata"
  Require ($pattern.ContainsKey($counter) -and $pattern[$counter] -le $qpc -and $counter -ge $lastCounter) "$relative sampled paint counter matches actual SWT0001 pattern log"
  if($lastId){$gap+=$id-$lastId-1;$intervals.Add(($qpc-$lastQpc)*1000.0/$report.frequency)}
  $lastId=$id;$lastQpc=$qpc;$lastReceive=$receive;$lastCounter=$counter
 }
 Require ($gap -eq $report.id_gaps) "$relative gap count"
 $measure=$intervals | Measure-Object -Average -Minimum -Maximum
 Require ([math]::Abs($measure.Average-$report.interval_avg_ms) -lt 0.00001) "$relative average interval"
 Require ([math]::Abs($measure.Minimum-$report.interval_min_ms) -lt 0.00001 -and [math]::Abs($measure.Maximum-$report.interval_max_ms) -lt 0.00001) "$relative interval range"
 if($relative -like 'continuous/*'){Require ($report.seconds -ge 30) "$relative minimum duration"}
 if($relative -like 'slow/*'){Require ($report.high_water -eq 3 -and $report.dropped_before_host+$report.dropped_host_queue -gt 0) 'Slow consumer did not exercise bounded overflow'}
 $reports.Add([pscustomobject]@{Run=$relative;Seconds=$report.seconds;Source=$report.source_frames;Received=$report.received;FPS=$report.fps;ProducerDrops=$report.dropped_before_host;HostQueueDrops=$report.dropped_host_queue;Busy=$report.busy;Contention=$report.contention;HighWater=$report.high_water;IDGaps=$report.id_gaps;ContentChanges=$report.sample_changes;Epoch=$report.epoch;FirstId=$report.first_id;LastId=$report.last_id})
}
$prior=$null
foreach($entry in $reports){if($prior){Require ($entry.Epoch -eq $prior.Epoch -and $entry.FirstId -gt $prior.LastId) 'Reconnect reset driver epoch/IDs'};$prior=$entry}
foreach($relative in @('absent-before/session-1-absent.json','absent-after-crash/session-1-absent.json')){
 $absent=Get-Content (Join-Path $EvidenceDirectory $relative) -Raw | ConvertFrom-Json
 Require ($absent.connected -eq 0 -and $absent.active -eq 1 -and $absent.source_delta -gt 0) "$relative no-Host progress"
}
$before=Get-Content (Join-Path $EvidenceDirectory 'before-health.json') -Raw | ConvertFrom-Json
$after=Get-Content (Join-Path $EvidenceDirectory 'after-health.json') -Raw | ConvertFrom-Json
Require ($before.Boot -eq $after.Boot -and $before.CiFlags -eq $after.CiFlags -and $after.SecureBoot -and $after.Hvci -eq 1) 'Security/boot state changed'
$summary=@{Outcome='PASS';Checks='Real frame metadata, QPC, pattern counter correlation, drop accounting, bounded capacity, reconnect and security';Runs=@($reports.ToArray())}
$summary | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $EvidenceDirectory 'verification.json') -Encoding utf8
$reports | Format-Table -AutoSize
