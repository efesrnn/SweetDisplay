#requires -Version 7.4
param(
    [string]$SdkPath,
    [string]$JavaHome,
    [string]$FixtureDirectory,
    [ValidateRange(2, 300)][int]$FrameCount = 90,
    [ValidateRange(96, 4096)][int]$FixtureWidth = 1280,
    [ValidateRange(96, 2160)][int]$FixtureHeight = 576,
    [ValidateRange(1, 240)][double]$FixtureFrameRate = 30
)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$rootFull = [System.IO.Path]::GetFullPath($root).TrimEnd([System.IO.Path]::DirectorySeparatorChar)
if (-not $SdkPath) {
    $SdkPath = if ($env:ANDROID_SDK_ROOT) { $env:ANDROID_SDK_ROOT } else {
        Join-Path $env:LOCALAPPDATA 'Android/Sdk'
    }
}
if (-not $JavaHome) {
    $localJdk = Get-ChildItem -LiteralPath (Join-Path $env:USERPROFILE '.gradle/jdks') `
        -Recurse -Filter javac.exe -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($localJdk) { $JavaHome = Split-Path (Split-Path $localJdk.FullName -Parent) -Parent }
    else {
        $systemJavac = Get-Command javac.exe -CommandType Application -ErrorAction Stop
        $JavaHome = Split-Path (Split-Path $systemJavac.Source -Parent) -Parent
    }
}
$source = Join-Path $root 'device/android-userspace-probe'
if (-not $FixtureDirectory) {
    $FixtureDirectory = Join-Path $root 'docs/evidence/private/phase3b/progressive-1-1280-30/host'
}
$build = Join-Path $root '.local/device-phase2b/probe-build'
$buildFull = [System.IO.Path]::GetFullPath($build)
if (-not $buildFull.StartsWith($rootFull + [System.IO.Path]::DirectorySeparatorChar,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to clean a build directory outside the repository: $buildFull"
}
$assets = Join-Path $build 'assets'
$classes = Join-Path $build 'classes'
$dex = Join-Path $build 'dex'
$buildTools = Join-Path $SdkPath 'build-tools/35.0.0'
$androidJar = Join-Path $SdkPath 'platforms/android-33/android.jar'
$aapt = Join-Path $buildTools 'aapt.exe'
$d8 = Join-Path $buildTools 'd8.bat'
$zipalign = Join-Path $buildTools 'zipalign.exe'
$apksigner = Join-Path $buildTools 'apksigner.bat'
$javac = Join-Path $JavaHome 'bin/javac.exe'
$keytool = Join-Path $JavaHome 'bin/keytool.exe'
$streamPath = Join-Path $FixtureDirectory 'stream.h264'
$csvPath = Join-Path $FixtureDirectory 'encode-output.csv'
foreach ($required in @($androidJar,$aapt,$d8,$zipalign,$apksigner,$javac,$keytool,$streamPath,$csvPath)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Required file missing: $required" }
}

Remove-Item -LiteralPath $build -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $assets,$classes,$dex -Force | Out-Null
$rows = @(Import-Csv -LiteralPath $csvPath | Select-Object -First $FrameCount)
if ($rows.Count -ne $FrameCount) { throw "Fixture has only $($rows.Count) access units" }
if ($rows[0].idr -ne '1' -or $rows[0].sps -ne '1' -or $rows[0].pps -ne '1') {
    throw 'First access unit is not IDR+SPS+PPS'
}
$byteCount = [int64](($rows | Measure-Object -Property bytes -Sum).Sum)
$input = [System.IO.File]::OpenRead($streamPath)
try {
    if ($input.Length -lt $byteCount) { throw 'Stream is shorter than selected access units' }
    $fixture = [byte[]]::new($byteCount)
    $read = 0
    while ($read -lt $fixture.Length) {
        $count = $input.Read($fixture, $read, $fixture.Length - $read)
        if ($count -eq 0) { throw 'Unexpected end of stream' }
        $read += $count
    }
} finally { $input.Dispose() }
[System.IO.File]::WriteAllBytes((Join-Path $assets 'test_stream.h264'), $fixture)
$index = foreach ($row in $rows) {
    $ptsUs = [int64]$row.pts / 10
    '{0},{1}' -f $row.bytes,$ptsUs
}
[System.IO.File]::WriteAllLines((Join-Path $assets 'test_stream.index'), $index,
    [System.Text.Encoding]::ASCII)
[System.IO.File]::WriteAllText((Join-Path $assets 'test_stream.config'),
    ('{0},{1},{2}' -f $FixtureWidth,$FixtureHeight,
        $FixtureFrameRate.ToString([System.Globalization.CultureInfo]::InvariantCulture)),
    [System.Text.Encoding]::ASCII)

$manifest = Join-Path $source 'AndroidManifest.xml'
$java = Join-Path $source 'src/com/sweetdisplay/userspaceprobe/MainActivity.java'
$unsigned = Join-Path $build 'probe-unsigned.apk'
$unaligned = Join-Path $build 'probe-unaligned.apk'
$aligned = Join-Path $build 'SweetDisplayUserspaceProbe.apk'
$keystore = Join-Path $build 'probe-debug.keystore'

& $aapt package -f -M $manifest -I $androidJar -A $assets -F $unaligned
if ($LASTEXITCODE -ne 0) { throw "aapt failed: $LASTEXITCODE" }
& $javac --release 8 -classpath $androidJar -d $classes $java
if ($LASTEXITCODE -ne 0) { throw "javac failed: $LASTEXITCODE" }
$env:JAVA_HOME = $JavaHome
$classFiles = @(Get-ChildItem -LiteralPath (Join-Path $classes 'com/sweetdisplay/userspaceprobe') `
    -Filter '*.class' -File | ForEach-Object {$_.FullName})
