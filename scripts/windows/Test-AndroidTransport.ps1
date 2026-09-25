#requires -Version 7.4
#requires -RunAsAdministrator
<#+
Runs the existing classified Display 3 -> hardware H.264 -> SweetDisplay sender
against the ordinary Android receiver through an ephemeral adb forward.

This controller never changes Windows security policy or the Android USB gadget.
Runtime evidence is written only below the ignored private evidence directory.
#>
param(
    [Parameter(Mandatory = $true)][string]$RunName,
    [ValidateRange(20, 600)][int]$Seconds = 30,
    [ValidateSet(800, 1280, 1920, 2400)][int]$Width = 1280,
    [ValidateSet(360, 576, 864, 1080)][int]$Height = 576,
    [ValidateSet(10, 30, 60)][int]$Rate = 30,
    [ValidateRange(100000, 100000000)][int]$Bitrate = 8000000,
    [ValidateRange(0, 590)][int]$RestartAtSeconds = 0,
    [ValidateRange(1024, 65535)][int]$Port = 48231,
    [ValidateSet('none', 'validate', 'inject')][string]$TouchMode = 'none',
    [string]$AdbPath = (Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe')
)
$ErrorActionPreference = 'Stop'
[Threading.Thread]::CurrentThread.CurrentCulture = [Globalization.CultureInfo]::InvariantCulture

if ($RunName -notmatch '^[a-zA-Z0-9-]+$') { throw 'Invalid run name' }
if ($RestartAtSeconds -and ($RestartAtSeconds -lt 10 -or $RestartAtSeconds -gt ($Seconds - 10))) {
    throw 'Receiver restart must leave at least ten seconds before and after restart'
}
if (!(Test-Path -LiteralPath $AdbPath -PathType Leaf)) { throw 'ADB executable not found' }

$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$privateRoot = Join-Path $repo ('docs\evidence\private\' +
        $(if ($TouchMode -eq 'none') { 'device-phase2c0' } else { 'device-phase2ct' }))
$run = Join-Path $privateRoot $RunName
if (Test-Path -LiteralPath $run) { throw 'Preserve evidence: use a new run name' }
New-Item -ItemType Directory -Path $privateRoot -Force | Out-Null
New-Item -ItemType Directory -Path $run | Out-Null
$hostDir = Join-Path $run 'host'
New-Item -ItemType Directory -Path $hostDir | Out-Null

$result = [ordered]@{
    Outcome = 'RUNNING'
    StartedUtc = [datetime]::UtcNow.ToString('o')
    RequestedSeconds = $Seconds
    Encode = @{ Width = $Width; Height = $Height; Rate = $Rate; Bitrate = $Bitrate }
    Port = $Port
    ReceiverRestartAtSeconds = $RestartAtSeconds
    ReceiverRestarted = $false
    TouchMode = $TouchMode
    UsbBefore = $null
    UsbAfter = $null
    HostExitCode = $null
    PatternExitCode = $null
    Error = $null
}
function Save-Result {
    $result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $run 'controller.json') -Encoding utf8
}
function Invoke-Adb([Parameter(ValueFromRemainingArguments = $true)][string[]]$Arguments) {
    $text = & $AdbPath @Arguments 2>&1
    if ($LASTEXITCODE -ne 0) { throw ('adb failed: ' + ($text -join ' ')) }
    return @($text)
}
function Get-Health([string]$Name) {
    $devices = @(Get-PnpDevice -Class Display, Monitor -PresentOnly | Where-Object {
        $_.FriendlyName -like '*SweetDisplay*' -or $_.InstanceId -match 'SweetDisplay|SWT0001'
    } | ForEach-Object {
        [pscustomobject]@{
            Class = $_.Class
            Name = $_.FriendlyName
            Status = $_.Status
            InstanceId = $_.InstanceId
            Problem = (Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_ProblemCode).Data
        }
    })
    $health = [ordered]@{
        Utc = [datetime]::UtcNow.ToString('o')
        SecureBoot = Confirm-SecureBootUEFI
        Hvci = Get-ItemPropertyValue 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\State' HVCIEnabled
        Boot = (Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime().ToString('o')
        Devices = $devices
    }
    $health | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run ($Name + '-health.json')) -Encoding utf8
    if (!$health.SecureBoot -or $health.Hvci -ne 1 -or $devices.Count -ne 2 -or
        @($devices | Where-Object { $_.Problem -ne 0 -or $_.Status -ne 'OK' }).Count) {
        throw 'Read-only Windows security/PnP health preflight failed'
    }
    return $health
}

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class SweetDisplayAndroidTestNative {
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
 [DllImport("user32.dll", SetLastError=true)] [return:MarshalAs(UnmanagedType.Bool)]
 public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
}
'@

