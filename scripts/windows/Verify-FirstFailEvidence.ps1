# Verifies diagnostic evidence, never converts an oracle failure into soak PASS.
param([Parameter(Mandatory=$true)][string]$EvidenceDirectory,[long]$QpcFrequency=0)
$ErrorActionPreference='Stop'
$dir=(Resolve-Path -LiteralPath $EvidenceDirectory).Path
function ReadJson($name){[IO.File]::ReadAllText((Join-Path $dir $name))|ConvertFrom-Json}
function Require([bool]$ok,[string]$why){if(!$ok){throw $why}}
$execution=ReadJson 'execution.json'
Require ([bool]$execution.CompletedUtc) 'Incomplete diagnostic run'
Require (!$execution.CleanupCheckError) 'Post-test health/cleanup check failed'
Require ($execution.Outcome -in @('CONTENT_ORACLE_FAILURE','NO_ANOMALY_REPRODUCED')) 'Unexpected execution failure'
Require ($execution.FailureEventCount -eq 0) 'Driver/system failure events'
$before=ReadJson 'before-health.json'
foreach($p in Get-ChildItem $dir -Filter '*-health.json'){
 $h=[IO.File]::ReadAllText($p.FullName)|ConvertFrom-Json
 Require ($h.SecureBoot -and $h.Hvci -eq 1 -and $h.CiStatus -eq 0 -and $h.CiFlags -eq $before.CiFlags -and ($h.CiFlags -band 2) -eq 0 -and $h.Boot -eq $before.Boot) 'Security/boot baseline'
 Require (@($h.Devices).Count -eq 2 -and @($h.Devices|Where-Object {$_.Problem -ne 0 -or $_.Status -ne 'OK'}).Count -eq 0) 'PnP health'
}
if(Test-Path (Join-Path $dir 'pattern-result.json')){$pattern=ReadJson 'pattern-result.json';$frequency=[double]$pattern.frequency}
else{Require ($execution.PatternForcedStop -and $QpcFrequency -gt 0) 'Missing producer clock/result; forensic override requires measured QPC frequency';$frequency=[double]$QpcFrequency}
$frames=@(Import-Csv (Join-Path $dir 'host/session-1-frames.csv'))
Require ($frames.Count -gt 0) 'No received frames'
$nonce=[Convert]::ToUInt32($execution.Nonce,16)
$ledger=@{};foreach($row in Import-Csv (Join-Path $dir 'pattern.csv')){$ledger[[uint32]$row.paint_tick]=[uint64]$row.qpc}
$resources=[IO.File]::ReadAllText((Join-Path $dir 'host/resource-generation.txt'))
$slots=@{};foreach($m in [regex]::Matches($resources,'slot=(\d+) shared_resource=(\S+) host_texture=(\S+)')){$slots[[int]$m.Groups[1].Value]=$m.Groups[3].Value}
Require ($slots.Count -eq 3) 'Resource slot inventory'
$previousId=[uint64]0;$previousQpc=[uint64]0;$previousCounter=[uint32]0;$bad=@();$epoch=$frames[0].epoch
foreach($f in $frames){
 Require ([uint64]$f.frame_id -gt $previousId -and [uint64]$f.source_qpc -gt $previousQpc) 'Frame metadata order'
 Require ([uint64]$f.source_qpc -le [uint64]$f.receive_qpc -and [uint64]$f.receive_qpc -le [uint64]$f.resource_acquired_qpc -and [uint64]$f.resource_acquired_qpc -le [uint64]$f.sample_complete_qpc) 'Per-frame timestamp order'
 Require ($f.epoch -eq $epoch -and $f.host_texture -eq $slots[[int]$f.slot]) 'Generation/resource association'
 Require ($f.width -eq 2400 -and $f.height -eq 1080 -and $f.format -eq 87 -and $f.flags -eq 0) 'Geometry/flags'
 $counter=[uint32]$f.counter
 if([uint32]$f.nonce -ne $nonce -or !$counter -or $counter -lt $previousCounter -or !$ledger.ContainsKey($counter) -or $ledger[$counter] -gt [uint64]$f.source_qpc){$bad+=,$f}else{$previousCounter=$counter}
 $previousId=[uint64]$f.frame_id;$previousQpc=[uint64]$f.source_qpc
}
$summary=[ordered]@{EvidenceValidation='VERIFIED';Acceptance=$execution.Outcome;CleanPatternShutdown=(!$execution.PatternForcedStop);Frames=$frames.Count;FirstToLastSeconds=([uint64]$frames[-1].source_qpc-[uint64]$frames[0].source_qpc)/$frequency;BadRows=$bad.Count;HistoricalCause='UNKNOWN';PipelineBugProven=$false}
$images=@(Get-ChildItem $dir -Recurse -Filter *.bmp)
if($execution.Outcome -eq 'CONTENT_ORACLE_FAILURE'){
 Require ($bad.Count -eq 1 -and $bad[0].frame_id -eq $frames[-1].frame_id) 'Not stopped at first invalid content'
 $failure=ReadJson 'host/first-failure.json'
 Require ($failure.frame_id -eq [uint64]$frames[-1].frame_id -and $failure.source_acquisition_id -eq $failure.frame_id -and $failure.source_qpc -eq [uint64]$frames[-1].source_qpc) 'First-fail/frame association'
 Require ($images.Count -eq 1 -and $failure.capture_hresult -eq 0 -and $failure.image_error -eq 0 -and $failure.same_held_texture_sample_matches_crop) 'Exactly one valid matching crop required'
 $history=@(Import-Csv (Join-Path $dir 'host/first-failure-history.csv'))
 Require ($history.Count -ge 1 -and $history.Count -le 120 -and $history[-1].frame_id -eq $failure.frame_id -and @($history|Where-Object reason -ne 0).Count -eq 1) 'Bounded first-fail history'
 if(!('SweetDisplayCropCheck' -as [type])){Add-Type -TypeDefinition @'
using System;
public static class SweetDisplayCropCheck {
 public static ulong Hash(byte[] p,int offset,int width) {unchecked {ulong h=1469598103934665603UL;foreach(int row in new int[]{24,72}) for(int x=0;x<2560;x++){h^=p[offset+(row*width+8)*4+x];h*=1099511628211UL;}return h;}}
 public static uint Decode(byte[] p,int offset,int width,int row){uint n=0;for(int b=0;b<32;b++)if(p[offset+(row*width+16+b*20)*4]>200)n|=1U<<b;return n;}
 public static bool Grey(byte[] p,int offset){for(int i=offset;i<p.Length;i+=4)if(p[i]!=48||p[i+1]!=48||p[i+2]!=48)return false;return true;}
}
'@}
 $pixels=[IO.File]::ReadAllBytes($images[0].FullName)
 Require ([BitConverter]::ToUInt16($pixels,0) -eq 0x4d42 -and [BitConverter]::ToInt32($pixels,18) -eq 656 -and [BitConverter]::ToInt32($pixels,22) -eq -96 -and [BitConverter]::ToUInt16($pixels,28) -eq 32) 'Crop BMP geometry'
 $offset=[BitConverter]::ToInt32($pixels,10)
 Require ($offset -eq 54 -and $pixels.Length -eq 54+656*96*4) 'Crop storage bound'
 $hash=[SweetDisplayCropCheck]::Hash($pixels,$offset,656)
 $observedNonce=[SweetDisplayCropCheck]::Decode($pixels,$offset,656,24)
 $observedCounter=[SweetDisplayCropCheck]::Decode($pixels,$offset,656,72)
 Require ($hash -eq [uint64]$failure.sample_hash -and $hash -eq [uint64]$failure.crop_sample_hash -and $observedNonce -eq $failure.observed_nonce -and $observedCounter -eq $failure.observed_counter) 'Independent crop decoding/hash mismatch'
 $summary.FirstMismatchSeconds=($failure.source_qpc-[uint64]$frames[0].source_qpc)/$frequency
 $summary.Failure=$failure
 $summary.CropUniformGrey48=[SweetDisplayCropCheck]::Grey($pixels,$offset)
 $events=@(Import-Csv (Join-Path $dir 'desktop-events.csv'))
 $control=@($events|Where-Object event -eq 'control_begin')
 if($control.Count){$summary.ControlToAcquisitionMs=($failure.source_qpc-[uint64]$control[0].qpc)*1000/$frequency}
 $summary.Classification='UNEXPLAINED_CONTENT_FAILURE'
 if($execution.ControlCase -eq 'cover' -and $summary.CropUniformGrey48 -and $control.Count -eq 1 -and $summary.ControlToAcquisitionMs -ge 0){$summary.Classification='CONTROLLED_COMPOSITOR_REPLACEMENT'}
 if($execution.ControlCase -eq 'snapshot'){
  $snapshot=@($events|Where-Object event -eq 'controlled_snapshot_counter')
  if($snapshot.Count -eq 1 -and $snapshot[0].value -eq $observedCounter -and $observedNonce -eq $nonce -and $observedCounter -lt $failure.previous_counter -and $summary.ControlToAcquisitionMs -ge 0){$summary.Classification='CONTROLLED_OLD_CONTENT_ON_DESKTOP'}
 }
}else{
 Require (!$execution.PatternForcedStop) 'Completed run must shut down cleanly'
 Require ($bad.Count -eq 0 -and $images.Count -eq 0) 'Unexpected content or image in completed run'
 $r=ReadJson 'host/session-1-result.json'
 Require ($r.received -eq $frames.Count) 'Frame/result count'
 Require ($r.seconds -ge $execution.RequestedSeconds -and $r.clean_disconnect -and $r.acknowledged -eq $r.received -and $r.held_end -eq 0 -and $r.high_water -le 3 -and $r.invalid -eq 0) 'Bounded duration/lifetime'
 $accounted=$r.received+$r.dropped_before_host+$r.dropped_host_queue+$r.busy+$r.invalid+$r.contention+$r.queue_depth_end+$r.held_end
 Require ($accounted -eq $r.source_frames) 'Source/drop accounting'
 foreach($t in Import-Csv (Join-Path $dir 'host/session-1-telemetry.csv')){
  Require ([int]$t.ready+[int]$t.held -le 3 -and [int]$t.high_water -le 3 -and $t.connected -eq '1' -and $t.epoch -eq $epoch) 'Queue/connection telemetry'
 }
 $summary.Result=$r
 $summary.Classification='NO_ANOMALY_REPRODUCED_NOT_A_FIX'
}
$summary|ConvertTo-Json -Depth 8|Set-Content (Join-Path $dir 'diagnostic-verification.json') -Encoding utf8
$summary|ConvertTo-Json -Depth 8
