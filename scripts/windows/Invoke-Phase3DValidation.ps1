# PHASE 3D validation adapter derived from the protected PHASE 3C checker; original unchanged.
# Test-only controller: no deployment, trust, boot/security or phone operations.
param([Parameter(Mandatory=$true)][string]$RunName,[switch]$AsyncDiagnostics)
$ErrorActionPreference='Stop'
[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
if($RunName -notmatch '^[a-zA-Z0-9-]+$'){throw 'Run name'}
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$root=Join-Path $repo ('docs/evidence/private/phase3d/'+$RunName)
$normal=$RunName+'-normal';$recovery=$RunName+'-reconnect'
foreach($name in @($RunName,$normal,$recovery)){if(Test-Path (Join-Path $repo ('docs/evidence/private/phase3d/'+$name))){throw 'Preserve all existing evidence'}}
New-Item -ItemType Directory $root|Out-Null
$result=@{Outcome='RUNNING';StartedUtc=[datetime]::UtcNow.ToString('o');Runs=@();Current='preflight';Error=$null}
function Save{$result|ConvertTo-Json -Depth 8|Set-Content (Join-Path $root controller.json)}
function Decode([string]$path){
 $destination=Join-Path $path decode;if(Test-Path $destination){throw 'Existing decode evidence'};New-Item -ItemType Directory $destination|Out-Null
 $args='"'+$path+'" 2400 1080 "'+$destination+'"'
 $p=Start-Process (Join-Path $repo 'out/phase3b/decoder/DecodeH264Evidence.exe') -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $destination stdout.txt) -RedirectStandardError (Join-Path $destination stderr.txt)
 $null=$p.Handle;if(!$p.WaitForExit(120000)){$p.Kill();throw 'Offline decoder timeout'};if($p.ExitCode -ne 0){throw ('Decode failed: '+[IO.File]::ReadAllText((Join-Path $destination stderr.txt)))}
}
try{
 Save
 foreach($name in @($normal,$recovery)){
  $reconnect=$name -eq $recovery;$seconds=if($reconnect){45}else{35};$dir=Join-Path $repo ('docs/evidence/private/phase3d/'+$name)
  $result.Current=$name+' live';Save
  # A distinct PowerShell process isolates Add-Type native definitions between runs.
  $args='-NoProfile -File "'+(Join-Path $PSScriptRoot Test-VisualTransport.ps1)+'" -RunName '+$name+' -Seconds '+$seconds+' -Port 48231'
  if($reconnect){$args+=' -DisconnectReceiver'}
  if($AsyncDiagnostics){$args+=' -AsyncDiagnostics'}
  $p=Start-Process (Join-Path $env:WINDIR 'System32/WindowsPowerShell/v1.0/powershell.exe') -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root ($name+'-stdout.txt')) -RedirectStandardError (Join-Path $root ($name+'-stderr.txt'))
  $null=$p.Handle;if(!$p.WaitForExit(180000)){throw 'Live controller timeout; inspect child state before any further test'}
  if($p.ExitCode -ne 0){throw ('Live test failed: '+[IO.File]::ReadAllText((Join-Path $root ($name+'-stderr.txt'))))}
  $result.Current=$name+' independent decode';Save
  Decode (Join-Path $dir host)
  foreach($rx in Get-ChildItem -LiteralPath $dir -Directory | Where-Object Name -match '^receiver-\d+$'){if(Test-Path (Join-Path $rx.FullName access-units.bin)){Decode $rx.FullName}}
  & (Join-Path $PSScriptRoot Verify-VisualEncoding.ps1) -EvidenceDirectory $dir -MinimumSeconds $seconds -DecodeSubdirectory decode | Set-Content (Join-Path $root ($name+'-encoding.txt'))
  & (Join-Path $PSScriptRoot Verify-VisualTransport.ps1) -EvidenceDirectory $dir -Reconnect:$reconnect | Set-Content (Join-Path $root ($name+'-transport.txt'))
  & (Join-Path $PSScriptRoot Verify-VisualRendering.ps1) -EvidenceDirectory $dir -Reconnect:$reconnect | Set-Content (Join-Path $root ($name+'-rendering.txt'))
  if($AsyncDiagnostics){
   foreach($rx in Get-ChildItem -LiteralPath $dir -Directory | Where-Object Name -match '^receiver-\d+$'){
    & (Join-Path $PSScriptRoot Review-VisualStages.ps1) -EvidenceDirectory $rx.FullName -OracleCsv (Join-Path $dir 'host/decode/decoded-frames.csv') | Set-Content (Join-Path $root ($name+'-'+$rx.Name+'-timing.txt'))
   }
  }
  $result.Runs+=@{Name=$name;Outcome='PASS_ENCODE_DECODE_TRANSPORT_RENDER_ARTIFACTS';Reconnect=$reconnect};Save
 }
 $result.Outcome='PASS_ARTIFACTS';$result.Current='AWAIT_INDEPENDENT_REVIEW_AND_OWNER_VISUAL_OBSERVATION';$result.Phase3D='PARTIAL'
}catch{$result.Outcome='ERROR';$result.Error=$_.Exception.Message;$result.Position=$_.InvocationInfo.PositionMessage}
finally{$result.CompletedUtc=[datetime]::UtcNow.ToString('o');Save}
if($result.Outcome -ne 'PASS_ARTIFACTS'){throw $result.Error}


