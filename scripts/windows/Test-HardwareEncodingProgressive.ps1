# Fixed PHASE 3B test sequence. No installation, security changes or next phase.
param([Parameter(Mandatory=$true)][ValidatePattern('^[a-zA-Z0-9-]+$')][string]$RunPrefix)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$root=Join-Path $repo 'docs/evidence/private/phase3b'
$dir=Join-Path $root ($RunPrefix+'-controller')
if(Test-Path $dir){throw 'Preserve evidence: choose a fresh RunPrefix'}
New-Item -ItemType Directory $dir|Out-Null
Start-Transcript -Path (Join-Path $dir 'transcript.txt')|Out-Null
$state=@{Outcome='RUNNING';StartedUtc=[datetime]::UtcNow.ToString('o');CompletedStages=@();Error=$null}
function Save{$state|ConvertTo-Json -Depth 6|Set-Content (Join-Path $dir 'controller-result.json') -Encoding utf8}
function Require($ok,$message){if(!$ok){throw $message}}
try{
 # The first actual low-resolution encode/decode gate must already have passed.
 $low=Get-Content (Join-Path $root 'encode-800-2/encoding-verification.json') -Raw|ConvertFrom-Json
 Require ($low.Outcome -eq 'PASS_ENCODE_DECODE' -and $low.Sessions[0].Encode.width -eq 800 -and $low.Sessions[0].Encode.outputs -eq 200) 'Missing verified 800x360 gate'
 $modes=@(
  @{Name='1280-30';Width=1280;Height=576;Rate=30;Bitrate=5000000},
  @{Name='1920-30';Width=1920;Height=864;Rate=30;Bitrate=10000000},
  @{Name='2400-30';Width=2400;Height=1080;Rate=30;Bitrate=15000000},
  @{Name='2400-60';Width=2400;Height=1080;Rate=60;Bitrate=25000000}
 )
 foreach($mode in $modes){
  $name=$RunPrefix+'-'+$mode.Name;$state.Current=$name;Save
  & "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -File (Join-Path $PSScriptRoot 'Test-HardwareEncoding.ps1') -RunName $name -Seconds 30 -Classified -Width $mode.Width -Height $mode.Height -Rate $mode.Rate -Bitrate $mode.Bitrate
  Require ($LASTEXITCODE -eq 0) ('Live stage failed: '+$name)
  $evidence=Join-Path $root $name;$hostDir=Join-Path $evidence 'host'
  & (Join-Path $repo 'out/phase3b/decoder/DecodeH264Evidence.exe') $hostDir $mode.Width $mode.Height 1> (Join-Path $evidence 'decode-stdout.txt') 2> (Join-Path $evidence 'decode-stderr.txt')
  Require ($LASTEXITCODE -eq 0) ('Independent decode failed: '+$name)
  & (Join-Path $PSScriptRoot 'Verify-HardwareEncoding.ps1') -EvidenceDirectory $evidence -MinimumSeconds 30 | Out-File (Join-Path $evidence 'verification-stdout.txt') -Encoding utf8
  $state.CompletedStages+=@{Name=$name;Outcome='PASS_ENCODE_DECODE'};Save
 }
 # Sustained test uses the same successful highest-mode binary/configuration.
 $name=$RunPrefix+'-sustained';$state.Current=$name;Save
 & "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -File (Join-Path $PSScriptRoot 'Test-HardwareEncoding.ps1') -RunName $name -Seconds 600 -Classified -Reconnect -Width 2400 -Height 1080 -Rate 60 -Bitrate 25000000
 Require ($LASTEXITCODE -eq 0) 'Sustained live test/reconnect failed'
 $evidence=Join-Path $root $name
 foreach($session in @('host','reconnect')){
  $sessionDir=Join-Path $evidence $session
  & (Join-Path $repo 'out/phase3b/decoder/DecodeH264Evidence.exe') $sessionDir 2400 1080 1> (Join-Path $sessionDir 'decode-stdout.txt') 2> (Join-Path $sessionDir 'decode-stderr.txt')
  Require ($LASTEXITCODE -eq 0) ('Independent sustained decode failed: '+$session)
 }
 & (Join-Path $PSScriptRoot 'Verify-HardwareEncoding.ps1') -EvidenceDirectory $evidence -MinimumSeconds 600 -RequireReconnect | Out-File (Join-Path $evidence 'verification-stdout.txt') -Encoding utf8
 $state.CompletedStages+=@{Name=$name;Outcome='PASS_ENCODE_DECODE'}
 $state.Outcome='TESTS_PASS_RESOURCE_REVIEW_REQUIRED'
}catch{$state.Outcome='STOPPED';$state.Error=$_.Exception.Message;throw}
finally{$state.CompletedUtc=[datetime]::UtcNow.ToString('o');Save;Stop-Transcript|Out-Null}
