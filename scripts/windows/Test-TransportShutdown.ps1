param([Parameter(Mandatory=$true)][string]$RunName)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'TransportShutdown.ps1')
if($RunName -notmatch '^[a-zA-Z0-9-]+$'){throw 'Run name'}
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$dir=Join-Path $repo ('docs/evidence/private/phase3c/'+$RunName)
if(Test-Path $dir){throw 'Preserve existing test evidence'}
New-Item -ItemType Directory $dir|Out-Null
$results=@()
foreach($case in @('receiver-first','both-exited','receiver-error','missing-result','not-drained','protocol-error','missing-ack','host-hang','host-error')){
 $fixture=Join-Path $dir $case;New-Item -ItemType Directory $fixture|Out-Null
 $r=@{drained=$true;frames=2;protocol_errors=0}
 if($case -eq 'not-drained'){$r.drained=$false};if($case -eq 'protocol-error'){$r.protocol_errors=1}
 if($case -ne 'missing-result'){$r|ConvertTo-Json|Set-Content (Join-Path $fixture 'receiver-result.json')}
 $tail=@('RX,5,123,5,100,8,0,0,0,0','TX,5,123,5,101,8,0,0,0,0')
 if($case -eq 'missing-ack'){$tail[1]='TX,7,123,5,101,8,0,0,0,0'}
 $tail|Set-Content (Join-Path $fixture 'protocol-messages.csv')
 $ms=if($case -eq 'host-hang'){8000}elseif($case -eq 'both-exited'){0}else{600}
 $exit=if($case -eq 'host-error'){7}else{0}
 $args='-NoProfile -Command "Start-Sleep -Milliseconds '+$ms+'; exit '+$exit+'"'
 $child=Start-Process -FilePath (Join-Path $env:WINDIR 'System32/WindowsPowerShell/v1.0/powershell.exe') -ArgumentList $args -WindowStyle Hidden -PassThru
 $null=$child.Handle;$accepted=$false;$message='';$hostErrorRejected=$false
 try{
  if($case -eq 'both-exited'){$child.WaitForExit()}
  $receiverCode=if($case -eq 'receiver-error'){1}else{0}
  $grace=if($case -eq 'host-hang'){100}else{5000}
  try{
   $proof=Wait-TransportReceiverDrain -HostProcess $child -ReceiverExitCode $receiverCode -ReceiverDirectory $fixture -GraceMilliseconds $grace
   # Production separately queries native Host exit code; never equate exit with success.
   if($child.ExitCode -ne 0){$hostErrorRejected=$true;throw 'Host nonzero exit remains a failure'}
   $accepted=$proof.ReceiverDrained -and $proof.HostExited
  }catch{$message=$_.Exception.Message}
  $expected=$case -in @('receiver-first','both-exited')
  if($accepted -ne $expected -or ($case -eq 'host-error' -and !$hostErrorRejected)){throw ('Shutdown policy regression: '+$case)}
  $results+=@{Case=$case;Outcome='PASS';Accepted=$accepted;Reason=$message}
 }finally{if(!$child.HasExited){$child.Kill();$child.WaitForExit()}}
}
$results|ConvertTo-Json -Depth 4|Set-Content (Join-Path $dir 'shutdown-tests.json')
Write-Output ('PASS '+$results.Count+' process shutdown cases')
