# One explicitly elevated stage. No deployment/security/phone operation.
param([Parameter(Mandatory=$true)][ValidateSet('Pilot','Observation','Soak')][string]$Stage,
 [Parameter(Mandatory=$true)][ValidatePattern('^[a-zA-Z0-9-]+$')][string]$RunPrefix)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$stageDir=Join-Path $repo ('docs/evidence/private/phase3a1/'+$RunPrefix+'-stage')
if(Test-Path $stageDir){throw 'Preserve stage evidence: choose a fresh RunPrefix'}
New-Item -ItemType Directory $stageDir|Out-Null
Start-Transcript -Path (Join-Path $stageDir 'transcript.txt')|Out-Null
$stageResult=@{Stage=$Stage;StartedUtc=[datetime]::UtcNow.ToString('o');Outcome='RUNNING';Error=$null}
try {
$cases=switch($Stage){
 'Pilot' {@(@{Name='cover';Seconds=20;Reconnect=$false},@{Name='snapshot';Seconds=20;Reconnect=$false})}
 'Observation' {@(@{Name='observe';Seconds=300;Reconnect=$true})}
 'Soak' {@(@{Name='observe';Seconds=1810;Reconnect=$true})}
}
foreach($case in $cases){
 $name=$RunPrefix+'-'+$case.Name
 $arguments=@('-NoProfile','-File',(Join-Path $PSScriptRoot 'Test-FirstFailDiagnostic.ps1'),'-RunName',$name,'-Seconds',$case.Seconds,'-ControlCase',$case.Name,'-Classified')
 if($case.Reconnect){$arguments+='-Reconnect'}
 & "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" @arguments
 if($LASTEXITCODE -ne 0){throw ('Stopped at classified failure: '+$name)}
 $dir=Join-Path $repo ('docs/evidence/private/phase3a1/'+$name)
 & (Join-Path $PSScriptRoot 'Verify-ClassifiedEvidence.ps1') -EvidenceDirectory $dir -MinimumSeconds $case.Seconds -RequireReconnect:$case.Reconnect
 if($Stage -eq 'Pilot'){
  $c=Get-Content (Join-Path $dir 'host/classification-result.json') -Raw|ConvertFrom-Json
  if(($case.Name -eq 'cover' -and !$c.B) -or ($case.Name -eq 'snapshot' -and !$c.C)){throw 'Required legitimate-content class not exercised'}
 }
}
$stageResult.Outcome='PASS'
}catch{$stageResult.Outcome='FAIL';$stageResult.Error=$_.Exception.Message;throw}
finally{$stageResult.CompletedUtc=[datetime]::UtcNow.ToString('o');$stageResult|ConvertTo-Json|Set-Content (Join-Path $stageDir 'stage-result.json') -Encoding utf8;Stop-Transcript|Out-Null}
