param([Parameter(Mandatory=$true)][string]$RunName)
$ErrorActionPreference='Stop'
if($RunName -notmatch '^[a-zA-Z0-9-]+$'){throw 'Run name'}
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$root=Join-Path $repo ('docs/evidence/private/phase3c/'+$RunName)
if(Test-Path $root){throw 'Preserve existing suite'}
New-Item -ItemType Directory $root|Out-Null
& (Join-Path $repo 'out/protocol-tests-content-v2/ProtocolTests.exe') *> (Join-Path $root 'protocol-tests.txt')
if($LASTEXITCODE){throw 'Deterministic tests failed'}
foreach($mode in @('normal','slow')){
 $dir=Join-Path $root $mode;$hostDir=Join-Path $dir 'host';$rxDir=Join-Path $dir 'receiver-1'
 New-Item -ItemType Directory $hostDir,$rxDir|Out-Null
 $port=if($mode -eq 'normal'){48233}else{48234};$delay=if($mode -eq 'normal'){0}else{80}
 $receiver=$null;$fixture=$null
 try{
  $receiver=Start-Process (Join-Path $repo 'out/device-simulator-content-v2/SweetDisplayDeviceSimulator.exe') -ArgumentList ('"'+$rxDir+'" '+$port+' 45 '+$delay+' 0') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $rxDir stdout.txt) -RedirectStandardError (Join-Path $rxDir stderr.txt)
  $null=$receiver.Handle;Start-Sleep -Milliseconds 300
  $args='"'+$hostDir+'" '+$port;if($mode -eq 'slow'){$args+=' slow-tail'}
  $fixture=Start-Process (Join-Path $repo 'out/transport-fixture-content-v2/TransportFixture.exe') -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $hostDir stdout.txt) -RedirectStandardError (Join-Path $hostDir stderr.txt)
  $null=$fixture.Handle
  if(!$fixture.WaitForExit(30000) -or $fixture.ExitCode -ne 0){throw 'Fixture failed'}
  if(!$receiver.WaitForExit(10000) -or $receiver.ExitCode -ne 0){throw 'Receiver failed'}
  & (Join-Path $PSScriptRoot 'Verify-LocalTransport.ps1') -EvidenceDirectory $dir -Synthetic | Set-Content (Join-Path $dir 'verification-output.txt')
 }finally{foreach($p in @($fixture,$receiver)){if($p -and !$p.HasExited){$p.Kill();$p.WaitForExit()}}}
}
Write-Output ('PASS deterministic + normal + slow transport suite: '+$RunName)
