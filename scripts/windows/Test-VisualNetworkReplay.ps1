# Recorded-fixture integration only. Does not open the installed display interface.
param([Parameter(Mandatory=$true)][string]$RunName,[switch]$Reconnect,[switch]$StageDiagnostics,[switch]$AsyncDiagnostics)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot Read-VisualSamples.ps1)
if($RunName -notmatch '^[a-zA-Z0-9-]+$'){throw 'Run name'}
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$dir=Join-Path $repo ('docs/evidence/private/phase3d/'+$RunName)
if(Test-Path $dir){throw 'Preserve network fixture evidence'}
New-Item -ItemType Directory $dir|Out-Null
$variant=if($AsyncDiagnostics){'device-simulator-visual-async-v3'}elseif($StageDiagnostics){'device-simulator-visual-diag-v2'}else{'device-simulator-visual-v1'}
$exe=Join-Path $repo ('out/'+$variant+'/SweetDisplayDeviceSimulator.exe')
Copy-Item -LiteralPath $exe -Destination (Join-Path $dir 'SweetDisplayDeviceSimulator.exe')
$fixture=Join-Path $repo 'docs/evidence/private/phase3c/flow-content-3-normal/host/access-units.bin'
$oracle=@{};foreach($row in Import-Csv (Join-Path $repo 'docs/evidence/private/phase3c/flow-content-3-normal/host/decode/decoded-frames.csv')){$oracle[$row.pts]=$row}
$rx=$null;$tx=$null;$result=@{Outcome='RUNNING';LiveDisplay3=$false;Reconnect=[bool]$Reconnect;Receivers=@()}
function Require($ok,$why){if(!$ok){throw $why}}
function StartReceiver([int]$index){
 $script:rxDir=Join-Path $dir ('receiver-'+$index);New-Item -ItemType Directory $rxDir|Out-Null
 $script:rx=Start-Process $exe -ArgumentList ('--listen "'+$rxDir+'" 48235 40 1') -WindowStyle Normal -PassThru -RedirectStandardOutput (Join-Path $rxDir stdout.txt) -RedirectStandardError (Join-Path $rxDir stderr.txt);$null=$rx.Handle
}
try{
 StartReceiver 1;Start-Sleep -Seconds 1;$txDir=Join-Path $dir sender;New-Item -ItemType Directory $txDir|Out-Null
 $tx=Start-Process $exe -ArgumentList ('--send-fixture "'+$txDir+'" "'+$fixture+'" 48235 300') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $txDir stdout.txt) -RedirectStandardError (Join-Path $txDir stderr.txt);$null=$tx.Handle
 if($Reconnect){Start-Sleep -Seconds 4;Require (!$rx.HasExited -and !$tx.HasExited) 'Active fixture before close';Require ($rx.CloseMainWindow()) 'Receiver WM_CLOSE';Require ($rx.WaitForExit(5000) -and $rx.ExitCode -eq 0) 'Receiver clean close';$result.SourceActiveAtClose=(!$tx.HasExited);Start-Sleep -Milliseconds 700;StartReceiver 2}
 Require ($tx.WaitForExit(30000) -and $tx.ExitCode -eq 0) 'Fixture sender exit';Require ($rx.WaitForExit(10000) -and $rx.ExitCode -eq 0) 'Receiver final drain'
 $t=Get-Content (Join-Path $txDir transport-result.json) -Raw|ConvertFrom-Json
 Require ($t.seen -eq $t.admitted+$t.disconnected+$t.resync_skipped+$t.queue_overflow -and $t.admitted -eq $t.acked+$t.queue_aborted+$t.unconfirmed -and !$t.pending -and !$t.protocol_errors) 'Fixture exact transport accounting'
 $sessions=@{}
 foreach($folder in Get-ChildItem $dir -Directory|Where-Object Name -like 'receiver-*'){
  $r=Get-Content (Join-Path $folder.FullName visual-result.json) -Raw|ConvertFrom-Json
  $gpu=Get-Content (Join-Path $folder.FullName gpu-path.json) -Raw|ConvertFrom-Json
  $received=@(Import-Csv (Join-Path $folder.FullName received.csv));$rows=@(Read-VisualSamples $folder.FullName);$first=$received[0]
  Require (([int]$first.flags -band 7) -eq 7 -and !$sessions.ContainsKey($first.session)) 'New session IDR/SPS/PPS';$sessions[$first.session]=$true
  Require ($r.received -eq $received.Count -and $r.received -eq $r.admitted+$r.queue_overflow+$r.resync_skips -and $r.admitted -eq $r.queue_taken+$r.queue_reset_drops+$r.queue_pending -and $r.queue_taken -eq $r.submitted -and $r.submitted -eq $r.decoded+$r.decoder_reset_drops -and $r.decoded -eq $rows.Count -and $r.decoded -eq $r.presented+$r.render_drops) 'Decode/render exact accounting'
  Require ($r.encoded_queue_peak -le 3 -and $r.decoder_pending_peak -le 16 -and $r.render_queue_bound -eq 1 -and $gpu.dxva_surface_proofs -eq $r.decoded -and !$r.software_fallback) 'GPU and boundedness'
  Require ($r.render_drops -eq $gpu.minimized+$gpu.occluded+$gpu.present_busy+$gpu.stale_generation -and $gpu.render_submissions -eq $r.decoded-$gpu.minimized) 'Separate render drop counts'
  $sample=@($rows|Where-Object {$_.diagnostic -eq '1' -and $_.presented -eq '1'});Require ($sample.Count -ge 2) 'Visible fixture samples'
  foreach($row in $sample){$o=$oracle[$row.pts];Require ($row.session -eq $first.session -and $o -and $row.nonce -eq $o.nonce -and $row.counter -eq $o.counter -and $row.ambiguous -eq '0') 'Rendered fixture oracle'}
  $result.Receivers+=@{Name=$folder.Name;Counts=$r;Gpu=$gpu;DiagnosticMatches=$sample.Count;Session=$first.session}
 }
 Require ($result.Receivers[-1].Counts.drained) 'Final decoder drain'
 if($Reconnect){Require ($sessions.Count -eq 2 -and $t.disconnected -gt 0 -and $t.resync_skipped -gt 0 -and $result.Receivers[0].Counts.closed_by_window) 'Real fixture socket reconnect'}
 $result.Outcome='PASS_NETWORK_REPLAY';$result.Transport=$t
}catch{$result.Outcome='ERROR';$result.Error=$_.Exception.Message;throw}
finally{foreach($p in @($tx,$rx)){if($p -and !$p.HasExited){$p.Kill();$p.WaitForExit();$result.ForcedFailureCleanup=$true}};$result|ConvertTo-Json -Depth 10|Set-Content (Join-Path $dir 'test-result.json')}
$result|ConvertTo-Json -Depth 10
