param(
    [string]$NdkRoot = "$env:LOCALAPPDATA\Android\Sdk\ndk\28.2.13676358",
    [string]$Configuration = 'Release',
    [ValidateSet(1, 2, 3, 4)][int]$Candidate = 4
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$diagnosticSource = Join-Path $repo 'device\final\diagnostic\sweetdisplay_diag.cpp'
$daemonSource = Join-Path $repo 'device\final\usb\sweetdisplay_usbd.cpp'
$outputDirectory = Join-Path $repo "out\final-device\$Configuration"
$suffix = switch ($Candidate) { 1 { 'final-usb' } 2 { 'final-usb2' } 3 { 'final-usb3' } 4 { 'final-usb4' } }
$diagnostic = Join-Path $outputDirectory "sweetdisplay-diag-$suffix"
$daemon = Join-Path $outputDirectory "sweetdisplay-usbd-$suffix"
$toolRoot = Join-Path $NdkRoot 'toolchains\llvm\prebuilt\windows-x86_64\bin'
$compiler = Join-Path $toolRoot 'aarch64-linux-android29-clang++.cmd'
$readelf = Join-Path $toolRoot 'llvm-readelf.exe'
$strip = Join-Path $toolRoot 'llvm-strip.exe'

foreach ($path in @($compiler, $readelf, $strip, $diagnosticSource, $daemonSource)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Required build input missing: $path" }
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

$common = @(
    '-std=c++17', '-O2', '-g0', '-fPIE', '-static',
    '-fno-exceptions', '-fno-rtti', '-fno-unwind-tables',
    '-fno-asynchronous-unwind-tables', '-ffunction-sections', '-fdata-sections',
    '-Wl,--gc-sections', '-Wall', '-Wextra', '-Werror'
)

& $compiler '-std=c++17' '-Wall' '-Wextra' '-Werror' '-fsyntax-only' `
    (Join-Path $repo 'device\final\diagnostic\tests\flip_events_test.cpp')
if ($LASTEXITCODE -ne 0) { throw 'FINAL-USB flip-event compile-time tests failed' }

$candidateMacro = "-DSWEETDISPLAY_FINAL_USB_CANDIDATE$Candidate=1"
& $compiler @common '-DSWEETDISPLAY_FINAL_USB=1' $candidateMacro $diagnosticSource '-o' $diagnostic
if ($LASTEXITCODE -ne 0) { throw "FINAL-USB diagnostic compile failed: $LASTEXITCODE" }
& $compiler @common $candidateMacro $daemonSource '-o' $daemon
if ($LASTEXITCODE -ne 0) { throw "FINAL-USB daemon compile failed: $LASTEXITCODE" }

foreach ($binary in @($diagnostic, $daemon)) {
    & $strip --strip-all $binary
    if ($LASTEXITCODE -ne 0) { throw "Strip failed: $binary" }
    $report = & $readelf -h -l -d $binary 2>&1
    if ($LASTEXITCODE -ne 0) { throw "ELF inspection failed: $binary" }
    if (($report -join "`n") -match 'INTERP|Dynamic section at offset') {
        throw "Static-ELF requirement failed: $binary"
    }
    $item = Get-Item -LiteralPath $binary
    $hash = (Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash.ToLowerInvariant()
    Write-Output "BINARY=$binary"
    Write-Output "BYTES=$($item.Length)"
    Write-Output "SHA256=$hash"
}
Write-Output 'FLIP_EVENT_TESTS=PASS (18 compile-time cases)'
Write-Output 'STATIC_ELF=PASS'
