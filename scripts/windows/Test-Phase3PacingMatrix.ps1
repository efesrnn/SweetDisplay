# Read-only local runtime experiments. Stop the batch on any failed test.
$ErrorActionPreference='Stop'
$test=Join-Path $PSScriptRoot 'Test-Phase3Capacity.ps1'
& $test -RunName pacing-60-sync0 -Seconds 30 -PatternFps 60 -SyncInterval 0 -InspectBefore 15
& $test -RunName pacing-120-sync0 -Seconds 30 -PatternFps 120 -SyncInterval 0 -InspectBefore 15
& $test -RunName pacing-120-no-sample -Seconds 30 -PatternFps 120 -SyncInterval 0 -InspectBefore 15 -SkipSampling
