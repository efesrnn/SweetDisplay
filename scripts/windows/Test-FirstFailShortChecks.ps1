# Run only after separately reviewing controlled failure evidence.
# One elevation before tests; no additional UAC/desktop switch during observation.
param([Parameter(Mandatory=$true)][ValidatePattern('^[a-zA-Z0-9-]+$')][string]$RunPrefix)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
foreach($test in @(@{Name='foreground';Seconds=30},@{Name='observe';Seconds=300})){
 $name=$RunPrefix+'-'+$test.Name
 & "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -File (Join-Path $PSScriptRoot 'Test-FirstFailDiagnostic.ps1') -RunName $name -Seconds $test.Seconds -ControlCase $test.Name
 if($LASTEXITCODE -ne 0){throw ('Stopped at first failed diagnostic: '+$name)}
 $dir=Join-Path $repo ('docs/evidence/private/phase3a1/'+$name)
 & (Join-Path $PSScriptRoot 'Verify-FirstFailEvidence.ps1') -EvidenceDirectory $dir
 if($test.Name -eq 'foreground'){
  $control=@(Import-Csv (Join-Path $dir 'desktop-events.csv')|Where-Object event -eq 'controlled_foreground_result')
  if($control.Count -ne 1 -or $control[0].value -ne '1'){throw 'Foreground control did not activate successfully'}
 }
}