& $d8 --lib $androidJar --output $dex @classFiles
if ($LASTEXITCODE -ne 0) { throw "d8 failed: $LASTEXITCODE" }
Copy-Item -LiteralPath $unaligned -Destination $unsigned
Push-Location $dex
try { & $aapt add $unsigned 'classes.dex' } finally { Pop-Location }
if ($LASTEXITCODE -ne 0) { throw "aapt add failed: $LASTEXITCODE" }
& $zipalign -f -p 4 $unsigned $aligned
if ($LASTEXITCODE -ne 0) { throw "zipalign failed: $LASTEXITCODE" }
& $keytool -genkeypair -keystore $keystore -storepass android -alias probe -keypass android `
    -dname 'CN=SweetDisplay Disposable Probe,O=Local Development,C=TR' -keyalg RSA -keysize 2048 `
    -validity 30 -noprompt
if ($LASTEXITCODE -ne 0) { throw "keytool failed: $LASTEXITCODE" }
& $apksigner sign --ks $keystore --ks-pass pass:android --key-pass pass:android $aligned
if ($LASTEXITCODE -ne 0) { throw "apksigner failed: $LASTEXITCODE" }
& $apksigner verify --verbose $aligned
if ($LASTEXITCODE -ne 0) { throw "apksigner verify failed: $LASTEXITCODE" }

[pscustomobject]@{
    apk = $aligned
    apk_bytes = (Get-Item -LiteralPath $aligned).Length
    apk_sha256 = (Get-FileHash -LiteralPath $aligned -Algorithm SHA256).Hash.ToLowerInvariant()
    fixture_frames = $rows.Count
    fixture_bytes = $fixture.Length
    fixture_sha256 = (Get-FileHash -LiteralPath (Join-Path $assets 'test_stream.h264') -Algorithm SHA256).Hash.ToLowerInvariant()
    fixture_width = $FixtureWidth
    fixture_height = $FixtureHeight
    fixture_frame_rate = $FixtureFrameRate
    first_access_unit = 'IDR+SPS+PPS'
} | ConvertTo-Json
