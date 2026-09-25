param([Parameter(Mandatory=$true)][string]$EvidenceDirectory,[int]$MinimumSeconds=0,[switch]$RequireReconnect,[string]$ReportPath)
$ErrorActionPreference='Stop'
$dir=(Resolve-Path -LiteralPath $EvidenceDirectory).Path
if($dir -notmatch '[\\/]phase3c[\\/]'){throw 'PHASE3C evidence required'}
function Json($p){[IO.File]::ReadAllText($p)|ConvertFrom-Json}
function Require([bool]$ok,[string]$message){if(!$ok){throw $message}}
$e=Json (Join-Path $dir 'execution.json')
Require ($e.Outcome -eq 'PASS_TRANSPORT_SOURCE' -and $e.Classified -and $e.CompletedUtc -and $e.HostExitCode -eq 0 -and $e.PatternExitCode -eq 0 -and !$e.PatternForcedStop -and !$e.HostForcedStop -and !$e.CleanupCheckError) 'Classified execution/clean shutdown did not pass'
Require ($e.FailureEventCount -eq 0 -and @(Get-ChildItem $dir -Recurse -Filter *.bmp).Count -eq 0) 'Failure event or unexpected image in passed run'
$before=Json (Join-Path $dir 'before-health.json')
foreach($file in Get-ChildItem $dir -Filter '*-health.json'){
 $h=Json $file.FullName
 Require ($h.SecureBoot -and $h.Hvci -eq 1 -and $h.CiStatus -eq 0 -and ($h.CiFlags -band 1) -eq 1 -and ($h.CiFlags -band 2) -eq 0 -and ($h.CiFlags -band 0x400) -ne 0 -and $h.CiFlags -eq $before.CiFlags -and $h.Boot -eq $before.Boot) 'Security/boot state'
 Require (@($h.Devices).Count -eq 2 -and @($h.Devices|Where-Object {$_.Problem -ne 0 -or $_.Status -ne 'OK'}).Count -eq 0) 'PnP state'
}
$nonce=[Convert]::ToUInt32($e.Nonce,16)
$ledger=@{};foreach($row in Import-Csv (Join-Path $dir 'pattern.csv')){$ledger[[uint32]$row.paint_tick]=[uint64]$row.qpc}
$reports=@();$prior=$null
$sessions=@('host');if($RequireReconnect){Require ($e.ReconnectExitCode -eq 0) 'Fresh-process reconnect missing';$sessions+='reconnect'}
foreach($session in $sessions){
 $p=Join-Path $dir $session;$r=Json (Join-Path $p 'session-1-result.json');$c=Json (Join-Path $p 'classification-result.json')
 Require ($r.seconds -ge $(if($session -eq 'host'){[Math]::Max($MinimumSeconds,$e.RequestedSeconds)}else{15}) -and $r.received -ge 2 -and $r.clean_disconnect -and $r.held_end -eq 0 -and $r.invalid -eq 0 -and $r.acknowledged -eq $r.received -and $r.high_water -le 3) 'Duration/queue/ack/clean disconnect'
 Require ($r.source_frames -eq $r.received+$r.dropped_before_host+$r.dropped_host_queue+$r.busy+$r.invalid+$r.contention+$r.queue_depth_end+$r.held_end) 'Exact drop accounting'
 Require ($c.transport_content_policy -and $c.D -eq 0 -and $c.E -eq $c.transport_counter_only -and $c.A+$c.B+$c.C+$c.E -eq $r.received) 'Classification totals'
 $frames=@(Import-Csv (Join-Path $p 'session-1-frames.csv'));$classes=@(Import-Csv (Join-Path $p 'classifications.csv'))
 Require ($frames.Count -eq $r.received -and $classes.Count -eq $frames.Count) 'Classification/frame association count'
 $identity=[IO.File]::ReadAllText((Join-Path $p 'resource-generation.txt'));$slots=@{}
 foreach($match in [regex]::Matches($identity,'slot=(\d+) shared_resource=(\S+) host_texture=(\S+)')){$slots[[int]$match.Groups[1].Value]=$match.Groups[3].Value}
 Require ($slots.Count -eq 3) 'Resource inventory'
 [uint64]$lastId=0;[uint64]$lastQpc=0;[uint64]$lastPresent=0;[uint32]$highCounter=0;[uint64]$gaps=0
 $counts=@{1=0;2=0;3=0;5=0};$transitions=0;$lastClass=0
 for($i=0;$i -lt $frames.Count;$i++){
  $f=$frames[$i];$cl=$classes[$i];[uint64]$id=$f.frame_id;[uint64]$qpc=$f.source_qpc;$k=[int]$cl.class
  Require ($id -gt $lastId -and $qpc -gt $lastQpc -and [uint64]$f.presentation_id -gt $lastPresent -and $qpc -le [uint64]$f.receive_qpc -and [uint64]$f.receive_qpc -le [uint64]$f.resource_acquired_qpc -and [uint64]$f.resource_acquired_qpc -le [uint64]$f.sample_complete_qpc) 'Metadata/QPC ordering'
  Require ($f.epoch -eq $r.epoch -and $f.width -eq 2400 -and $f.height -eq 1080 -and $f.format -eq 87 -and $f.flags -eq 0 -and $f.host_texture -eq $slots[[int]$f.slot]) 'Geometry/slot/generation'
  Require ($cl.frame_id -eq $f.frame_id -and $cl.source_qpc -eq $f.source_qpc -and $cl.nonce -eq $f.nonce -and $cl.counter -eq $f.counter -and [uint32]$cl.previous_counter -eq $highCounter -and $k -in @(1,2,3,5)) 'Classification/frame association'
  if($k -in @(1,3,5)){$counter=[uint32]$f.counter;Require ([uint32]$f.nonce -eq $nonce -and $counter -gt 0 -and $ledger.ContainsKey($counter) -and $ledger[$counter] -eq [uint64]$cl.render_qpc -and $ledger[$counter] -le $qpc -and $counter -le [uint32]$cl.published) 'Pattern render ledger';if($k -eq 1){Require ($counter -ge $highCounter) 'Uncorroborated regression'}else{Require ($counter -lt $highCounter) 'Content regression required';if($k -eq 5){Require ([int]$cl.reason -eq 4) 'Only counter-only E may continue'}};$highCounter=[Math]::Max($highCounter,$counter)}
  if($k -eq 2 -or $k -eq 3){Require ($cl.reference_match -eq '1' -and $cl.reference_error -eq '0' -and $cl.rgb_hash -eq $cl.reference_hash1 -and $cl.rgb_hash -eq $cl.reference_hash2 -and [uint64]$cl.reference_begin_qpc -ge [uint64]$f.sample_complete_qpc -and [uint64]$cl.reference_end_qpc -ge [uint64]$cl.reference_begin_qpc) 'Independent desktop corroboration'}
  $counts[$k]++;if($lastClass -and $lastClass -ne $k){$transitions++};$lastClass=$k
  $gap=if($lastId){$id-$lastId-1}else{0};Require ($gap -eq [uint64]$f.id_gap) 'Gap field';$gaps+=$gap;$lastId=$id;$lastQpc=$qpc;$lastPresent=[uint64]$f.presentation_id
 }
 Require ($counts[1] -eq $c.A -and $counts[2] -eq $c.B -and $counts[3] -eq $c.C -and $counts[5] -eq $c.E -and $gaps -eq $r.id_gaps) 'Summary/class/drop counts'
 foreach($t in Import-Csv (Join-Path $p 'session-1-telemetry.csv')){Require ([int]$t.ready+[int]$t.held -le 3 -and [int]$t.high_water -le 3 -and $t.connected -eq '1' -and $t.epoch -eq $r.epoch) 'Queue telemetry'}
 if($prior){Require ($r.epoch -eq $prior.epoch -and $r.first_id -gt $prior.last_id) 'Fresh reconnect identity/order'};$prior=$r
 $reports+=@{Session=$session;Host=$r;Classes=$c;ClassTransitions=$transitions}
}
$profiles=@();$resources=@(Import-Csv (Join-Path $dir 'resources.csv'))
foreach($group in $resources|Group-Object Role){
 $warm=@($group.Group|Where-Object {[double]$_.ElapsedSeconds -ge 120});if($warm.Count -lt 3){$warm=@($group.Group)}
 $profiles+=@{Role=$group.Name;Samples=$warm.Count;FirstElapsed=$warm[0].ElapsedSeconds;LastElapsed=$warm[-1].ElapsedSeconds;PrivateDelta=[long]$warm[-1].PrivateBytes-[long]$warm[0].PrivateBytes;HandlesDelta=[long]$warm[-1].Handles-[long]$warm[0].Handles;PrivateMin=($warm|ForEach-Object {[long]$_.PrivateBytes}|Measure-Object -Minimum).Minimum;PrivateMax=($warm|ForEach-Object {[long]$_.PrivateBytes}|Measure-Object -Maximum).Maximum;HandlesMin=($warm|ForEach-Object {[long]$_.Handles}|Measure-Object -Minimum).Minimum;HandlesMax=($warm|ForEach-Object {[long]$_.Handles}|Measure-Object -Maximum).Maximum}
}
$summary=@{Outcome='PASS_TRANSPORT_SOURCE';HistoricalSoak='UNKNOWN';Runs=$reports;Resources=$profiles;ResourceTrendRequiresReview=$true;Scope='Transport scope: counter-only E remains observable/UNKNOWN, requires independent AU decode/content proof; all resource and metadata invariants remain enforced'}
if(!$ReportPath){$ReportPath=Join-Path $dir 'classified-verification.json'}
if(Test-Path -LiteralPath $ReportPath){throw 'Preserve existing handoff report'}
$summary|ConvertTo-Json -Depth 9|Set-Content $ReportPath -Encoding utf8
$summary|ConvertTo-Json -Depth 9


