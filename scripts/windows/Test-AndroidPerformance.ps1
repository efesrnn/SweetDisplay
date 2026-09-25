#requires -Version 7.4
#requires -RunAsAdministrator
<# DEVICE PHASE 2C-PERF baseline collector.
Fixed 2400x1080/60/30 Mbps hardware path, ordinary ADB forward, no touch.
The controller records bounded private telemetry only and never changes USB or
Windows security policy. Create abort.request in the run directory for a clean
operator abort. #>
param(
    [Parameter(Mandatory = $true)][string]$RunName,
    [ValidateRange(60, 1200)][int]$Seconds = 900,
    [switch]$ShortControl,
    [ValidateRange(1024, 65535)][int]$Port = 48231,
    [string]$AdbPath = (Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe')
)
$ErrorActionPreference = 'Stop'
[Threading.Thread]::CurrentThread.CurrentCulture = [Globalization.CultureInfo]::InvariantCulture
if ($RunName -notmatch '^[a-zA-Z0-9-]+$') { throw 'Invalid run name' }
if ($Seconds -lt 900 -and !$ShortControl) { throw 'Runs below 900 seconds require explicit -ShortControl and do not satisfy the long-baseline gate' }
if (!(Test-Path -LiteralPath $AdbPath -PathType Leaf)) { throw 'ADB executable not found' }
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$run = Join-Path $repo ('docs\evidence\private\device-phase2cperf\' + $RunName)
if (Test-Path -LiteralPath $run) { throw 'Preserve evidence: use a new run name' }
$hostDir = Join-Path $run 'host'
New-Item -ItemType Directory -Path $hostDir -Force | Out-Null
$abortFile = Join-Path $run 'abort.request'
$stopFile = Join-Path $run 'normal-host-stop.request'
$result = [ordered]@{
    Outcome='RUNNING';StartedUtc=[datetime]::UtcNow.ToString('o');RequestedSeconds=$Seconds;ShortControl=[bool]$ShortControl
    Width=2400;Height=1080;Rate=60;Bitrate=30000000;Port=$Port
    UsbBefore=$null;UsbAfter=$null;HostExitCode=$null;PatternExitCode=$null
    OperatorAbortFile=$abortFile;OperatorAborted=$false;Error=$null
}
function Save-Result { $result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $run 'controller.json') -Encoding utf8 }
function Invoke-Adb([Parameter(ValueFromRemainingArguments=$true)][string[]]$Arguments) { $text=& $AdbPath @Arguments 2>&1;if($LASTEXITCODE-ne0){throw('adb failed: '+($text-join' '))};return @($text) }
function Get-Health([string]$Name) {
    $devices=@(Get-PnpDevice -Class Display,Monitor -PresentOnly|Where-Object {$_.FriendlyName-like'*SweetDisplay*'-or$_.InstanceId-match'SweetDisplay|SWT0001'}|ForEach-Object{[pscustomobject]@{Class=$_.Class;Status=$_.Status;Problem=(Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_ProblemCode).Data}})
    $health=[ordered]@{Utc=[datetime]::UtcNow.ToString('o');SecureBoot=Confirm-SecureBootUEFI;Hvci=Get-ItemPropertyValue 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\State' HVCIEnabled;Boot=(Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime().ToString('o');Devices=$devices}
    $health|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $run ($Name+'-health.json')) -Encoding utf8
    if(!$health.SecureBoot-or$health.Hvci-ne1-or$devices.Count-ne2-or@($devices|Where-Object{$_.Status-ne'OK'-or$_.Problem-ne0}).Count){throw 'Windows security/PnP health failed'}
    return $health
}
function Add-ResourceSample([string]$Role,[int]$Id,[double]$Elapsed) {
    $p=Get-Process -Id $Id -ErrorAction SilentlyContinue;if(!$p){return}
    [pscustomobject]@{ControllerUtc=[datetime]::UtcNow.ToString('o');ControllerTick=[Diagnostics.Stopwatch]::GetTimestamp();ElapsedSeconds=$Elapsed;Role=$Role;ProcessId=$Id;CpuSeconds=$p.TotalProcessorTime.TotalSeconds;PrivateBytes=$p.PrivateMemorySize64;WorkingSetBytes=$p.WorkingSet64;Handles=$p.HandleCount;Threads=$p.Threads.Count}|Export-Csv -LiteralPath (Join-Path $run 'windows-resources.csv') -Append -NoTypeInformation -Encoding utf8
}
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class SweetDisplayPerfNative {
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h,out uint pid);
 [DllImport("user32.dll",SetLastError=true)] [return:MarshalAs(UnmanagedType.Bool)] public static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
}
'@
$pattern=$null;$hostProcess=$null;$before=$null;$forwardCreated=$false;$driverPid=0;$helperPid=0
try {
    Save-Result;$before=Get-Health 'before'
    $drive=Get-PSDrive -Name ([IO.Path]::GetPathRoot($repo).Substring(0,1));if($drive.Free-lt10GB){throw 'At least 10 GiB free space is required for bounded 15-minute payload evidence'}
    $drivers=@(Get-CimInstance Win32_Process -Filter "Name='WUDFHost.exe'"|Where-Object{$_.CommandLine-match'SweetDisplayDriverGroup'});if($drivers.Count-ne1){throw 'Unique SweetDisplay UMDF host required'};$driverPid=[int]$drivers[0].ProcessId
    $helpers=@(Get-Process SweetDisplayDevice -ErrorAction Stop);if($helpers.Count-ne1){throw 'Unique SweetDisplayDevice helper required'};$helperPid=$helpers[0].Id
    if((Invoke-Adb get-state|Select-Object -First 1)-ne'device'){throw 'ADB device is not ready'}
    $result.UsbBefore=(Invoke-Adb shell getprop sys.usb.config|Select-Object -First 1).Trim();if($result.UsbBefore-ne'mtp,adb'){throw 'Unexpected USB composition'}
    if(!(Invoke-Adb shell pm list packages com.sweetdisplay.receiver|Select-String 'package:com.sweetdisplay.receiver')){throw 'Android receiver is not installed'}
    Invoke-Adb forward ('tcp:'+$Port) ('tcp:'+$Port)|Out-Null;$forwardCreated=$true
    Invoke-Adb shell am force-stop com.sweetdisplay.receiver|Out-Null
    Invoke-Adb logcat -c|Out-Null
    Invoke-Adb shell am start -n com.sweetdisplay.receiver/.MainActivity|Out-Null
    Start-Sleep -Seconds 2
    $nonce='{0:X8}'-f(Get-Random -Minimum 1 -Maximum ([int]::MaxValue));$result.Nonce=$nonce
    $pattern=Start-Process -FilePath (Join-Path $repo 'out\gpu-pattern\SweetDisplayGpuPattern.exe') -ArgumentList ('"'+$run+'" '+$nonce+' '+($Seconds+90)+' 0 1 observe') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'pattern-stdout.txt') -RedirectStandardError (Join-Path $run 'pattern-stderr.txt')
    Start-Sleep -Seconds 2;if($pattern.HasExited){throw 'Pattern failed during startup'}
    $hostArgs='--output "'+$hostDir+'" --seconds '+$Seconds+' --stop-file "'+$stopFile+'" --first-fail --classified --nonce '+$nonce+' --encode --encode-width 2400 --encode-height 1080 --encode-fps 60 --encode-bitrate 30000000 --encode-uncapped --transport-port '+$Port
    $hostProcess=Start-Process -FilePath (Join-Path $repo 'out\transport-host-content-v2\SweetDisplayHost.exe') -ArgumentList $hostArgs -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $hostDir 'stdout.txt') -RedirectStandardError (Join-Path $hostDir 'stderr.txt')
    $started=[datetime]::UtcNow;$deadline=$started.AddSeconds($Seconds+90);$nextSample=$started;$nextGpu=$started;$nextHealth=$started.AddSeconds(300);$nextProgress=60;$healthIndex=0
    Write-Host ('PERF '+$(if($ShortControl){'short control'}else{'baseline'})+' active. Safe abort file: '+$abortFile) -ForegroundColor Green
    while(!$hostProcess.WaitForExit(100)) {
        $elapsed=([datetime]::UtcNow-$started).TotalSeconds
        if($pattern.HasExited){throw 'Pattern exited while Host was active'}
        if([datetime]::UtcNow-gt$deadline){throw 'Host exceeded bounded deadline'}
        if(Test-Path -LiteralPath $abortFile){$result.OperatorAborted=$true;if(!(Test-Path -LiteralPath $stopFile)){'operator-abort'|Set-Content -LiteralPath $stopFile -Encoding ascii}}
        if($elapsed-ge$nextProgress){Write-Host ('PERF elapsed: '+[int]$elapsed+' / '+$Seconds+' seconds') -ForegroundColor Cyan;$nextProgress+=60}
        if([datetime]::UtcNow-ge$nextSample){
            Add-ResourceSample 'Host' $hostProcess.Id $elapsed;Add-ResourceSample 'Pattern' $pattern.Id $elapsed;Add-ResourceSample 'Driver' $driverPid $elapsed;Add-ResourceSample 'Helper' $helperPid $elapsed
            [pscustomobject]@{ControllerUtc=[datetime]::UtcNow.ToString('o');ControllerTick=[Diagnostics.Stopwatch]::GetTimestamp();ElapsedSeconds=$elapsed}|Export-Csv -LiteralPath (Join-Path $run 'controller-epochs.csv') -Append -NoTypeInformation -Encoding utf8
            $nextSample=[datetime]::UtcNow.AddSeconds(1)
        }
        if([datetime]::UtcNow-ge$nextGpu){
            try{Get-CimInstance -ClassName Win32_PerfFormattedData_GPUPerformanceCounters_GPUEngine -Filter ("Name LIKE '%pid_"+$hostProcess.Id+"_%'")|Where-Object UtilizationPercentage -gt 0|Select-Object @{Name='ElapsedSeconds';Expression={$elapsed}},Name,UtilizationPercentage|Export-Csv -LiteralPath (Join-Path $run 'gpu-engines.csv') -Append -NoTypeInformation -Encoding utf8}catch{Add-Content -LiteralPath (Join-Path $run 'gpu-counter-errors.txt') -Value $_.Exception.Message}
            $nextGpu=[datetime]::UtcNow.AddSeconds(2)
        }
        if([datetime]::UtcNow-ge$nextHealth){$healthIndex++;Get-Health ('periodic-'+$healthIndex)|Out-Null;$nextHealth=[datetime]::UtcNow.AddSeconds(300)}
    }
    $result.HostExitCode=$hostProcess.ExitCode;if($hostProcess.ExitCode-ne0){throw('Host failed: '+[IO.File]::ReadAllText((Join-Path $hostDir 'stderr.txt')))}
    if($result.OperatorAborted){$result.Outcome='ABORTED'}else{
        foreach($required in @('session-1-result.json','encode-result.json','transport-result.json','classification-result.json')){if(!(Test-Path -LiteralPath (Join-Path $hostDir $required))){throw('Missing Host result: '+$required)}}
        $source=Get-Content -Raw (Join-Path $hostDir 'session-1-result.json')|ConvertFrom-Json;$encode=Get-Content -Raw (Join-Path $hostDir 'encode-result.json')|ConvertFrom-Json;$transport=Get-Content -Raw (Join-Path $hostDir 'transport-result.json')|ConvertFrom-Json;$classification=Get-Content -Raw (Join-Path $hostDir 'classification-result.json')|ConvertFrom-Json
        if(!$source.clean_disconnect-or!$encode.hardware_only-or$encode.software_fallback-or!$encode.drained-or$transport.protocol_errors-ne0-or$transport.pending-ne0-or$transport.queue_peak-gt3-or$classification.B-or$classification.C-or$classification.D-or$classification.E){throw 'Baseline invariant failed'}
        $result.Source=$source;$result.Encoder=$encode;$result.Transport=$transport;$result.Classification=$classification;$result.Outcome='PASS_COUNTERS_PENDING_ANALYSIS'
    }
} catch {$result.Outcome='ERROR';$result.Error=$_.Exception.Message}
finally {
    if($hostProcess-and!$hostProcess.HasExited){if(!(Test-Path -LiteralPath $stopFile)){'cleanup-stop'|Set-Content -LiteralPath $stopFile -Encoding ascii};if(!$hostProcess.WaitForExit(15000)){$hostProcess.Kill();$result.HostForcedStop=$true}}
    try{Invoke-Adb -Arguments @('logcat','-d','-v','epoch','-s','SWDPRX:I','*:S')|Set-Content -LiteralPath (Join-Path $run 'android-swdprx.log') -Encoding utf8;$result.UsbAfter=(Invoke-Adb shell getprop sys.usb.config|Select-Object -First 1).Trim();if($result.UsbAfter-ne'mtp,adb'){throw 'USB composition changed'}}catch{$result.CleanupError=$_.Exception.Message;$result.Outcome='ERROR'}
    if($forwardCreated){try{Invoke-Adb forward --remove ('tcp:'+$Port)|Out-Null}catch{$result.ForwardCleanupError=$_.Exception.Message;$result.Outcome='ERROR'}}
    if($pattern-and!$pattern.HasExited){$closed=$false;$generation=Join-Path $hostDir 'resource-generation.txt';if(Test-Path $generation){$identity=[IO.File]::ReadAllText($generation);if($identity-match'pattern_hwnd=(\d+)'){$window=[IntPtr]::new([long]$Matches[1]);[uint32]$owner=0;[SweetDisplayPerfNative]::GetWindowThreadProcessId($window,[ref]$owner)|Out-Null;if($owner-eq$pattern.Id){$closed=[SweetDisplayPerfNative]::PostMessage($window,0x10,[IntPtr]::Zero,[IntPtr]::Zero)}}};if(!$closed){$closed=$pattern.CloseMainWindow()};if(!$pattern.WaitForExit(5000)){$pattern.Kill();$result.PatternForcedStop=$true;$result.Outcome='ERROR'}}
    if($pattern-and$pattern.HasExited){$result.PatternExitCode=$pattern.ExitCode;if($pattern.ExitCode-ne0){$result.Outcome='ERROR'}}
    try{$after=Get-Health 'after';if($before-and($before.Boot-ne$after.Boot-or$before.Hvci-ne$after.Hvci)){throw 'Windows boot/security changed'}}catch{$result.HealthCleanupError=$_.Exception.Message;$result.Outcome='ERROR'}
    $result.CompletedUtc=[datetime]::UtcNow.ToString('o');Save-Result
}
if($result.Outcome-ne'PASS_COUNTERS_PENDING_ANALYSIS'){throw($result.Outcome+': '+$result.Error+' '+$result.CleanupError+' '+$result.HealthCleanupError)}
Write-Output ('PASS_COUNTERS_PENDING_ANALYSIS: '+$RunName)