$pattern = $null
$hostProcess = $null
$patternHandle = [IntPtr]::Zero
$before = $null
$forwardCreated = $false
try {
    Save-Result
    $before = Get-Health 'before'
    if ((Invoke-Adb get-state | Select-Object -First 1) -ne 'device') { throw 'ADB device is not ready' }
    $result.UsbBefore = (Invoke-Adb shell getprop sys.usb.config | Select-Object -First 1).Trim()
    if ($result.UsbBefore -ne 'mtp,adb') { throw ('Unexpected USB composition: ' + $result.UsbBefore) }
    if (!(Invoke-Adb shell pm list packages com.sweetdisplay.receiver | Select-String 'package:com.sweetdisplay.receiver')) {
        throw 'Android receiver is not installed'
    }

    Invoke-Adb forward ('tcp:' + $Port) ('tcp:' + $Port) | Out-Null
    $forwardCreated = $true
    Invoke-Adb logcat -c | Out-Null
    Invoke-Adb shell am start -n com.sweetdisplay.receiver/.MainActivity | Out-Null
    Start-Sleep -Seconds 2

    $nonce = '{0:X8}' -f (Get-Random -Minimum 1 -Maximum ([int]::MaxValue))
    $result.Nonce = $nonce
    $patternExe = Join-Path $repo 'out\gpu-pattern\SweetDisplayGpuPattern.exe'
    $patternArgs = '"' + $run + '" ' + $nonce + ' ' + ($Seconds + 90) +
        ' 0 1 ' + $(if ($TouchMode -eq 'none') { 'observe' } else { 'touch' })
    $pattern = Start-Process -FilePath $patternExe -ArgumentList $patternArgs -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $run 'pattern-stdout.txt') `
        -RedirectStandardError (Join-Path $run 'pattern-stderr.txt')
    $patternHandle = $pattern.Handle
    Start-Sleep -Seconds 2
    if ($pattern.HasExited) { throw 'Pattern failed during startup' }

    $hostExe = Join-Path $repo $(if ($TouchMode -eq 'none') {
        'out\transport-host-content-v2\SweetDisplayHost.exe'
    } else { 'out\transport-host\SweetDisplayHost.exe' })
    $hostArgs = '--output "' + $hostDir + '" --seconds ' + $Seconds +
        ' --first-fail --classified --nonce ' + $nonce +
        ' --encode --encode-width ' + $Width + ' --encode-height ' + $Height +
        ' --encode-fps ' + $Rate + ' --encode-bitrate ' + $Bitrate +
        ' --encode-uncapped --transport-port ' + $Port
    if ($TouchMode -ne 'none') { $hostArgs += ' --touch-mode ' + $TouchMode }
    $hostProcess = Start-Process -FilePath $hostExe -ArgumentList $hostArgs -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $hostDir 'stdout.txt') `
        -RedirectStandardError (Join-Path $hostDir 'stderr.txt')
    $hostHandle = $hostProcess.Handle
    $started = [datetime]::UtcNow
    $deadline = $started.AddSeconds($Seconds + 60)
    $nextSample = $started
    $restartDone = $false
    while (!$hostProcess.WaitForExit(100)) {
        $elapsed = ([datetime]::UtcNow - $started).TotalSeconds
        if ($pattern.HasExited) { throw 'Pattern exited while Host was active' }
        if ([datetime]::UtcNow -gt $deadline) { throw 'Host exceeded bounded deadline' }
        if ($RestartAtSeconds -and !$restartDone -and $elapsed -ge $RestartAtSeconds) {
            Invoke-Adb shell am force-stop com.sweetdisplay.receiver | Out-Null
            $result.ReceiverStopUtc = [datetime]::UtcNow.ToString('o')
            if ($hostProcess.HasExited) { throw 'Host exited during controlled receiver stop' }
            Start-Sleep -Seconds 3
            Invoke-Adb shell am start -n com.sweetdisplay.receiver/.MainActivity | Out-Null
            $result.ReceiverStartUtc = [datetime]::UtcNow.ToString('o')
            $result.ReceiverRestarted = $true
            $restartDone = $true
            Save-Result
        }
        if ([datetime]::UtcNow -ge $nextSample) {
            foreach ($entry in @(@{ Role = 'Host'; Process = $hostProcess }, @{ Role = 'Pattern'; Process = $pattern })) {
                $process = Get-Process -Id $entry.Process.Id -ErrorAction SilentlyContinue
                if ($process) {
                    [pscustomobject]@{
                        ElapsedSeconds = $elapsed
                        Role = $entry.Role
                        ProcessId = $process.Id
                        CpuSeconds = $process.TotalProcessorTime.TotalSeconds
                        PrivateBytes = $process.PrivateMemorySize64
                        WorkingSetBytes = $process.WorkingSet64
                        Handles = $process.HandleCount
                    } | Export-Csv -LiteralPath (Join-Path $run 'windows-resources.csv') -Append -NoTypeInformation -Encoding utf8
                }
            }
            $nextSample = [datetime]::UtcNow.AddSeconds(5)
        }
    }
    $result.HostExitCode = $hostProcess.ExitCode
    if ($hostProcess.ExitCode -ne 0) {
        throw ('Host failed: ' + [IO.File]::ReadAllText((Join-Path $hostDir 'stderr.txt')))
    }
    # The pattern owns the independent WM_POINTER CSV.  Close it before parsing
    # that CSV; otherwise its exclusive writer handle turns a completed inject
    # run into a false-negative controller result.
    if ($pattern -and !$pattern.HasExited) {
        $closed = $false
        $generation = Join-Path $hostDir 'resource-generation.txt'
        if (Test-Path -LiteralPath $generation) {
            $identity = [IO.File]::ReadAllText($generation)
            if ($identity -match 'pattern_hwnd=(\d+)') {
                $window = [IntPtr]::new([long]$Matches[1])
                [uint32]$owner = 0
                [SweetDisplayAndroidTestNative]::GetWindowThreadProcessId($window, [ref]$owner) | Out-Null
                if ($owner -eq $pattern.Id) {
                    $closed = [SweetDisplayAndroidTestNative]::PostMessage(
                        $window, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
                }
            }
        }
        if (!$closed) { $closed = $pattern.CloseMainWindow() }
        if (!$pattern.WaitForExit(5000)) { throw 'Pattern did not close before touch-event validation' }
    }
    if ($pattern -and $pattern.HasExited) {
        $result.PatternExitCode = $pattern.ExitCode
        if ($pattern.ExitCode -ne 0) { throw ('Pattern failed: exit code ' + $pattern.ExitCode) }
    }

    foreach ($required in @('session-1-result.json', 'encode-result.json', 'transport-result.json', 'classification-result.json')) {
        if (!(Test-Path -LiteralPath (Join-Path $hostDir $required))) { throw ('Missing Host result: ' + $required) }
    }
    $source = Get-Content -Raw -LiteralPath (Join-Path $hostDir 'session-1-result.json') | ConvertFrom-Json
    $encode = Get-Content -Raw -LiteralPath (Join-Path $hostDir 'encode-result.json') | ConvertFrom-Json
    $transport = Get-Content -Raw -LiteralPath (Join-Path $hostDir 'transport-result.json') | ConvertFrom-Json
    $classification = Get-Content -Raw -LiteralPath (Join-Path $hostDir 'classification-result.json') | ConvertFrom-Json
    if (!$source.clean_disconnect -or !$encode.hardware_only -or $encode.software_fallback -or !$encode.drained) {
        throw 'Source/encoder acceptance invariant failed'
    }
    if ($transport.protocol_errors -ne 0 -or $transport.pending -ne 0 -or $transport.queue_peak -gt 3 -or $transport.acked -lt 1) {
        throw 'Transport acceptance invariant failed'
    }
    if ($TouchMode -ne 'none' -and ($transport.touch_messages -lt 1 -or
            !(Test-Path -LiteralPath (Join-Path $hostDir 'touch-events.csv')))) {
        throw 'No validated real touch messages'
    }
    if ($TouchMode -eq 'inject') {
        $targetEvents = Join-Path $run 'touch-target-events.csv'
        if (!(Test-Path -LiteralPath $targetEvents) -or
                @(Import-Csv -LiteralPath $targetEvents).Count -lt 1) {
            throw 'Injected touch was not independently observed by the Display 3 target'
        }
    }
    if ($classification.B -or $classification.C -or $classification.D -or $classification.E) {
        throw 'Content classification failed'
    }
    if ($RestartAtSeconds -and (!$result.ReceiverRestarted -or $transport.connections -lt 2)) {
        throw 'Controlled receiver reconnect did not establish a new transport session'
    }

    $result.Source = $source
    $result.Encoder = $encode
    $result.Transport = $transport
    $result.Classification = $classification
    $result.Outcome = 'PASS_COUNTERS_PENDING_VISUAL_CONFIRMATION'
}
catch {
    $result.Outcome = 'ERROR'
    $result.Error = $_.Exception.Message
}
finally {
    if ($hostProcess -and !$hostProcess.HasExited) {
        $hostProcess.Kill()
        $result.HostForcedStop = $true
    }
    try {
        Invoke-Adb -Arguments @('logcat', '-d', '-v', 'threadtime', '-s', 'SWDPRX:I', '*:S') |
            Set-Content -LiteralPath (Join-Path $run 'android-swdprx.log') -Encoding utf8
        $result.UsbAfter = (Invoke-Adb shell getprop sys.usb.config | Select-Object -First 1).Trim()
        if ($result.UsbAfter -ne 'mtp,adb') { throw ('USB composition changed: ' + $result.UsbAfter) }
    }
    catch {
        $result.Outcome = 'ERROR'
        $result.CleanupError = $_.Exception.Message
    }
    if ($forwardCreated) {
        try { Invoke-Adb forward --remove ('tcp:' + $Port) | Out-Null }
        catch { $result.ForwardCleanupError = $_.Exception.Message; $result.Outcome = 'ERROR' }
    }
    if ($pattern -and !$pattern.HasExited) {
        $closed = $false
        $generation = Join-Path $hostDir 'resource-generation.txt'
        if (Test-Path -LiteralPath $generation) {
            $identity = [IO.File]::ReadAllText($generation)
            if ($identity -match 'pattern_hwnd=(\d+)') {
                $window = [IntPtr]::new([long]$Matches[1])
                [uint32]$owner = 0
                [SweetDisplayAndroidTestNative]::GetWindowThreadProcessId($window, [ref]$owner) | Out-Null
                if ($owner -eq $pattern.Id) {
                    $closed = [SweetDisplayAndroidTestNative]::PostMessage(
                        $window, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
                }
            }
        }
        if (!$closed) { $closed = $pattern.CloseMainWindow() }
        if (!$pattern.WaitForExit(5000)) {
            $pattern.Kill()
            $result.PatternForcedStop = $true
            $result.Outcome = 'ERROR'
        }
    }
    if ($pattern -and $pattern.HasExited) { $result.PatternExitCode = $pattern.ExitCode }
    try {
        $after = Get-Health 'after'
        if ($before -and ($before.Boot -ne $after.Boot -or $before.Hvci -ne $after.Hvci)) {
            throw 'Windows boot/security state changed during test'
        }
    }
    catch {
        $result.HealthCleanupError = $_.Exception.Message
        $result.Outcome = 'ERROR'
    }
    $result.CompletedUtc = [datetime]::UtcNow.ToString('o')
    Save-Result
}

if ($result.Outcome -ne 'PASS_COUNTERS_PENDING_VISUAL_CONFIRMATION') {
    throw ($result.Outcome + ': ' + $result.Error + ' ' + $result.CleanupError + ' ' + $result.HealthCleanupError)
}
Write-Output ($result.Outcome + ': ' + $RunName)
