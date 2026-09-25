param(
    [string]$NdkRoot = "$env:LOCALAPPDATA\Android\Sdk\ndk\28.2.13676358",
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Join-Path $repo 'device\final\diagnostic\sweetdisplay_diag.cpp'
$outputDirectory = Join-Path $repo "out\final-device\$Configuration"
$output = Join-Path $outputDirectory 'sweetdisplay-diag-v3'
$compiler = Join-Path $NdkRoot 'toolchains\llvm\prebuilt\windows-x86_64\bin\aarch64-linux-android29-clang++.cmd'
$readelf = Join-Path $NdkRoot 'toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe'
$strip = Join-Path $NdkRoot 'toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-strip.exe'

if (-not (Test-Path -LiteralPath $compiler)) {
    throw "Android NDK compiler not found: $compiler"
}

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

$arguments = @(
    '-std=c++17', '-O2', '-g0', '-fPIE', '-static',
    '-DSWEETDISPLAY_DIAGNOSTIC_V3=1',
    '-fno-exceptions', '-fno-rtti', '-fno-unwind-tables',
    '-fno-asynchronous-unwind-tables', '-ffunction-sections', '-fdata-sections',
    '-Wl,--gc-sections', '-Wall', '-Wextra', '-Werror',
    $source, '-o', $output
)

& $compiler @arguments
if ($LASTEXITCODE -ne 0) { throw "Diagnostic v3 compile failed: $LASTEXITCODE" }

& $strip --strip-all $output
if ($LASTEXITCODE -ne 0) { throw "Diagnostic v3 strip failed: $LASTEXITCODE" }

$elfReport = & $readelf -h -l -d $output 2>&1
if ($LASTEXITCODE -ne 0) { throw "Diagnostic v3 ELF inspection failed: $LASTEXITCODE" }
if (($elfReport -join "`n") -match 'INTERP|Dynamic section at offset') {
    throw 'Diagnostic v3 unexpectedly has an interpreter or dynamic section'
}

$hash = (Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash.ToLowerInvariant()
$item = Get-Item -LiteralPath $output
Write-Output "OUTPUT=$output"
Write-Output "BYTES=$($item.Length)"
Write-Output "SHA256=$hash"
Write-Output 'STATIC_ELF=PASS'
