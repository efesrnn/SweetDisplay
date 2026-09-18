# Run from an elevated Windows PowerShell. No install/signing/security changes.
# Creates one temporary pattern window and terminates only its own crash-test Host.
param([string]$RunName = ('run-' + (Get-Date -Format 'yyyyMMdd-HHmmss')))
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
if($RunName -notmatch '^[a-zA-Z0-9-]+$'){throw 'RunName must be a simple directory name'}
$evidence=Join-Path $repo ('docs\evidence\private\phase2\'+$RunName)
if(Test-Path -LiteralPath $evidence){throw 'Evidence directory already exists; preserve earlier run'}
if(!([Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator))){throw 'Administrator token required for frame interface and read-only health checks'}
New-Item -ItemType Directory -Path $evidence | Out-Null
$hostExe=Join-Path $repo 'out\host\SweetDisplayHost.exe'
$probeExe=Join-Path $repo 'out\diagnostics\SweetDisplayFrameProbe.exe'
$ledger=[Collections.Generic.List[object]]::new()
$result=@{Outcome='Running';StartedUtc=[DateTime]::UtcNow.ToString('o');Steps=@();Error=$null}
$pattern=$null;$crashHost=$null
function Save {
 $result.Steps=@($ledger.ToArray())
 $result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $evidence 'execution.json') -Encoding utf8
}
function Read-FailureEvents($Filter) {
 try {Get-WinEvent -FilterHashtable $Filter -ErrorAction Stop}
 catch {if($_.FullyQualifiedErrorId -notlike 'NoMatchingEventsFound*'){throw}}
}
function Step([string]$Name,[scriptblock]$Action){
 $entry=@{Name=$Name;Command=$Action.ToString().Trim();Output=$null;Error=$null};$ledger.Add($entry);Save
 try{$entry.Output=& $Action}catch{$entry.Error=$_.Exception.Message;throw}finally{Save}
}
function Run-Host([string]$Name,[int]$Seconds,[string]$Extra=''){
 $dir=Join-Path $evidence $Name;New-Item -ItemType Directory -Path $dir | Out-Null
 $arguments='--output "'+$dir+'" --seconds '+$Seconds+' '+$Extra
 $entry=@{Executable=$hostExe;Arguments=$arguments;ExitCode=$null;Stdout=$null;Stderr=$null};$ledger.Add($entry);Save
 # Native invocation preserves the actual native exit code.
 # Host bounds readiness (15 s), each GPU wait (0.5 s) and each measurement run.
 $hostArguments=@('--output',$dir,'--seconds',[string]$Seconds)+@($Extra.Split(' ',[StringSplitOptions]::RemoveEmptyEntries))
 $prior=$ErrorActionPreference
 try {$ErrorActionPreference='Continue'; & $hostExe @hostArguments 1> (Join-Path $dir 'stdout.txt') 2> (Join-Path $dir 'stderr.txt'); $exitCode=$LASTEXITCODE}
 finally {$ErrorActionPreference=$prior}
 # File.ReadAllText returns plain strings. Windows PowerShell Get-Content strings
 # carry ETS filesystem properties which must not enter the JSON command ledger.
 $entry.ExitCode=$exitCode;$entry.Stdout=[IO.File]::ReadAllText((Join-Path $dir 'stdout.txt'));$entry.Stderr=[IO.File]::ReadAllText((Join-Path $dir 'stderr.txt'));Save
 if($exitCode -ne 0){throw "Host exit=$exitCode; $($entry.Stderr)"}
 $dir
}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class SweetDisplayPhase2CI {
 [StructLayout(LayoutKind.Sequential)] public struct Info {public uint Length;public uint Options;}
 [DllImport("ntdll.dll")] public static extern int NtQuerySystemInformation(int c,ref Info i,uint n,out uint r);
 [DllImport("kernel32.dll",SetLastError=true)] [return:MarshalAs(UnmanagedType.Bool)] public static extern bool GetExitCodeProcess(IntPtr h,out uint code);
}
'@
function Health([string]$Name){
 $ci=[SweetDisplayPhase2CI+Info]::new();$ci.Length=8;[uint32]$n=0
 $status=[SweetDisplayPhase2CI]::NtQuerySystemInformation(103,[ref]$ci,8,[ref]$n)
 $devices=@(Get-PnpDevice -PresentOnly | Where-Object {$_.FriendlyName -like '*SweetDisplay*' -or $_.InstanceId -match 'SweetDisplay|SWT0001'} | ForEach-Object {
  [pscustomobject]@{Name=$_.FriendlyName;Class=$_.Class;Status=$_.Status;InstanceId=$_.InstanceId;Problem=(Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_ProblemCode).Data;Inf=(Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_DriverInfPath).Data}
 })
 $snapshot=@{SecureBoot=Confirm-SecureBootUEFI;Hvci=Get-ItemPropertyValue 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\State' HVCIEnabled;CiStatus=$status;CiFlags=$ci.Options;Devices=$devices;Boot=(Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime().ToString('o')}
 $snapshot | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $evidence ($Name+'-health.json')) -Encoding utf8
 if(!$snapshot.SecureBoot -or $snapshot.Hvci -ne 1 -or $status -ne 0 -or ($ci.Options -band 1) -eq 0 -or ($ci.Options -band 2) -ne 0 -or ($ci.Options -band 0x400) -eq 0){throw 'Required security baseline check failed'}
 if($devices.Count -ne 2 -or @($devices | Where-Object {$_.Problem -ne 0 -or $_.Status -ne 'OK'}).Count){throw 'Project PnP health failed'}
 & $probeExe --inventory (Join-Path $evidence ($Name+'-display.txt'))
 if($LASTEXITCODE -ne 0){throw 'Display inventory failed'}
 $snapshot
}
try {
 Step 'Read-only initial health/security' {$script:before=Health 'before';$before}
 Step 'Start changing SWT0001 pattern; no ETW or BMP' {
  $script:nonce=('{0:X8}' -f (Get-Random -Minimum 1 -Maximum ([int]::MaxValue)))
  $script:pattern=Start-Process -FilePath $probeExe -ArgumentList ('--pattern "'+$evidence+'" '+$nonce) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $evidence 'pattern-stdout.txt') -RedirectStandardError (Join-Path $evidence 'pattern-stderr.txt')
  Start-Sleep -Seconds 2;$pattern.Refresh();if($pattern.HasExited){throw "Pattern exit=$($pattern.ExitCode)"}
  @{Executable=$probeExe;Arguments=@('--pattern',$evidence,$nonce);ProcessId=$pattern.Id}
 }
 Step 'Host absent: source continues' {Run-Host 'absent-before' 4 '--inspect-seconds 4'}
 Step '35 seconds real GPU frames and clean reconnect for another 35 seconds' {
  Run-Host 'continuous' 35 ('--sample --nonce '+$nonce+' --sessions 2')
 }
 Step 'New Host process with slow consumption: bounded overflow' {
  Run-Host 'slow' 12 ('--sample --nonce '+$nonce+' --slow-ms 100')
 }
 Step 'Terminate only test Host while it owns a GPU lease' {
  $dir=Join-Path $evidence 'crash';New-Item -ItemType Directory -Path $dir | Out-Null
  $args='--output "'+$dir+'" --seconds 60 --sample --nonce '+$nonce+' --hold-ms 1000'
  $script:crashHost=Start-Process -FilePath $hostExe -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt')
  $crashHandle=$crashHost.Handle
  $deadline=(Get-Date).AddSeconds(10)
  do {Start-Sleep -Milliseconds 50;$crashHost.Refresh();if($crashHost.HasExited){throw "Crash probe exited early: $(Get-Content (Join-Path $dir 'stderr.txt') -Raw)"};$text=Get-Content (Join-Path $dir 'stdout.txt') -Raw} while($text -notmatch 'GPU lease HELD' -and (Get-Date) -lt $deadline)
  if($text -notmatch 'GPU lease HELD'){throw 'Crash probe never acquired GPU lease'}
  $crashHost.Kill();$crashHost.WaitForExit()
  [uint32]$crashCode=0
  if(![SweetDisplayPhase2CI]::GetExitCodeProcess($crashHandle,[ref]$crashCode)){throw 'Crash exit-code query failed'}
  @{Executable=$hostExe;Arguments=$args;Action='Terminate own test process after HELD marker';ProcessId=$crashHost.Id;ExitCode=$crashCode}
 }
 Step 'After Host crash: source continues and connection is cleaned' {Run-Host 'absent-after-crash' 4 '--inspect-seconds 4'}
 Step 'Fresh Host reconnect after crash without reboot' {Run-Host 'after-crash' 12 ('--sample --nonce '+$nonce)}
 Step 'Read-only final health/security and failure-event audit' {
  $after=Health 'after'
  if($after.Boot -ne $before.Boot){throw 'Boot time changed during integration'}
  $started=[datetime]::Parse($result.StartedUtc).ToLocalTime()
  $events=@(Read-FailureEvents @{LogName='Application';StartTime=$started;Id=1000,1001} | Where-Object {$_.Message -match 'WUDFHost|SweetDisplayDriver'})
  $events+=@(Read-FailureEvents @{LogName='System';StartTime=$started;Id=10110,10111,41,1001})
  ConvertTo-Json -InputObject @($events | Select-Object TimeCreated,Id,ProviderName,Message) -Depth 4 | Set-Content (Join-Path $evidence 'failure-events.json') -Encoding utf8
  $result.FailureEventCount=$events.Count
  if($events.Count){throw 'New driver/system failure event; review private evidence'}
  $after
 }
 $result.Outcome='PASS'
} catch {$result.Outcome='FAIL';$result.Error=@{Message=$_.Exception.Message;HResult=('0x{0:X8}' -f $_.Exception.HResult)}}
finally {
 if($pattern -and !$pattern.HasExited){$result.PatternCloseRequested=$pattern.CloseMainWindow()}
 if($crashHost -and !$crashHost.HasExited){$crashHost.Kill()}
 $result.CompletedUtc=[DateTime]::UtcNow.ToString('o');Save
}
if($result.Outcome -ne 'PASS'){throw $result.Error.Message}
Write-Output "PASS: integration evidence saved under docs/evidence/private/phase2/$RunName"
