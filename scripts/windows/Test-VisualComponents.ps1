# Non-admin component tests. Recorded video is never labelled live Display 3.
param([Parameter(Mandatory=$true)][string]$RunName,[switch]$StageDiagnostics,[switch]$AsyncDiagnostics)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot Read-VisualSamples.ps1)
if($RunName -notmatch '^[a-zA-Z0-9-]+$'){throw 'Run name'}
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$dir=Join-Path $repo ('docs/evidence/private/phase3d/'+$RunName)
if(Test-Path $dir){throw 'Preserve component evidence'}
New-Item -ItemType Directory $dir|Out-Null
$variant=if($AsyncDiagnostics){'device-simulator-visual-async-v3'}elseif($StageDiagnostics){'device-simulator-visual-diag-v2'}else{'device-simulator-visual-v1'}
$exe=Join-Path $repo ('out/'+$variant+'/SweetDisplayDeviceSimulator.exe')
$fixture=Join-Path $repo 'docs/evidence/private/phase3c/flow-content-3-normal/host/access-units.bin'
$oracle=@{};foreach($row in Import-Csv (Join-Path $repo 'docs/evidence/private/phase3c/flow-content-3-normal/host/decode/decoded-frames.csv')){$oracle[$row.pts]=$row}
$result=@{Outcome='RUNNING';Tests=@();LiveDisplay3=$false}
function Save{$result|ConvertTo-Json -Depth 8|Set-Content (Join-Path $dir 'tests.json')}
function Require($ok,$why){if(!$ok){throw $why}}
function ValidateRender($path){
 $r=Get-Content (Join-Path $path 'visual-result.json') -Raw|ConvertFrom-Json
 Require ($r.received -eq $r.admitted+$r.queue_overflow+$r.resync_skips -and $r.admitted -eq $r.queue_taken+$r.queue_reset_drops+$r.queue_pending -and $r.queue_taken -eq $r.submitted -and $r.submitted -eq $r.decoded+$r.decoder_reset_drops -and $r.decoded -eq $r.presented+$r.render_drops) 'Exact component accounting'
 Require ($r.encoded_queue_peak -le 3 -and $r.decoder_pending_peak -le 16 -and $r.render_queue_bound -eq 1 -and !$r.decode_failures) 'Bounded components'
 $rows=@(Read-VisualSamples $path);Require ($rows.Count -eq $r.decoded) 'Decode ledger count'
 $samples=@($rows|Where-Object {$_.diagnostic -eq '1' -and $_.presented -eq '1'});Require ($samples.Count -ge 2) 'Visible diagnostic samples'
 foreach($row in $samples){$o=$oracle[$row.pts];Require ($o -and $row.nonce -eq $o.nonce -and $row.counter -eq $o.counter -and $row.ambiguous -eq '0') 'Sparse rendered content correspondence'}
 $known=@{};foreach($group in $rows|Group-Object session){Require (!$known.ContainsKey($group.Name)) 'Unique session';$known[$group.Name]=$true;[long]$pts=-1;foreach($row in $group.Group){Require ([long]$row.pts -gt $pts) 'PTS order';$pts=[long]$row.pts}}
 return @{Counts=$r;PresentedDiagnosticMatches=$samples.Count;Sessions=$known.Count}
}
try{
 Save;Copy-Item -LiteralPath $exe -Destination (Join-Path $dir 'SweetDisplayDeviceSimulator.exe')
 Get-FileHash $exe,(Join-Path $repo 'windows/simulator/VisualPipeline.h'),(Join-Path $repo 'windows/simulator/SweetDisplayDeviceSimulator.cpp')|ConvertTo-Json|Set-Content (Join-Path $dir 'build-sha256.json')
 & $exe --self-test *> (Join-Path $dir 'queue-tests.txt');Require ($LASTEXITCODE -eq 0) 'Queue/session/malformed NAL tests'
 foreach($name in @('replay','close','corrupt')){
  $path=Join-Path $dir $name;New-Item -ItemType Directory $path|Out-Null;$inputFile=$fixture;$count=100;$close=0
  if($name -eq 'close'){$close=1}
  if($name -eq 'corrupt'){
   $inputFile=Join-Path $path 'corrupt-au.bin';$writer=[IO.BinaryWriter]::new([IO.File]::Create($inputFile))
   try{$au=[byte[]](0,0,0,1,0x67,0,0,0,1,0x68,0,0,0,1,0x65);$writer.Write([uint32]$au.Length);$writer.Write([uint64]0);$writer.Write($au)}finally{$writer.Dispose()};$count=1
  }
  $args='--replay "'+$path+'" "'+$inputFile+'" '+$count+' '+$close
  $p=Start-Process $exe -ArgumentList $args -WindowStyle Normal -PassThru -RedirectStandardOutput (Join-Path $path stdout.txt) -RedirectStandardError (Join-Path $path stderr.txt);$null=$p.Handle
  if(!$p.WaitForExit(30000)){throw 'Component timeout; inspect process before retry'}
  if($name -eq 'corrupt'){Require ($p.ExitCode -ne 0 -and ([IO.File]::ReadAllText((Join-Path $path stderr.txt))).Contains('decode exact drain count')) 'Malformed SPS/PPS rejected at exact output drain';$result.Tests+=@{Name=$name;Outcome='PASS_EXPECTED_REJECTION';Exit=$p.ExitCode}}
  else{$review=ValidateRender $path;Require ($p.ExitCode -eq 0) 'Component exit';if($name -eq 'replay'){Require ($review.Counts.decoded -eq 200 -and $review.Sessions -eq 2 -and $review.Counts.resize_count -ge 2) 'Two fresh GPU sessions and resize'}else{Require ($review.Counts.closed_by_window) 'WM_CLOSE path'};$result.Tests+=@{Name=$name;Outcome='PASS';Review=$review}}
  Save
 }
 $result.Outcome='PASS_COMPONENTS';$result.LiveAcceptance=$false
}catch{$result.Outcome='ERROR';$result.Error=$_.Exception.Message;throw}finally{Save}
$result|ConvertTo-Json -Depth 8
