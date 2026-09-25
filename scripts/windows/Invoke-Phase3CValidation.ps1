# Test-only controller: no deployment, trust, boot/security or phone operations.
param([Parameter(Mandatory=$true)][string]$RunName)
$ErrorActionPreference='Stop'
[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
if($RunName -notmatch '^[a-zA-Z0-9-]+$'){throw 'Run name'}
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$root=Join-Path $repo ('docs/evidence/private/phase3c/'+$RunName)
$normal=$RunName+'-normal';$recovery=$RunName+'-reconnect'
foreach($name in @($RunName,$normal,$recovery)){if(Test-Path (Join-Path $repo ('docs/evidence/private/phase3c/'+$name))){throw 'Preserve all existing evidence'}}
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
  $reconnect=$name -eq $recovery;$seconds=if($reconnect){45}else{35};$dir=Join-Path $repo ('docs/evidence/private/phase3c/'+$name)
  $result.Current=$name+' live';Save
  # A distinct PowerShell process isolates Add-Type native definitions between runs.
  $args='-NoProfile -File "'+(Join-Path $PSScriptRoot Test-LocalTransport.ps1)+'" -RunName '+$name+' -Seconds '+$seconds+' -Port 48231'
  if($reconnect){$args+=' -DisconnectReceiver'}
  $p=Start-Process (Join-Path $PSHOME powershell.exe) -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root ($name+'-stdout.txt')) -RedirectStandardError (Join-Path $root ($name+'-stderr.txt'))
  $null=$p.Handle;if(!$p.WaitForExit(180000)){throw 'Live controller timeout; inspect child state before any further test'}
  if($p.ExitCode -ne 0){throw ('Live test failed: '+[IO.File]::ReadAllText((Join-Path $root ($name+'-stderr.txt'))))}
  $result.Current=$name+' independent decode';Save
  Decode (Join-Path $dir host)
  foreach($rx in Get-ChildItem -LiteralPath $dir -Directory | Where-Object Name -match '^receiver-\d+$'){if(Test-Path (Join-Path $rx.FullName access-units.bin)){Decode $rx.FullName}}
  & (Join-Path $PSScriptRoot Verify-TransportEncoding.ps1) -EvidenceDirectory $dir -MinimumSeconds $seconds -DecodeSubdirectory decode | Set-Content (Join-Path $root ($name+'-encoding.txt'))
  & (Join-Path $PSScriptRoot Verify-LocalTransport.ps1) -EvidenceDirectory $dir -Reconnect:$reconnect | Set-Content (Join-Path $root ($name+'-transport.txt'))
  $result.Runs+=@{Name=$name;Outcome='PASS_ENCODE_DECODE_TRANSPORT';Reconnect=$reconnect};Save
 }
 $result.Outcome='PASS';$result.Current='COMPLETE'
}catch{$result.Outcome='ERROR';$result.Error=$_.Exception.Message;$result.Position=$_.InvocationInfo.PositionMessage}
finally{$result.CompletedUtc=[datetime]::UtcNow.ToString('o');Save}
if($result.Outcome -ne 'PASS'){throw $result.Error}
