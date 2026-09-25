#requires -Version 7.4
#requires -RunAsAdministrator
param([Parameter(Mandatory=$true)][string]$RunDirectory)
$ErrorActionPreference='Stop'
[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
$RunDirectory=(Resolve-Path -LiteralPath $RunDirectory).Path
$configurationPath=Join-Path $RunDirectory 'elevated-host-command.json'
if(!(Test-Path -LiteralPath $configurationPath -PathType Leaf)){throw 'Missing elevated Host command'}
$configuration=Get-Content -Raw -LiteralPath $configurationPath|ConvertFrom-Json
$hostDir=Join-Path $RunDirectory 'host';$resultPath=Join-Path $RunDirectory 'elevated-host-result.json';$startPath=Join-Path $RunDirectory 'elevated-host-start.json';$readyPath=Join-Path $RunDirectory 'elevated-worker-ready.json';$patternReady=Join-Path $RunDirectory 'pattern-ready.request';$workerStop=Join-Path $RunDirectory 'elevated-worker-stop.request'
$result=[ordered]@{Outcome='RUNNING';StartedUtc=[datetime]::UtcNow.ToString('o');HostPid=$null;HostExitCode=$null;HostForcedStop=$false;Error=$null}
function Save-Result{$result|ConvertTo-Json -Depth 10|Set-Content -LiteralPath $resultPath -Encoding utf8}
function Get-Health([string]$Name){
 $devices=@(Get-PnpDevice -Class Display,Monitor -PresentOnly|Where-Object{$_.FriendlyName-like'*SweetDisplay*'-or$_.InstanceId-match'SweetDisplay|SWT0001'}|ForEach-Object{[pscustomobject]@{Class=$_.Class;Status=$_.Status;Problem=(Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_ProblemCode).Data}})
 $drivers=@(Get-CimInstance Win32_Process -Filter "Name='WUDFHost.exe'"|Where-Object{$_.CommandLine-match'SweetDisplayDriverGroup'})
 $health=[ordered]@{Utc=[datetime]::UtcNow.ToString('o');SecureBoot=Confirm-SecureBootUEFI;Hvci=Get-ItemPropertyValue 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\State' HVCIEnabled;Boot=(Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime().ToString('o');Devices=$devices;DriverPids=@($drivers.ProcessId)}
 $health|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $RunDirectory ($Name+'-health.json')) -Encoding utf8
 if(!$health.SecureBoot-or$health.Hvci-ne1-or$devices.Count-ne2-or@($devices|Where-Object{$_.Status-ne'OK'-or$_.Problem-ne0}).Count-or$drivers.Count-ne1){throw 'Windows security/PnP/driver health failed'}
 return $health
}
$hostProcess=$null;$before=$null
try{
 Save-Result;$before=Get-Health 'before'
 [ordered]@{Utc=[datetime]::UtcNow.ToString('o');DriverPid=[int]$before.DriverPids[0]}|ConvertTo-Json|Set-Content -LiteralPath $readyPath -Encoding utf8
 $patternDeadline=[datetime]::UtcNow.AddSeconds(120)
 while(!(Test-Path -LiteralPath $patternReady)){if(Test-Path -LiteralPath $workerStop){throw 'Elevated worker stopped before pattern readiness'};if([datetime]::UtcNow-gt$patternDeadline){throw 'Pattern readiness deadline'};Start-Sleep -Milliseconds 100}
 $hostProcess=Start-Process -FilePath ([string]$configuration.HostExe) -ArgumentList ([string]$configuration.HostArguments) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $hostDir 'stdout.txt') -RedirectStandardError (Join-Path $hostDir 'stderr.txt')
 $result.HostPid=$hostProcess.Id
 [ordered]@{Utc=[datetime]::UtcNow.ToString('o');HostPid=$hostProcess.Id;DriverPid=[int]$before.DriverPids[0]}|ConvertTo-Json|Set-Content -LiteralPath $startPath -Encoding utf8
 $deadline=[datetime]::UtcNow.AddSeconds([int]$configuration.Seconds+180)
 while(!$hostProcess.WaitForExit(100)){
  if(Test-Path -LiteralPath $workerStop){if(!(Test-Path -LiteralPath ([string]$configuration.StopFile))){'elevated-worker-stop'|Set-Content -LiteralPath ([string]$configuration.StopFile) -Encoding ascii}}
  if([datetime]::UtcNow-gt$deadline){if(!(Test-Path -LiteralPath ([string]$configuration.StopFile))){'elevated-worker-deadline'|Set-Content -LiteralPath ([string]$configuration.StopFile) -Encoding ascii};if(!$hostProcess.WaitForExit(20000)){$hostProcess.Kill();$result.HostForcedStop=$true};throw 'Host exceeded elevated bounded deadline'}
 }
 $result.HostExitCode=$hostProcess.ExitCode;if($hostProcess.ExitCode-ne0){throw('Host failed: '+[IO.File]::ReadAllText((Join-Path $hostDir 'stderr.txt')))}
 $result.Outcome='PASS'
}catch{$result.Outcome='ERROR';$result.Error=$_.Exception.Message}
finally{
 if($hostProcess-and!$hostProcess.HasExited){if(!(Test-Path -LiteralPath ([string]$configuration.StopFile))){'elevated-worker-cleanup'|Set-Content -LiteralPath ([string]$configuration.StopFile) -Encoding ascii};if(!$hostProcess.WaitForExit(20000)){$hostProcess.Kill();$result.HostForcedStop=$true}}
 try{$after=Get-Health 'after';if($before-and($before.Boot-ne$after.Boot-or$before.Hvci-ne$after.Hvci)){throw 'Windows boot/security changed'}}catch{$result.HealthError=$_.Exception.Message;$result.Outcome='ERROR'}
 $result.CompletedUtc=[datetime]::UtcNow.ToString('o');Save-Result
}
if($result.Outcome-ne'PASS'){throw($result.Outcome+': '+$result.Error+' '+$result.HealthError)}
