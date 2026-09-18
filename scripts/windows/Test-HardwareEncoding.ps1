# Diagnostic sessions only: no install, signing, boot/security change or phone action.
param([Parameter(Mandatory=$true)][string]$RunName,
 [ValidateRange(10,1900)][int]$Seconds=300,
 [ValidateSet('observe','cover','snapshot','minimize','foreground')][string]$ControlCase='observe',
 [switch]$Classified,[switch]$Reconnect,[int]$Width=800,[int]$Height=360,[int]$Rate=10,[int]$Bitrate=2000000,[switch]$UncappedSubmission)
$ErrorActionPreference='Stop'
[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
if(!([Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator))){throw 'Existing device interface requires an elevated token'}
if($RunName -notmatch '^[a-zA-Z0-9-]+$'){throw 'Invalid run name'}
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$dir=Join-Path $repo ('docs/evidence/private/phase3b/'+$RunName)
if(Test-Path $dir){throw 'Preserve evidence: use a new run name'}
New-Item -ItemType Directory -Path $dir|Out-Null
$consumerDir=Join-Path $dir 'host';New-Item -ItemType Directory -Path $consumerDir|Out-Null
$result=@{Outcome='RUNNING';ControlCase=$ControlCase;Classified=[bool]$Classified;RequestedSeconds=$Seconds;StartedUtc=[datetime]::UtcNow.ToString('o');Commands=@();Error=$null}
function Save{$result|ConvertTo-Json -Depth 10|Set-Content (Join-Path $dir 'execution.json') -Encoding utf8}
function Log([string]$Command,$Value){$result.Commands+=@{Utc=[datetime]::UtcNow.ToString('o');Command=$Command;Result=$Value};Save}
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class SweetDisplayDiagNative {
 [StructLayout(LayoutKind.Sequential)] public struct Info {public uint Length;public uint Options;}
 [DllImport("ntdll.dll")] public static extern int NtQuerySystemInformation(int c,ref Info i,uint n,out uint r);
 [DllImport("kernel32.dll",SetLastError=true)] [return:MarshalAs(UnmanagedType.Bool)] public static extern bool GetExitCodeProcess(IntPtr h,out uint code);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h,out uint pid);
 [DllImport("user32.dll",SetLastError=true)] [return:MarshalAs(UnmanagedType.Bool)] public static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
}
'@
function Health([string]$Name){
 $ci=[SweetDisplayDiagNative+Info]::new();$ci.Length=8;[uint32]$n=0
 $status=[SweetDisplayDiagNative]::NtQuerySystemInformation(103,[ref]$ci,8,[ref]$n)
 $devices=@(Get-PnpDevice -Class Display,Monitor -PresentOnly|Where-Object {$_.FriendlyName -like '*SweetDisplay*' -or $_.InstanceId -match 'SweetDisplay|SWT0001'}|ForEach-Object {
  [pscustomobject]@{Class=$_.Class;Name=$_.FriendlyName;Status=$_.Status;InstanceId=$_.InstanceId;Problem=(Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_ProblemCode).Data;Inf=(Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_DriverInfPath).Data}
 })
 $snapshot=@{Utc=[datetime]::UtcNow.ToString('o');SecureBoot=Confirm-SecureBootUEFI;Hvci=Get-ItemPropertyValue 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\State' HVCIEnabled;CiStatus=$status;CiFlags=$ci.Options;Devices=$devices;Boot=(Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime().ToString('o')}
 $snapshot|ConvertTo-Json -Depth 6|Set-Content (Join-Path $dir ($Name+'-health.json')) -Encoding utf8
 if(!$snapshot.SecureBoot -or $snapshot.Hvci -ne 1 -or $status -ne 0 -or ($ci.Options -band 1) -eq 0 -or ($ci.Options -band 2) -ne 0 -or ($ci.Options -band 0x400) -eq 0){throw 'Security baseline failure'}
 if($devices.Count -ne 2 -or @($devices|Where-Object {$_.Problem -ne 0 -or $_.Status -ne 'OK'}).Count){throw 'SweetDisplay PnP health failure'}
 return $snapshot
}
function Events($Filter){try{Get-WinEvent -FilterHashtable $Filter -ErrorAction Stop}catch{if($_.FullyQualifiedErrorId -notlike 'NoMatchingEventsFound*'){throw}}}
if($Width -notin @(800,1280,1920,2400) -or $Height -notin @(360,576,864,1080) -or $Rate -notin @(10,30,60) -or $Bitrate -lt 100000 -or $Bitrate -gt 100000000){throw 'Invalid encoding test mode'}
$encodeArgs=' --encode --encode-width '+$Width+' --encode-height '+$Height+' --encode-fps '+$Rate+' --encode-bitrate '+$Bitrate
if($UncappedSubmission){$encodeArgs+=' --encode-uncapped'}
$result.EncodeMode=@{Width=$Width;Height=$Height;Rate=$Rate;Bitrate=$Bitrate;UncappedSubmission=[bool]$UncappedSubmission}
$pattern=$null;$consumer=$null;$before=$null;$driverPid=0
try {
 Save;$before=Health 'before';Log 'Read-only preflight' $before
 $drivers=@(Get-CimInstance Win32_Process -Filter "Name='WUDFHost.exe'"|Where-Object {$_.CommandLine -match 'SweetDisplayDriverGroup'})
 if($drivers.Count -ne 1){throw 'Cannot identify unique existing SweetDisplay UMDF host'};$driverPid=[int]$drivers[0].ProcessId
 $nonce='{0:X8}' -f (Get-Random -Minimum 1 -Maximum ([int]::MaxValue));$result.Nonce=$nonce;$result.DriverPid=$driverPid
 $patternExe=Join-Path $repo 'out/gpu-pattern/SweetDisplayGpuPattern.exe'
 $patternArgs='"'+$dir+'" '+$nonce+' '+($Seconds+120)+' 0 1 '+$ControlCase
 $pattern=Start-Process -FilePath $patternExe -ArgumentList $patternArgs -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'pattern-stdout.txt') -RedirectStandardError (Join-Path $dir 'pattern-stderr.txt')
 $patternHandle=$pattern.Handle;Log 'Start own instrumented test pattern' @{Executable=$patternExe;Arguments=$patternArgs;ProcessId=$pattern.Id}
 Start-Sleep -Seconds 2
 if($pattern.HasExited){throw ('Pattern startup failed: '+[IO.File]::ReadAllText((Join-Path $dir 'pattern-stderr.txt')))}
 $hostExe=Join-Path $repo $(if($UncappedSubmission){'out/encode-host-control/SweetDisplayHost.exe'}else{'out/encode-host/SweetDisplayHost.exe'})
 $arguments='--output "'+$consumerDir+'" --seconds '+$Seconds+' --first-fail --nonce '+$nonce
 if($Classified){$arguments+=' --classified'}
 $arguments+=$encodeArgs
 $consumer=Start-Process -FilePath $hostExe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $consumerDir 'stdout.txt') -RedirectStandardError (Join-Path $consumerDir 'stderr.txt')
 $processHandle=$consumer.Handle;Log 'Start fail-fast Host' @{Executable=$hostExe;Arguments=$arguments;ProcessId=$consumer.Id}
 $start=[datetime]::UtcNow;$deadline=$start.AddSeconds($Seconds+45);$nextProfile=$start;$nextHealth=$start.AddSeconds(30);$healthIndex=0
 while(!$consumer.WaitForExit(100)){
  if($pattern.HasExited){throw ('Pattern exited during test: '+[IO.File]::ReadAllText((Join-Path $dir 'pattern-stderr.txt')))}
  if([datetime]::UtcNow -gt $deadline){throw 'Host exceeded bounded test deadline'}
  if([datetime]::UtcNow -ge $nextProfile){
   foreach($entry in @(@{Role='Host';Id=$consumer.Id},@{Role='Pattern';Id=$pattern.Id},@{Role='Driver';Id=$driverPid})){
    $p=Get-Process -Id $entry.Id -ErrorAction SilentlyContinue
    if($p){[pscustomobject]@{Qpc=[Diagnostics.Stopwatch]::GetTimestamp();ElapsedSeconds=([datetime]::UtcNow-$start).TotalSeconds;Role=$entry.Role;Id=$p.Id;CpuSeconds=$p.TotalProcessorTime.TotalSeconds;PrivateBytes=$p.PrivateMemorySize64;WorkingSetBytes=$p.WorkingSet64;Handles=$p.HandleCount}|Export-Csv (Join-Path $dir 'resources.csv') -Append -NoTypeInformation -Encoding utf8}
     }
  try{
   Get-CimInstance -ClassName Win32_PerfFormattedData_GPUPerformanceCounters_GPUEngine -Filter ("Name LIKE '%pid_"+$consumer.Id+"_%'") | Select-Object @{Name='ElapsedSeconds';Expression={([datetime]::UtcNow-$start).TotalSeconds}},Name,UtilizationPercentage | Export-Csv (Join-Path $dir 'gpu-engines.csv') -Append -NoTypeInformation -Encoding utf8
  }catch{Add-Content (Join-Path $dir 'gpu-counter-errors.txt') $_.Exception.Message}
  $nextProfile=[datetime]::UtcNow.AddSeconds(5)
  }
  if([datetime]::UtcNow -ge $nextHealth){$healthIndex++;Health ('periodic-'+$healthIndex)|Out-Null;$nextHealth=[datetime]::UtcNow.AddSeconds(30)}
 }
 [uint32]$exitCode=0;if(![SweetDisplayDiagNative]::GetExitCodeProcess($processHandle,[ref]$exitCode)){throw 'Host exit query failed'}
 $result.HostExitCode=$exitCode
 Log 'Host completed' @{ExitCode=$exitCode;Stdout=[IO.File]::ReadAllText((Join-Path $consumerDir 'stdout.txt'));Stderr=[IO.File]::ReadAllText((Join-Path $consumerDir 'stderr.txt'))}
 if($exitCode -ne 0){
  if(Test-Path (Join-Path $consumerDir 'first-failure.json')){$result.Outcome='CONTENT_ORACLE_FAILURE';$result.FirstFailure=Get-Content (Join-Path $consumerDir 'first-failure.json') -Raw|ConvertFrom-Json}
  if($Classified -and (Test-Path (Join-Path $consumerDir 'classification-result.json'))){$result.Classifications=Get-Content (Join-Path $consumerDir 'classification-result.json') -Raw|ConvertFrom-Json;if($result.Classifications.E){$result.Outcome='UNCLASSIFIED'}elseif($result.Classifications.D){$result.Outcome='INTEGRITY_FAILURE'}}
  throw ('Host exit='+$exitCode+'; '+[IO.File]::ReadAllText((Join-Path $consumerDir 'stderr.txt')))
 }
 if($Reconnect){
  if(!$Classified){throw 'Reconnect verification here requires classified mode'}
  $reconnectDir=Join-Path $dir 'reconnect';New-Item -ItemType Directory $reconnectDir|Out-Null
  $reconnectArgs='--output "'+$reconnectDir+'" --seconds 15 --classified --nonce '+$nonce
  $reconnectArgs+=$encodeArgs
  $consumer=Start-Process -FilePath $hostExe -ArgumentList $reconnectArgs -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $reconnectDir 'stdout.txt') -RedirectStandardError (Join-Path $reconnectDir 'stderr.txt')
  $reconnectHandle=$consumer.Handle;Log 'Start fresh-process classified reconnect' @{Executable=$hostExe;Arguments=$reconnectArgs;ProcessId=$consumer.Id}
  if(!$consumer.WaitForExit(25000)){throw 'Reconnect exceeded deadline'}
  [uint32]$reconnectExit=0;if(![SweetDisplayDiagNative]::GetExitCodeProcess($reconnectHandle,[ref]$reconnectExit)){throw 'Reconnect exit query failed'}
  $result.ReconnectExitCode=$reconnectExit
  Log 'Reconnect completed' @{ExitCode=$reconnectExit;Stdout=[IO.File]::ReadAllText((Join-Path $reconnectDir 'stdout.txt'));Stderr=[IO.File]::ReadAllText((Join-Path $reconnectDir 'stderr.txt'))}
  if($reconnectExit -ne 0){throw ('Reconnect failed: '+[IO.File]::ReadAllText((Join-Path $reconnectDir 'stderr.txt')))}
 }
 $result.Outcome=if($Classified){'PASS_CLASSIFIED'}else{'NO_ANOMALY_REPRODUCED'}
}catch{if($result.Outcome -notin @('CONTENT_ORACLE_FAILURE','UNCLASSIFIED','INTEGRITY_FAILURE')){$result.Outcome='ERROR'};$result.Error=$_.Exception.Message}
finally {
 if($consumer -and !$consumer.HasExited){$consumer.Kill();$result.HostForcedStop=$true}
 if($pattern -and !$pattern.HasExited){
  # Process.MainWindowHandle can select our overlay instead of the pattern.
  # Target the recorded producer HWND only after checking its owning PID.
  $generation=Join-Path $consumerDir 'resource-generation.txt'
  $closed=$false
  if(Test-Path $generation){
   $identity=[IO.File]::ReadAllText($generation)
   if($identity -match 'pattern_hwnd=(\d+)'){
    $patternWindow=[IntPtr]::new([long]$Matches[1]);[uint32]$owner=0
    [SweetDisplayDiagNative]::GetWindowThreadProcessId($patternWindow,[ref]$owner)|Out-Null
    if($owner -eq $pattern.Id){$closed=[SweetDisplayDiagNative]::PostMessage($patternWindow,0x10,[IntPtr]::Zero,[IntPtr]::Zero)}
   }
  }
  if(!$closed){$closed=$pattern.CloseMainWindow()}
  $result.PatternCloseRequested=$closed
  if(!$pattern.WaitForExit(5000)){$pattern.Kill();$result.PatternForcedStop=$true;$result.Outcome='ERROR';$result.CleanupCheckError='Own pattern required forced termination'}
 }
 try{
  if($pattern -and $pattern.HasExited){
   [uint32]$patternExit=0
   if(![SweetDisplayDiagNative]::GetExitCodeProcess($patternHandle,[ref]$patternExit)){$result.Outcome='ERROR';$result.CleanupCheckError='Pattern exit query failed'}
   $result.PatternExitCode=$patternExit
   Log 'Close own producer window and wait for process exit' @{CloseRequested=$result.PatternCloseRequested;Forced=[bool]$result.PatternForcedStop;ExitCode=$patternExit}
   if($patternExit -ne 0){$result.Outcome='ERROR';$result.CleanupCheckError='Pattern did not exit cleanly'}
  }
  $after=Health 'after';Log 'Read-only post-test health' $after
  if($before -and ($before.Boot -ne $after.Boot -or $before.CiFlags -ne $after.CiFlags)){throw 'Boot/security baseline changed'}
  if($driverPid){Get-Process -Id $driverPid|Select-Object Id,HandleCount,PrivateMemorySize64,WorkingSet64|ConvertTo-Json|Set-Content (Join-Path $dir 'after-driver-resources.json') -Encoding utf8}
  $from=([datetime]$result.StartedUtc).ToLocalTime()
  $events=@(Events @{LogName='Application';StartTime=$from;Id=1000,1001}|Where-Object {$_.Message -match 'WUDFHost|SweetDisplayDriver|SweetDisplayGpuPattern|SweetDisplayHost'})
  $events+=@(Events @{LogName='System';StartTime=$from;Id=10110,10111,41,1001,4101})
  $result.FailureEventCount=$events.Count
  ConvertTo-Json -InputObject @($events|Select-Object TimeCreated,Id,ProviderName,Message) -Depth 4|Set-Content (Join-Path $dir 'failure-events.json') -Encoding utf8
  if($events.Count){throw 'Relevant failure events recorded'}
 }catch{$result.CleanupCheckError=$_.Exception.Message;$result.Outcome='ERROR'}
 $result.CompletedUtc=[datetime]::UtcNow.ToString('o');Save
}
if($result.Outcome -notin @('NO_ANOMALY_REPRODUCED','PASS_CLASSIFIED')){throw ($result.Outcome+': '+$result.Error)}
Write-Output ($result.Outcome+': '+$RunName+'; historical soak remains UNKNOWN')
