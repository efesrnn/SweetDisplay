# Offline verifier for PHASE 3B only. Never rewrite PHASE 3A evidence.
param([Parameter(Mandatory=$true)][string]$EvidenceDirectory,[int]$MinimumSeconds=20,[switch]$RequireReconnect,[string]$DecodeSubdirectory='')
$ErrorActionPreference='Stop'
$dir=(Resolve-Path -LiteralPath $EvidenceDirectory).Path
if($dir -notmatch '[\\/]phase3b[\\/]'){throw 'PHASE 3B evidence directory required'}
function Json($p){[IO.File]::ReadAllText($p)|ConvertFrom-Json}
function Require([bool]$condition,[string]$message){if(!$condition){throw $message}}
# Existing independent shared-texture/classification checks stay in force.
& (Join-Path $PSScriptRoot 'Verify-ClassifiedEvidence.ps1') -EvidenceDirectory $dir -MinimumSeconds $MinimumSeconds -RequireReconnect:$RequireReconnect -ReportPath (Join-Path $dir 'handoff-verification.json') | Out-Null
$sessions=@('host');if($RequireReconnect){$sessions+='reconnect'}
$summary=@()
foreach($session in $sessions){
 $p=Join-Path $dir $session
 $decodePath=if($DecodeSubdirectory){Join-Path $p $DecodeSubdirectory}else{$p}
 $h=Json (Join-Path $p 'session-1-result.json');$e=Json (Join-Path $p 'encode-result.json');$d=Json (Join-Path $decodePath 'decode-result.json')
 Require ($e.hardware_only -and $e.gpu_surface_input -and !$e.software_fallback -and $e.drained -and $e.clean_shutdown) 'Hardware/GPU/shutdown flags'
 Require ($e.input -eq $h.received -and $e.input -eq $e.accepted+$e.rate_drops+$e.backpressure_drops -and $e.outputs -eq $e.accepted -and !$e.pending -and !$e.invalid -and $e.queue_peak -le 4 -and $e.pool_peak -le 4 -and $e.outputs -ge 2) 'Exact encoder queue/drop accounting'
 Require ($d.outcome -eq 'PASS_DECODE' -and $d.decoded -eq $e.outputs -and $d.inputs -eq $e.outputs -and $d.width -eq $e.width -and $d.height -eq $e.height -and $d.bytes -eq $e.bytes) 'Independent decode count/geometry'
 Require ((Get-Item (Join-Path $p 'stream.h264')).Length -eq $e.bytes -and (Get-Item (Join-Path $p 'access-units.bin')).Length -eq $e.bytes+12*$e.outputs) 'Bitstream/AU byte accounting'
 $calls=[IO.File]::ReadAllText((Join-Path $p 'encoder-calls.txt'))
 foreach($marker in @('MFTEnum2 hardware only=0x00000000','Activate hardware MFT=0x00000000','async=1 d3d11_aware=1','MFT SET_D3D_MANAGER=0x00000000','SetOutput H264=0x00000000','SetInput NV12=0x00000000','encoder Shutdown=0x00000000','software_fallback=0')){Require ($calls.Contains($marker)) ('Missing hardware evidence: '+$marker)}
 Require ($calls -match ('input_surface_proof format=103 width='+$e.width+' height='+$e.height+' usage=0 cpu_access=0 current_length=\d+')) 'Actual NV12 GPU surface proof'
 $inputs=@(Import-Csv (Join-Path $p 'encode-input.csv'));$frames=@(Import-Csv (Join-Path $p 'session-1-frames.csv'));$outputs=@(Import-Csv (Join-Path $p 'encode-output.csv'));$decoded=@(Import-Csv (Join-Path $decodePath 'decoded-frames.csv'))
 Require ($inputs.Count -eq $e.input -and $frames.Count -eq $inputs.Count -and $outputs.Count -eq $e.outputs -and $decoded.Count -eq $outputs.Count) 'Evidence record totals'
 $accepted=@{};$rate=0;$pressure=0;[long]$lastPts=-1
 for($i=0;$i -lt $inputs.Count;$i++){
  $x=$inputs[$i];$f=$frames[$i]
  Require ($x.frame_id -eq $f.frame_id -and $x.source_qpc -eq $f.source_qpc -and $x.nonce -eq $f.nonce -and $x.counter -eq $f.counter) 'Encoder/source frame association'
  Require ([int]$x.pending -le 4 -and [int]$x.pool_busy -le 4) 'Per-frame queue bound'
  switch($x.decision){
   'accepted' {Require ([long]$x.pts -gt $lastPts -and !$accepted.ContainsKey($x.pts) -and [long]$x.submit_qpc -ge [long]$x.source_qpc -and [double]$x.conversion_ms -ge 0) 'Accepted timestamp/order';$lastPts=[long]$x.pts;$accepted[$x.pts]=$x}
   'rate' {$rate++}
   'backpressure' {$pressure++}
   default {throw 'Unknown input decision'}
  }
 }
 Require ($accepted.Count -eq $e.accepted -and $rate -eq $e.rate_drops -and $pressure -eq $e.backpressure_drops) 'Decision category totals'
 [long]$totalBytes=0;$idr=0;$key=0;[long]$lastOutput=-1
 for($i=0;$i -lt $outputs.Count;$i++){
  $o=$outputs[$i];$v=$decoded[$i]
  Require ($accepted.ContainsKey($o.pts)) 'Output without accepted input'
  $x=$accepted[$o.pts]
  Require ([long]$o.pts -gt $lastOutput -and $o.frame_id -eq $x.frame_id -and $o.source_qpc -eq $x.source_qpc -and $o.submit_qpc -eq $x.submit_qpc -and [long]$o.output_qpc -ge [long]$o.submit_qpc) 'Output identity/order'
  Require ($v.pts -eq $o.pts -and [long]$v.index -eq $i -and $v.width -eq $e.width -and $v.height -eq $e.height -and $v.nonce -eq $x.nonce -and $v.counter -eq $x.counter -and $v.ambiguous -eq '0') 'Decoded/source content correspondence'
  Require ([int]$v.aperture_x -ge 0 -and [int]$v.aperture_y -ge 0 -and [int]$v.aperture_x+[int]$v.width -le [int]$v.coded_width -and [int]$v.aperture_y+[int]$v.height -le [int]$v.coded_height) 'Decoded visible aperture bounds'
  $totalBytes+=[long]$o.bytes;$idr+=[int]$o.idr;$key+=[int]($o.clean_point -ne '0');$lastOutput=[long]$o.pts;$accepted.Remove($o.pts)
 }
 Require (!$accepted.Count -and $totalBytes -eq $e.bytes -and $idr -eq $e.idr -and $key -eq $e.keyframes -and $idr -gt 0) 'Final output bytes/IDR/accounting'
 $summary+=@{Session=$session;Outcome='PASS_ENCODE_DECODE';Encode=$e;Decode=$d;ContentScope='All decoded frames matched source nonce/counter sampled regions; not exhaustive whole-frame pixel equivalence'}
}
$report=@{Outcome='PASS_ENCODE_DECODE';Sessions=$summary;ResourceTrendRequiresReview=$true;Sustained60Fps='NOT YET TESTED';Phase3AEvidence='UNCHANGED'}
$report|ConvertTo-Json -Depth 8|Set-Content (Join-Path $dir 'encoding-verification.json') -Encoding utf8
$report|ConvertTo-Json -Depth 8
