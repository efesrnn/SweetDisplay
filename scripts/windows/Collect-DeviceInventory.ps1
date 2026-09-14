#requires -Version 7.4
param(
    [ValidateSet('Android', 'Fastboot')][string]$Mode = 'Android',
    [string]$AdbPath = 'adb.exe',
    [string]$FastbootPath = 'fastboot',
    [string]$Serial
)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$directory = Join-Path $root 'docs/evidence/private'
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$log = Join-Path $directory ('inventory-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '.jsonl')
$command = if ($Mode -eq 'Android') { $AdbPath } else { $FastbootPath }
$resolved = Get-Command $command -CommandType Application -ErrorAction SilentlyContinue
if (-not $resolved) { throw "Executable not found: $command. Use official Platform-Tools; see docs/HOST_ENVIRONMENT.md." }
$executable = $resolved.Source

function Invoke-ReadQuery([string[]]$Arguments) {
    $info = [System.Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $executable
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    foreach ($argument in $Arguments) { $info.ArgumentList.Add($argument) }
    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $info
    $started = [DateTimeOffset]::UtcNow.ToString('o')
    try {
        [void]$process.Start()
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        $timeout = -not $process.WaitForExit(10000)
        if ($timeout) { $process.Kill(); $process.WaitForExit() }
        $result = [pscustomobject]@{
            utc = $started; executable = $executable; arguments = $Arguments
            exit_code = $process.ExitCode; timed_out = $timeout
            stdout = $stdout.GetAwaiter().GetResult(); stderr = $stderr.GetAwaiter().GetResult()
        }
        $result | ConvertTo-Json -Compress -Depth 4 | Add-Content -LiteralPath $log -Encoding utf8
        return $result
    } finally { $process.Dispose() }
}

$versionArgs = if ($Mode -eq 'Android') { @('version') } else { @('--version') }
$null = Invoke-ReadQuery $versionArgs
$listing = Invoke-ReadQuery @('devices')
if ($listing.exit_code -ne 0 -or $listing.timed_out) { throw "Device listing failed; see local log $log" }
$available = @()
foreach ($line in ($listing.stdout -split '\r?\n')) {
    if ($Mode -eq 'Android' -and $line -match '^(\S+)\s+device$') { $available += $Matches[1] }
    if ($Mode -eq 'Fastboot' -and $line -match '^(\S+)\s+fastboot$') { $available += $Matches[1] }
}
if (-not $Serial) {
    if ($available.Count -ne 1) {
        Write-Host "No unique ready $Mode device ($($available.Count) found). No shell/getvar queries sent. Log: $log"
        exit 2
    }
    $Serial = $available[0]
} elseif ($Serial -notin $available) { throw "Requested device is not ready in $Mode mode. No queries sent. Log: $log" }

if ($Mode -eq 'Fastboot') {
    foreach ($variable in @('product','current-slot','slot-count','has-slot:boot','unlocked','secure','all')) {
        $null = Invoke-ReadQuery @('-s', $Serial, 'getvar', $variable)
    }
} else {
    $properties = @(
        'ro.product.device','ro.product.model','ro.product.manufacturer','ro.product.name',
        'ro.build.fingerprint','ro.build.version.release','ro.build.version.sdk',
        'ro.build.version.incremental','ro.miui.ui.version.name','ro.boot.slot_suffix',
        'ro.boot.flash.locked','ro.boot.verifiedbootstate','ro.boot.vbmeta.device_state',
        'ro.build.ab_update','ro.boot.dynamic_partitions','ro.boot.hardware'
    )
    foreach ($property in $properties) { $null = Invoke-ReadQuery @('-s', $Serial, 'shell', 'getprop', $property) }
    foreach ($query in @(
        'uname -a','cat /proc/version','cat /proc/cmdline','cat /proc/bus/input/devices',
        'ls -l /sys/class/drm','ls -l /dev/dri','ls -l /dev/fb*',
        'ls -l /dev/video*','ls -l /dev/media*','ls -l /sys/class/udc',
        'ls -l /vendor/lib/modules','ls -l /vendor/firmware',
        'ls -l /vendor/etc/camera','ls -l /vendor/lib64/hw',
        'ls -l /dev/block/by-name'
    )) { $null = Invoke-ReadQuery @('-s', $Serial, 'shell', $query) }
}
Write-Host "Read-only inventory saved: $log. Review each command status; missing/denied/unsupported values remain unknown."

