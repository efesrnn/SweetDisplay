# Diagnostic recorded-stream reproduction, NOT live Display 3 acceptance.
param([Parameter(Mandatory=$true)][ValidatePattern('^[a-zA-Z0-9-]+$')][string]$RunName,
 [ValidateRange(120,1700)][int]$Count=1700,[ValidateRange(10,100)][int]$IntervalMs=17,[switch]$AsyncDiagnostics)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$dir=Join-Path $repo ('docs/evidence/private/phase3d/'+$RunName)
if(Test-Path $dir){throw 'Preserve diagnostic evidence'}
New-Item -ItemType Directory $dir|Out-Null
$variant=if($AsyncDiagnostics){'device-simulator-visual-async-v3'}else{'device-simulator-visual-diag-v2'}
$exe=Join-Path $repo ('out/'+$variant+'/SweetDisplayDeviceSimulator.exe')
$fixture=Join-Path $repo 'docs/evidence/private/phase3c/flow-content-3-normal/host/access-units.bin'
Copy-Item $exe $dir
Get-FileHash $exe,$fixture -Algorithm SHA256|ConvertTo-Json|Set-Content (Join-Path $dir inputs.json)
$rxDir=Join-Path $dir receiver-1;$txDir=Join-Path $dir sender
New-Item -ItemType Directory $rxDir,$txDir|Out-Null
$rx=$null;$tx=$null;$result=@{Outcome='RUNNING';LiveDisplay3=$false;Count=$Count;IntervalMs=$IntervalMs}
try{
 & $exe --self-test *> (Join-Path $dir self-test.txt);if($LASTEXITCODE){throw 'Component self test'}
 $rx=Start-Process $exe -ArgumentList ('--listen "'+$rxDir+'" 48236 100 1') -WindowStyle Normal -PassThru -RedirectStandardOutput (Join-Path $rxDir stdout.txt) -RedirectStandardError (Join-Path $rxDir stderr.txt);$null=$rx.Handle
 Start-Sleep -Seconds 1
 $tx=Start-Process $exe -ArgumentList ('--send-fixture "'+$txDir+'" "'+$fixture+'" 48236 '+$Count+' '+$IntervalMs) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $txDir stdout.txt) -RedirectStandardError (Join-Path $txDir stderr.txt);$null=$tx.Handle
 if(!$tx.WaitForExit(100000) -or $tx.ExitCode -ne 0){throw 'Recorded sender failure'}
 if(!$rx.WaitForExit(10000) -or $rx.ExitCode -ne 0){throw 'Diagnostic receiver failure'}
 $result.Outcome='MEASUREMENTS_READY';$result.Receiver=Get-Content (Join-Path $rxDir visual-result.json) -Raw|ConvertFrom-Json
 $result.Transport=Get-Content (Join-Path $txDir transport-result.json) -Raw|ConvertFrom-Json
}catch{$result.Outcome='ERROR';$result.Error=$_.Exception.Message;throw}
finally{foreach($p in @($tx,$rx)){if($p -and !$p.HasExited){$p.Kill();$p.WaitForExit();$result.ForcedFailureCleanup=$true}};$result|ConvertTo-Json -Depth 8|Set-Content (Join-Path $dir test-result.json)}
$result|ConvertTo-Json -Depth 8
