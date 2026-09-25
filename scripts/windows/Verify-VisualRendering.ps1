# Closed-evidence verifier. No live operations and no changes to historical runs.
param([Parameter(Mandatory=$true)][string]$EvidenceDirectory,[switch]$Reconnect,[string]$ReviewDirectory)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot Read-VisualSamples.ps1)
$dir=(Resolve-Path -LiteralPath $EvidenceDirectory).Path
if($dir -notmatch '[\\/]docs[\\/]evidence[\\/]private[\\/]phase3d[\\/]'){throw 'Private PHASE3D evidence required'}
function Require($ok,$why){if(!$ok){throw $why}}
function Json($p){Get-Content -LiteralPath $p -Raw|ConvertFrom-Json}
function Distribution($values){$v=@($values|Sort-Object);Require ($v.Count -gt 0) 'Empty latency sample';return @{P50=$v[[Math]::Ceiling(.5*$v.Count)-1];P95=$v[[Math]::Ceiling(.95*$v.Count)-1];P99=$v[[Math]::Ceiling(.99*$v.Count)-1];Max=$v[-1]}}
$review=if($ReviewDirectory){(Resolve-Path -LiteralPath $ReviewDirectory).Path}else{$dir}
if($ReviewDirectory -and ($review -eq $dir -or $review -notmatch '[\\/]docs[\\/]evidence[\\/]private[\\/]phase3d[\\/]')){throw 'Separate private PHASE3D review required'}
$report=Join-Path $review 'render-verification.json';if(Test-Path $report){throw 'Preserve existing rendering review'}
$execution=Json (Join-Path $dir 'execution.json')
Require ($execution.Outcome -eq 'PASS_TRANSPORT_SOURCE' -and !$execution.ReceiverForcedStop -and $execution.ReceiverExitCode -eq 0) 'Controller/receiver clean completion'
$encoding=Json (Join-Path $review 'encoding-verification.json');$transport=Json (Join-Path $review 'transport-verification.json')
Require ($transport.Outcome -eq 'PASS_TRANSPORT' -or ($ReviewDirectory -and $transport.Outcome -eq 'PASS_TRANSPORT_ARTIFACTS_ONLY')) 'Full independent transport verification prerequisite'
$hostResult=Json (Join-Path $dir 'host/session-1-result.json')
$oraclePath=if($ReviewDirectory){Join-Path $review 'host-decode/decoded-frames.csv'}else{Join-Path $dir 'host/decode/decoded-frames.csv'}
$oracle=@{};foreach($row in Import-Csv $oraclePath){$oracle[$row.pts]=$row}
$receivers=@(Get-ChildItem -LiteralPath $dir -Directory|Where-Object Name -match '^receiver-\d+$'|Sort-Object Name)
Require ($receivers.Count -eq $(if($Reconnect){2}else{1})) 'Receiver count'
$summary=@();$sessions=@{}
foreach($receiver in $receivers){
 $p=$receiver.FullName;$r=Json (Join-Path $p 'visual-result.json');$gpu=Json (Join-Path $p 'gpu-path.json')
 $rows=@(Read-VisualSamples $p);$received=@(Import-Csv (Join-Path $p 'received.csv'))
 Require (!$r.replay -and $r.received -eq $received.Count -and $r.received -eq $r.admitted+$r.queue_overflow+$r.resync_skips) 'Real receive and exact admission accounting'
 Require ($r.admitted -eq $r.queue_taken+$r.queue_reset_drops+$r.queue_pending -and !$r.queue_pending -and $r.submitted -eq $r.queue_taken) 'Exact encoded queue accounting'
 Require ($r.submitted -eq $r.decoded+$r.decoder_reset_drops -and $r.decoded -eq $rows.Count -and $r.decoded -eq $r.presented+$r.render_drops) 'Exact decode/presentation accounting'
 Require ($r.encoded_queue_peak -le 3 -and $r.decoder_pending_peak -le 16 -and $r.render_queue_bound -eq 1 -and !$r.decode_failures) 'Decode/render bounds'
 Require ($r.hardware_surface_validation -and !$r.software_fallback -and $gpu.hardware_device -and $gpu.d3d11_aware -and $gpu.manager_never_detached -and $gpu.dxva_surface_proofs -eq $r.decoded -and $gpu.decoder_clsid -eq '62CE7E72-4C71-4D20-B15D-452831A87D9D') 'Actual hardware decoder surface proof'
 Require ($gpu.presented -eq $r.presented -and $r.render_drops -eq $gpu.minimized+$gpu.occluded+$gpu.present_busy+$gpu.stale_generation) 'Separate render drop accounting'
 $first=$received[0];Require (([int]$first.flags -band 7) -eq 7 -and !$sessions.ContainsKey($first.session)) 'New session starts at IDR/SPS/PPS'
 $sessions[$first.session]=$true;$map=@{};foreach($row in $received){$map[$row.sequence]=$row}
 [long]$pts=-1;[uint64]$frameId=0;[uint64]$presentTime=0;$checks=0;$changing=@{}
 foreach($row in $rows){
  $source=$map[$row.sequence]
  Require ($source -and $row.session -eq $first.session -and $row.session -eq $source.session -and $row.frame_id -eq $source.frame_id -and $row.source_ns -eq $source.source_ns -and $row.pts -eq $source.pts -and $row.receive_ns -eq $source.receive_ns) 'Rendered source/session/timestamp identity'
  Require ([long]$row.pts -gt $pts -and [uint64]$row.frame_id -gt $frameId -and [uint64]$row.decoded_ns -ge [uint64]$row.receive_ns -and [uint64]$row.present_ns -ge [uint64]$row.decoded_ns) 'Output ordering/clock consistency'
  $pts=[long]$row.pts;$frameId=[uint64]$row.frame_id
  if($row.presented -eq '1'){Require ([uint64]$row.present_ns -gt $presentTime) 'Presentation order';$presentTime=[uint64]$row.present_ns}
  if($row.diagnostic -eq '1' -and $row.presented -eq '1'){
   $expected=$oracle[$row.pts];Require ($expected -and $row.nonce -eq $expected.nonce -and $row.counter -eq $expected.counter -and $row.ambiguous -eq '0') 'Rendered backbuffer nonce/counter differs from independently decoded source'
   $checks++;$changing[$row.nonce+':'+$row.counter]=$true
  }
 }
 $presented=@($rows|Where-Object presented -eq '1');Require ($presented.Count -gt 1 -and $checks -ge 10 -and $changing.Count -ge 10) 'Changing visible-frame diagnostic evidence'
 $span=([double]$presented[-1].present_ns-[double]$presented[0].present_ns)/1e9
 if(!$Reconnect){Require ($hostResult.seconds -ge 30 -and $span -ge 30) 'At least 30 seconds of live presentation'}
 $lat=@($presented|ForEach-Object {([double]$_.present_ns-[double]$_.receive_ns)/1e6});$latency=Distribution $lat
 # Explicit initial validation guard, not a performance claim: receipt to CPU
 # Present acceptance must remain under 500ms. This does not measure photons.
 Require ($latency.Max -le 500) 'Unbounded/over-budget receive-to-present latency'
 $bins=@($presented|Group-Object { [int][Math]::Floor(([double]$_.receive_ns-[double]$presented[0].receive_ns)/1e10)}|ForEach-Object {@{Bin=$_.Name;Count=$_.Count;LatencyMs=(Distribution @($_.Group|ForEach-Object {([double]$_.present_ns-[double]$_.receive_ns)/1e6}))}})
 $mem=@(Import-Csv (Join-Path $p 'resources.csv'));Require ($mem.Count -ge 3) 'Resource observations missing'
 $steady=@($mem|Where-Object {[double]$_.time_ns-[double]$mem[0].time_ns -ge 3e9})
 Require ($steady.Count -ge 2) 'Insufficient post-startup resource observations'
 $privateGrowth=[long]$steady[-1].private_bytes-[long]$steady[0].private_bytes;$handleGrowth=[int]$steady[-1].handles-[int]$steady[0].handles
 Require ($privateGrowth -le 33554432 -and $handleGrowth -le 16) 'Short-run resource growth requires investigation'
 $cpu=([double]$steady[-1].cpu_100ns-[double]$steady[0].cpu_100ns)*100/(([double]$steady[-1].time_ns-[double]$steady[0].time_ns)/100)
 $gaps=@();for($i=1;$i -lt $presented.Count;$i++){$gaps+=([double]$presented[$i].present_ns-[double]$presented[$i-1].present_ns)/1e6}
 $sparse=if(Test-Path (Join-Path $p sparse-result.json)){Json (Join-Path $p sparse-result.json)}else{$null}
 if($receiver -eq $receivers[-1]){Require ($r.drained -and !$r.closed_by_window) 'Final receiver drain/shutdown'}else{Require ($Reconnect -and $r.closed_by_window -and !$r.drained) 'Controlled WM_CLOSE old receiver'}
 $summary+=@{Receiver=$receiver.Name;Session=$first.session;Counts=$r;Gpu=$gpu;PresentationSpanSeconds=$span;ReceivedFpsOverHostDuration=$r.received/$hostResult.seconds;DecodedFpsOverHostDuration=$r.decoded/$hostResult.seconds;PresentedFpsOverHostDuration=$r.presented/$hostResult.seconds;ActivePresentCadence=($presented.Count-1)/$span;DiagnosticMatches=$checks;UniqueDiagnosticContents=$changing.Count;ReceiveToPresentAcceptanceMs=$latency;TenSecondBins=$bins;PrivateGrowthBytes=$privateGrowth;HandleGrowth=$handleGrowth;CpuPercentOneCore=$cpu;LeakFreeLongTermClaim=$false}
 $summary[-1].InterPresentGapMs=Distribution $gaps;$summary[-1].SparseDiagnostics=$sparse
}
$output=@{Outcome='PASS_RENDER_ARTIFACTS';Reconnect=[bool]$Reconnect;SourceFps=$hostResult.source_frames/$hostResult.seconds;HostFps=$hostResult.received/$hostResult.seconds;HostDuration=$hostResult.seconds;Receivers=$summary;OwnerVisibleObservationStillRequired=$true;Scope='GPU decoder-bound NV12 surfaces, successful Present calls and sparse rendered-backbuffer correspondence; not scan-out/photons or 60-FPS proof'}
$output|ConvertTo-Json -Depth 12|Set-Content -LiteralPath $report
$output|ConvertTo-Json -Depth 12
