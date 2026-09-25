#requires -Version 7.4
#requires -RunAsAdministrator
<# Windows-only release stress test.  It uses the production TouchInput::Session
state machine and an independent SweetDisplay target; no phone or ADB operation. #>
param(
    [Parameter(Mandatory = $true)][string]$RunName,
    [ValidateRange(1, 100)][int]$Iterations = 20
)
$ErrorActionPreference = 'Stop'
if ($RunName -notmatch '^[a-zA-Z0-9-]+$') { throw 'Invalid run name' }
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$run = Join-Path $repo ('docs\evidence\private\device-phase2ct1\' + $RunName)
if (Test-Path -LiteralPath $run) { throw 'Preserve evidence: use a new run name' }
New-Item -ItemType Directory -Path $run | Out-Null
$result = [ordered]@{ Outcome = 'RUNNING'; Iterations = $Iterations; ExpectedDowns = 11 * $Iterations; PatternExitCode = $null; HarnessExitCode = $null; Error = $null }
function Save-Result { $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run 'controller.json') -Encoding utf8 }
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class SweetDisplayTouchHarnessNative {
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
 [DllImport("user32.dll", SetLastError=true)] [return:MarshalAs(UnmanagedType.Bool)]
 public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
}
'@
$pattern = $null
try {
    Save-Result
    $pattern = Start-Process -FilePath (Join-Path $repo 'out\gpu-pattern\SweetDisplayGpuPattern.exe') `
        -ArgumentList ('"' + $run + '" 5445321 600 0 1 touch') -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $run 'pattern-stdout.txt') -RedirectStandardError (Join-Path $run 'pattern-stderr.txt')
    $ready = Join-Path $run 'touch-target-events.csv'
    $deadline = [datetime]::UtcNow.AddSeconds(15)
    while (!(Test-Path -LiteralPath $ready) -and [datetime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 100 }
    if (!(Test-Path -LiteralPath $ready) -or $pattern.HasExited) { throw 'Independent target did not become ready' }
    $harness = Start-Process -FilePath (Join-Path $repo 'out\touch-release-harness\TouchReleaseHarness.exe') `
        -ArgumentList ('"' + (Join-Path $run 'harness') + '" ' + $Iterations) -Wait -PassThru `
        -RedirectStandardOutput (Join-Path $run 'harness-stdout.txt') -RedirectStandardError (Join-Path $run 'harness-stderr.txt')
    $result.HarnessExitCode = $harness.ExitCode
    if ($harness.ExitCode -ne 0) { throw ('Harness failed: ' + [IO.File]::ReadAllText((Join-Path $run 'harness-stderr.txt'))) }
    # Wait on the independently observed terminal predicate, not a fixed delay.
    # InjectTouchInput delivery is asynchronous relative to the harness process.
    $targetEvents = @(); $active = $null; $downs = 0; $ups = 0
    $targetDeadline = [datetime]::UtcNow.AddSeconds(30)
    do {
        $targetEvents = @(Import-Csv -LiteralPath $ready)
        $active = [Collections.Generic.HashSet[string]]::new(); $downs = 0; $ups = 0; $invalidTargetState = $false
        foreach ($event in $targetEvents) {
            $id = [string]$event.pointer_id
            if ($event.event -eq 'DOWN') { if (!$active.Add($id)) { $invalidTargetState = $true }; $downs++ }
            elseif ($event.event -eq 'UP') { if (!$active.Remove($id)) { $invalidTargetState = $true }; $ups++ }
        }
        if (!$invalidTargetState -and $downs -eq $result.ExpectedDowns -and $ups -eq $result.ExpectedDowns -and !$active.Count) { break }
        if ($pattern.HasExited) { throw 'Independent target exited before terminal touch state' }
        Start-Sleep -Milliseconds 100
    } while ([datetime]::UtcNow -lt $targetDeadline)
    if ($invalidTargetState -or $downs -ne $result.ExpectedDowns -or $ups -ne $result.ExpectedDowns -or $active.Count) { throw 'Target did not observe exact terminal touch state before deadline' }
    if (!$pattern.CloseMainWindow() -or !$pattern.WaitForExit(5000)) { throw 'Independent target did not exit cleanly' }
    $result.PatternExitCode = $pattern.ExitCode
    if ($pattern.ExitCode -ne 0) { throw 'Independent target failed' }
    $api = @(Import-Csv -LiteralPath (Join-Path $run 'harness\touch-api.csv'))
    $unexpected = @($api | Where-Object { $_.success -eq '0' -and [uint32]$_.error -ne 21 })
    if ($unexpected.Count) { throw 'Unexpected InjectTouchInput failure; inspect private API ledger' }
    if (@(Get-Content -LiteralPath (Join-Path $run 'harness\touch-events.csv') | Select-String 'RELEASE_UNCERTAIN').Count) { throw 'Uncertain local release' }
    $result.TargetDowns = $downs; $result.TargetUps = $ups; $result.ApiRetries = @($api | Where-Object { $_.success -eq '0' }).Count
    $result.Outcome = 'PASS'
}
catch { $result.Outcome = 'ERROR'; $result.Error = $_.Exception.Message }
finally {
    if ($pattern -and !$pattern.HasExited) { $pattern.CloseMainWindow() | Out-Null; $null = $pattern.WaitForExit(5000) }
    if ($pattern -and $pattern.HasExited) { $result.PatternExitCode = $pattern.ExitCode }
    $result.CompletedUtc = [datetime]::UtcNow.ToString('o'); Save-Result
}
if ($result.Outcome -ne 'PASS') { throw ($result.Outcome + ': ' + $result.Error) }
Write-Output ('PASS: ' + $RunName)
