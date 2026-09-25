param(
    [string]$NdkRoot = "$env:LOCALAPPDATA\Android\Sdk\ndk\28.2.13676358",
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Join-Path $repo 'device\final\diagnostic\sweetdisplay_diag.cpp'
$outputDirectory = Join-Path $repo "out\final-device\$Configuration"
$output = Join-Path $outputDirectory 'sweetdisplay-diag'
$compiler = Join-Path $NdkRoot 'toolchains\llvm\prebuilt\windows-x86_64\bin\aarch64-linux-android29-clang++.cmd'
$readelf = Join-Path $NdkRoot 'toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe'
$strip = Join-Path $NdkRoot 'toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-strip.exe'

if (-not (Test-Path -LiteralPath $compiler)) {
    throw "Android NDK compiler not found: $compiler"
}

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

$arguments = @(
    '-std=c++17', '-O2', '-g0', '-fPIE', '-static',
    '-fno-exceptions', '-fno-rtti', '-fno-unwind-tables',
    '-fno-asynchronous-unwind-tables', '-ffunction-sections', '-fdata-sections',
    '-Wl,--gc-sections', '-Wall', '-Wextra', '-Werror',
    $source, '-o', $output
)

& $compiler @arguments
if ($LASTEXITCODE -ne 0) { throw "Diagnostic compile failed: $LASTEXITCODE" }

& $strip --strip-all $output
if ($LASTEXITCODE -ne 0) { throw "Diagnostic strip failed: $LASTEXITCODE" }

& $readelf -h -l -d $output
if ($LASTEXITCODE -ne 0) { throw "ELF inspection failed: $LASTEXITCODE" }

$hash = (Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash.ToLowerInvariant()
$item = Get-Item -LiteralPath $output
Write-Output "OUTPUT=$output"
Write-Output "BYTES=$($item.Length)"
Write-Output "SHA256=$hash"
