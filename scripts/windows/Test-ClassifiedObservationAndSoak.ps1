# Fixed, bounded two-stage test in one UAC session. No general command broker.
param([Parameter(Mandatory=$true)][ValidatePattern('^[a-zA-Z0-9-]+$')][string]$RunPrefix)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$dir=Join-Path $repo ('docs/evidence/private/phase3a1/'+$RunPrefix+'-controller')
if(Test-Path $dir){throw 'Preserve evidence: use a new RunPrefix'}
New-Item -ItemType Directory $dir|Out-Null
Start-Transcript -Path (Join-Path $dir 'transcript.txt')|Out-Null
$state=@{Outcome='OBSERVATION_RUNNING';StartedUtc=[datetime]::UtcNow.ToString('o');Error=$null}
function Save{$state|ConvertTo-Json -Depth 4|Set-Content (Join-Path $dir 'controller-result.json') -Encoding utf8}
try {
 Save
 & "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -File (Join-Path $PSScriptRoot 'Test-ClassifiedPhase3.ps1') -Stage Observation -RunPrefix ($RunPrefix+'-observation')
 if($LASTEXITCODE -ne 0){throw 'Observation stage failed; soak not started'}
 $report=Join-Path $repo ('docs/evidence/private/phase3a1/'+$RunPrefix+'-observation-observe/classified-verification.json')
 if(!(Test-Path $report)){throw 'Missing verified observation evidence'}
 $state.ObservationSha256=(Get-FileHash $report).Hash
 $state.Outcome='WAITING_RESOURCE_REVIEW';Save
 # The calling agent inspects resources before continuing the already-authorized
 # soak. This is an evidence-review gate, not a new security permission request.
 $gate=Join-Path $dir 'observation-review.json';$deadline=[datetime]::UtcNow.AddMinutes(5)
 while(!(Test-Path $gate)){
  if([datetime]::UtcNow -ge $deadline){throw 'Bounded evidence-review wait expired; soak not started'}
  Start-Sleep -Milliseconds 250
 }
 $review=Get-Content $gate -Raw|ConvertFrom-Json
 if($review.Decision -ne 'RUN_SOAK' -or $review.EvidenceSha256 -ne $state.ObservationSha256 -or (Get-FileHash $report).Hash -ne $state.ObservationSha256){throw 'Observation review did not authorize this evidence for soak'}
 $state.Outcome='SOAK_RUNNING';Save
 & "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -File (Join-Path $PSScriptRoot 'Test-ClassifiedPhase3.ps1') -Stage Soak -RunPrefix ($RunPrefix+'-soak')
 if($LASTEXITCODE -ne 0){throw 'Soak stage failed; stop before PHASE 3B'}
 $state.Outcome='TESTS_PASS_RESOURCE_REVIEW_REQUIRED'
}catch{$state.Outcome='STOPPED';$state.Error=$_.Exception.Message;throw}
finally{$state.CompletedUtc=[datetime]::UtcNow.ToString('o');Save;Stop-Transcript|Out-Null}
