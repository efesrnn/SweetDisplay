# Elevated local benchmark only. No installation, signing, boot or phone commands.
param([string]$RunName='gpu-capacity-1',[ValidateRange(10,7200)][int]$Seconds=60,
 [ValidateRange(0,240)][int]$PatternFps=60,[ValidateRange(0,1)][int]$SyncInterval=1,
 [switch]$SkipSampling,[ValidateRange(0,60)][int]$InspectBefore=0,
 [ValidateRange(0,60)][int]$MetadataSeconds=0)
$ErrorActionPreference='Stop'
[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
if($RunName -notmatch '^[a-zA-Z0-9-]+$'){throw 'Invalid run name'}
if(!([Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator))){throw 'Elevated token required for the existing frame interface'}
$evidence=Join-Path $repo ('docs\evidence\private\phase3\'+$RunName)
if(Test-Path $evidence){throw 'Preserve existing evidence; choose a new run name'}
New-Item -ItemType Directory -Path $evidence | Out-Null
$hostExe=Join-Path $repo 'out\host\SweetDisplayHost.exe'
$patternExe=Join-Path $repo 'out\gpu-pattern\SweetDisplayGpuPattern.exe'
$probeExe=Join-Path $repo 'out\diagnostics\SweetDisplayFrameProbe.exe'
$result=@{Outcome='Running';StartedUtc=[DateTime]::UtcNow.ToString('o');Commands=@();Error=$null}
$pattern=$null;$consumer=$null
function Save {$result | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $evidence 'execution.json') -Encoding utf8}
function Log([string]$Command,$Value){$result.Commands+=@{Command=$Command;Result=$Value};Save}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class SweetDisplayCapacityNative {
 [StructLayout(LayoutKind.Sequential)] public struct Info {public uint Length;public uint Options;}
 [DllImport("ntdll.dll")] public static extern int NtQuerySystemInformation(int c,ref Info i,uint n,out uint r);
 [DllImport("kernel32.dll",SetLastError=true)] [return:MarshalAs(UnmanagedType.Bool)] public static extern bool GetExitCodeProcess(IntPtr h,out uint code);
}
'@
function Health([string]$Name){
 $ci=[SweetDisplayCapacityNative+Info]::new();$ci.Length=8;[uint32]$n=0
 $status=[SweetDisplayCapacityNative]::NtQuerySystemInformation(103,[ref]$ci,8,[ref]$n)
 $devices=@(Get-PnpDevice -Class Display,Monitor -PresentOnly | Where-Object {$_.FriendlyName -like '*SweetDisplay*' -or $_.InstanceId -match 'SweetDisplay|SWT0001'} | ForEach-Object {
  [pscustomobject]@{Name=$_.FriendlyName;Class=$_.Class;Status=$_.Status;InstanceId=$_.InstanceId;Problem=(Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_ProblemCode).Data;Inf=(Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_DriverInfPath).Data}
 })
 $snapshot=@{Utc=[DateTime]::UtcNow.ToString('o');SecureBoot=Confirm-SecureBootUEFI;Hvci=Get-ItemPropertyValue 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\State' HVCIEnabled;CiStatus=$status;CiFlags=$ci.Options;Devices=$devices;Boot=(Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime().ToString('o')}
 $snapshot | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $evidence ($Name+'-health.json')) -Encoding utf8
 if(!$snapshot.SecureBoot -or $snapshot.Hvci -ne 1 -or $status -ne 0 -or ($ci.Options -band 1) -eq 0 -or ($ci.Options -band 2) -ne 0 -or ($ci.Options -band 0x400) -eq 0){throw 'Security baseline failed'}
 if($devices.Count -ne 2 -or @($devices | Where-Object {$_.Problem -ne 0 -or $_.Status -ne 'OK'}).Count){throw 'SweetDisplay PnP health failed'}
 $snapshot
}
function Read-Events($Filter){try{Get-WinEvent -FilterHashtable $Filter -ErrorAction Stop}catch{if($_.FullyQualifiedErrorId -notlike 'NoMatchingEventsFound*'){throw}}}
function Profile([hashtable]$Roles){
 $elapsed=([DateTime]::UtcNow-$script:profileStart).TotalSeconds
 foreach($role in $Roles.Keys){
  try {$process=Get-Process -Id $Roles[$role] -ErrorAction Stop}
  catch {if($role -eq 'Host' -and $consumer.HasExited){continue};throw}
  [pscustomobject]@{Run=$script:profileLabel;ElapsedSeconds=$elapsed;Role=$role;ProcessId=$process.Id;CpuSeconds=$process.TotalProcessorTime.TotalSeconds;PrivateBytes=$process.PrivateMemorySize64;WorkingSetBytes=$process.WorkingSet64;Handles=$process.HandleCount;Threads=$process.Threads.Count} |
   Export-Csv (Join-Path $evidence 'resources.csv') -Append -NoTypeInformation -Encoding utf8
 }
 if($script:gpuCountersAvailable){
  try {
   $filter=($Roles.Values | ForEach-Object {"Name LIKE 'pid_$($_)_%'"}) -join ' OR '
   Get-CimInstance -ClassName Win32_PerfFormattedData_GPUPerformanceCounters_GPUEngine -Filter $filter | ForEach-Object {
    [pscustomobject]@{Run=$script:profileLabel;ElapsedSeconds=$elapsed;Engine=$_.Name;UtilizationPercent=$_.UtilizationPercentage} |
     Export-Csv (Join-Path $evidence 'gpu-engines.csv') -Append -NoTypeInformation -Encoding utf8
   }
  } catch {$script:gpuCountersAvailable=$false;Log 'Optional GPU counters unavailable' $_.Exception.Message}
 }
}
function Run-Consumer([string]$Name,[int]$Duration){
 $dir=Join-Path $evidence $Name;New-Item -ItemType Directory -Path $dir | Out-Null
 $arguments='--output "'+$dir+'" --seconds '+$Duration
 if(!$SkipSampling){$arguments+=' --sample --nonce '+$script:nonce}
 $script:consumer=Start-Process -FilePath $hostExe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt')
 $processHandle=$consumer.Handle
 Log 'Start bounded Host measurement' @{Executable=$hostExe;Arguments=$arguments;ProcessId=$consumer.Id}
 $roles=@{Host=$consumer.Id;Pattern=$pattern.Id};if($script:driverPid){$roles.Driver=$script:driverPid}
 $script:profileStart=[DateTime]::UtcNow
 $script:profileLabel=$Name
 $deadline=$script:profileStart.AddSeconds($Duration+30);$nextSample=$script:profileStart;$nextHealth=$script:profileStart.AddSeconds(30);$healthIndex=0
 while(!$consumer.WaitForExit(100)){
  if($pattern.HasExited){throw "Pattern exited during measurement: $([IO.File]::ReadAllText((Join-Path $evidence 'pattern-stderr.txt')))"}
  if([DateTime]::UtcNow -gt $deadline){$consumer.Kill();throw 'Own test Host exceeded bounded duration'}
  if([DateTime]::UtcNow -ge $nextSample){Profile $roles;$nextSample=[DateTime]::UtcNow.AddSeconds(5)}
  if([DateTime]::UtcNow -ge $nextHealth){$healthIndex++;Health ($Name+'-periodic-'+$healthIndex) | Out-Null;$nextHealth=[DateTime]::UtcNow.AddSeconds(30)}
 }
 [uint32]$code=0;if(![SweetDisplayCapacityNative]::GetExitCodeProcess($processHandle,[ref]$code)){throw 'Exit-code query failed'}
 Log 'Host completed' @{ExitCode=$code;Stdout=[IO.File]::ReadAllText((Join-Path $dir 'stdout.txt'));Stderr=[IO.File]::ReadAllText((Join-Path $dir 'stderr.txt'))}
 if($code -ne 0){throw "Host exit=$code; $([IO.File]::ReadAllText((Join-Path $dir 'stderr.txt')))"}
}
try {
 Save
 $before=Health 'before';Log 'Read-only baseline health/security' $before
 & $probeExe --inventory (Join-Path $evidence 'before-display.txt');if($LASTEXITCODE -ne 0){throw 'Display inventory failed'}
 $script:driverPid=0
 $candidates=@(Get-CimInstance Win32_Process -Filter "Name='WUDFHost.exe'")
 $matches=@($candidates | Where-Object {$_.CommandLine -match 'SweetDisplayDriverGroup'})
 if(!$matches.Count){
  $matches=@(foreach($c in $candidates){try{if(@((Get-Process -Id $c.ProcessId).Modules | Where-Object ModuleName -eq 'SweetDisplayDriver.dll').Count){$c}}catch{}})
 }
 if($matches.Count -eq 1){$script:driverPid=[int]$matches[0].ProcessId}
 Log 'Identify SweetDisplay UMDF host by group or loaded project DLL' @{MatchingProcesses=@($matches | Select-Object Name,ProcessId,CommandLine);DriverProcessId=$script:driverPid}
 $result.LogicalProcessors=[Environment]::ProcessorCount
 $script:gpuCountersAvailable=$true
 try {Get-CimClass -ClassName Win32_PerfFormattedData_GPUPerformanceCounters_GPUEngine | Out-Null}catch{$script:gpuCountersAvailable=$false;Log 'Optional GPU counter class unavailable' $_.Exception.Message}
 $script:nonce=('{0:X8}' -f (Get-Random -Minimum 1 -Maximum ([int]::MaxValue)))
 $patternArguments='"'+$evidence+'" '+$nonce+' '+($Seconds+90+$InspectBefore+$MetadataSeconds)+' '+$PatternFps+' '+$SyncInterval
 $script:pattern=Start-Process -FilePath $patternExe -ArgumentList $patternArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $evidence 'pattern-stdout.txt') -RedirectStandardError (Join-Path $evidence 'pattern-stderr.txt')
 $patternHandle=$pattern.Handle
 Log 'Start D3D11 pattern on SWT0001' @{Executable=$patternExe;Arguments=$patternArguments;ProcessId=$pattern.Id}
 Start-Sleep -Seconds 2;$pattern.Refresh();if($pattern.HasExited){throw "Pattern exited: $([IO.File]::ReadAllText((Join-Path $evidence 'pattern-stderr.txt')))"}
 if($InspectBefore){
  $absentDir=Join-Path $evidence 'absent';New-Item -ItemType Directory -Path $absentDir|Out-Null
  & $hostExe --output $absentDir --inspect-seconds $InspectBefore 1> (Join-Path $absentDir 'stdout.txt') 2> (Join-Path $absentDir 'stderr.txt')
  $absentExit=$LASTEXITCODE
  Log 'Measure IddCx acquisitions without shared textures/Host connection' @{Command='SweetDisplayHost --inspect-seconds '+$InspectBefore;ExitCode=$absentExit;Report=[IO.File]::ReadAllText((Join-Path $absentDir 'session-1-absent.json'))}
  if($absentExit -ne 0){throw "Absent Host inspection exit=$absentExit"}
 }
 if($MetadataSeconds){
  $traceDir=Join-Path $evidence 'source-metadata';New-Item -ItemType Directory -Path $traceDir|Out-Null
  & $probeExe --metadata $traceDir $MetadataSeconds 1> (Join-Path $traceDir 'stdout.txt') 2> (Join-Path $traceDir 'stderr.txt')
  $traceExit=$LASTEXITCODE
  Log 'Metadata-only IddCx ETW without Host; capture keyword disabled' @{Command='SweetDisplayFrameProbe --metadata '+$MetadataSeconds;ExitCode=$traceExit;Report=[IO.File]::ReadAllText((Join-Path $traceDir 'result.json'))}
  if($traceExit -ne 0){throw "Metadata-only ETW exit=$traceExit"}
 }
 Run-Consumer 'main' $Seconds
 Run-Consumer 'reconnect' 10
 $after=Health 'after';Log 'Read-only final health/security' $after
 if($before.Boot -ne $after.Boot){throw 'Boot time changed'}
 $started=([datetime]$result.StartedUtc).ToLocalTime()
 $events=@(Read-Events @{LogName='Application';StartTime=$started;Id=1000,1001} | Where-Object {$_.Message -match 'WUDFHost|SweetDisplayDriver'})
 $events+=@(Read-Events @{LogName='System';StartTime=$started;Id=10110,10111,41,1001})
 ConvertTo-Json -InputObject @($events | Select-Object TimeCreated,Id,ProviderName,Message) -Depth 4 | Set-Content (Join-Path $evidence 'failure-events.json') -Encoding utf8
 $result.FailureEventCount=$events.Count;if($events.Count){throw 'Driver/system failure event; inspect evidence'}
 & $probeExe --inventory (Join-Path $evidence 'after-display.txt');if($LASTEXITCODE -ne 0){throw 'Final display inventory failed'}
 $result.Outcome='PASS'
} catch {$result.Outcome='FAIL';$result.Error=@{Message=$_.Exception.Message;HResult=('0x{0:X8}' -f $_.Exception.HResult)}}
finally {
 if($consumer -and !$consumer.HasExited){$consumer.Kill()}
 if($pattern -and !$pattern.HasExited){$result.PatternCloseRequested=$pattern.CloseMainWindow();if(!$pattern.WaitForExit(5000)){$pattern.Kill();$result.PatternForcedStop=$true}}
 $result.CompletedUtc=[DateTime]::UtcNow.ToString('o');Save
}
if($result.Outcome -ne 'PASS'){throw $result.Error.Message}
Write-Output ('PASS: '+$RunName)
