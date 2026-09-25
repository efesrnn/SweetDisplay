param(
    [Parameter(Mandatory=$true)][string]$Python
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$privateRoot = Join-Path $repo 'docs\evidence\private\final-boot-bringup\v4'
$stockRoot = Join-Path $repo 'docs\evidence\private\stock\V14.0.2.0.TKFTRXM'
$stockRecoveryRamdisk = Join-Path $stockRoot 'analysis\recovery\ramdisk'
$prepRoot = Join-Path $repo 'docs\evidence\private\final-boot-prep1'
$unpacked = Join-Path $prepRoot 'stock-roundtrip\unpacked'
$diagnostic = Join-Path $repo 'out\final-device\Release\sweetdisplay-diag-v4'
$ramdisk = Join-Path $privateRoot 'SWEETDISPLAY-TEMP-BOOT-v4.ramdisk.gz'
$manifest = Join-Path $privateRoot 'boot-critical-manifest.json'
$candidate = Join-Path $privateRoot 'SWEETDISPLAY-TEMP-BOOT-v4.img'
$validated = Join-Path $privateRoot 'validated-unpacked'
$mkbootimg = Join-Path $repo 'third_party\upstream\aosp-mkbootimg-android13-r36\mkbootimg.py'
$unpackBootimg = Join-Path $repo 'third_party\upstream\aosp-mkbootimg-android13-r36\unpack_bootimg.py'
$ramdiskBuilder = Join-Path $repo 'device\final\tools\build_ramdisk.py'
$initRc = Join-Path $repo 'device\final\diagnostic\init.sweetdisplay.v4.rc'

foreach ($path in @($Python, $stockRecoveryRamdisk,
        (Join-Path $unpacked 'kernel'), (Join-Path $unpacked 'dtb'),
        $diagnostic, $mkbootimg, $unpackBootimg, $ramdiskBuilder, $initRc)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Required input missing: $path" }
}

New-Item -ItemType Directory -Path $privateRoot -Force | Out-Null

& $Python $ramdiskBuilder `
    --stock-recovery-ramdisk $stockRecoveryRamdisk `
    --diagnostic $diagnostic `
    --init-rc $initRc `
    --required-empty-directory mnt `
    --required-empty-directory debug_ramdisk `
    --required-empty-directory apex `
    --output $ramdisk `
    --manifest $manifest
if ($LASTEXITCODE -ne 0) { throw "Ramdisk v4 build failed: $LASTEXITCODE" }

$cmdline = 'androidboot.hardware=qcom androidboot.memcg=1 lpm_levels.sleep_disabled=1 video=vfb:640x400,bpp=32,memsize=3072000 msm_rtb.filter=0x237 service_locator.enable=1 swiotlb=1 androidboot.usbcontroller=a600000.dwc3 loop.max_part=7 buildvariant=user'
& $Python $mkbootimg `
    --header_version 2 --os_version 13.0.0 --os_patch_level 2023-08 `
    --kernel (Join-Path $unpacked 'kernel') --ramdisk $ramdisk `
    --dtb (Join-Path $unpacked 'dtb') --pagesize 4096 --base 0 `
    --kernel_offset 0x8000 --ramdisk_offset 0x01000000 `
    --second_offset 0 --tags_offset 0x100 --dtb_offset 0x01f00000 `
    --board '' --cmdline $cmdline --output $candidate
if ($LASTEXITCODE -ne 0) { throw "Boot image v4 build failed: $LASTEXITCODE" }

& $Python $unpackBootimg --boot_img $candidate --out $validated --format info
if ($LASTEXITCODE -ne 0) { throw "Candidate v4 re-unpack failed: $LASTEXITCODE" }

$stockKernelHash = (Get-FileHash -LiteralPath (Join-Path $unpacked 'kernel') -Algorithm SHA256).Hash.ToLowerInvariant()
$stockDtbHash = (Get-FileHash -LiteralPath (Join-Path $unpacked 'dtb') -Algorithm SHA256).Hash.ToLowerInvariant()
$validatedKernelHash = (Get-FileHash -LiteralPath (Join-Path $validated 'kernel') -Algorithm SHA256).Hash.ToLowerInvariant()
$validatedDtbHash = (Get-FileHash -LiteralPath (Join-Path $validated 'dtb') -Algorithm SHA256).Hash.ToLowerInvariant()
$validatedRamdiskHash = (Get-FileHash -LiteralPath (Join-Path $validated 'ramdisk') -Algorithm SHA256).Hash.ToLowerInvariant()
$candidateHash = (Get-FileHash -LiteralPath $candidate -Algorithm SHA256).Hash.ToLowerInvariant()
$ramdiskHash = (Get-FileHash -LiteralPath $ramdisk -Algorithm SHA256).Hash.ToLowerInvariant()
$diagnosticHash = (Get-FileHash -LiteralPath $diagnostic -Algorithm SHA256).Hash.ToLowerInvariant()
$item = Get-Item -LiteralPath $candidate

if ($stockKernelHash -ne '764b10ccaf74d2fb0a4569d1254bfc21251a3351afe296d149f639c3d65c7bfc') {
    throw 'Pinned stock kernel hash changed'
}
if ($stockDtbHash -ne '8f1b614d87eb9c6835ecf56cfe6f7a338855a5317dad1eab6391222e04804d10') {
    throw 'Pinned stock DTB hash changed'
}
if ($validatedKernelHash -ne $stockKernelHash) { throw 'Candidate v4 kernel hash mismatch' }
if ($validatedDtbHash -ne $stockDtbHash) { throw 'Candidate v4 DTB hash mismatch' }
if ($validatedRamdiskHash -ne $ramdiskHash) { throw 'Candidate v4 ramdisk hash mismatch' }
if ($item.Length -gt 134217728 -or $item.Length -gt 805306368) {
    throw 'Candidate v4 exceeds a validated size envelope'
}

$bytes = [System.IO.File]::ReadAllBytes($candidate)
$magic = [Text.Encoding]::ASCII.GetString($bytes, 0, 8)
$pageSize = [BitConverter]::ToUInt32($bytes, 36)
$headerVersion = [BitConverter]::ToUInt32($bytes, 40)
$headerSize = [BitConverter]::ToUInt32($bytes, 1644)
$footerMagic = [Text.Encoding]::ASCII.GetString($bytes, $bytes.Length - 64, 4)
if ($magic -ne 'ANDROID!' -or $pageSize -ne 4096 -or
        $headerVersion -ne 2 -or $headerSize -ne 1660) {
    throw 'Candidate v4 boot header mismatch'
}
if ($footerMagic -eq 'AVBf') { throw 'Unexpected AVB footer on candidate v4' }

$manifestData = Get-Content -Raw -LiteralPath $manifest | ConvertFrom-Json
foreach ($required in @('mnt', 'debug_ramdisk', 'apex')) {
    $entry = @($manifestData | Where-Object { $_.path -eq $required })
    if ($entry.Count -ne 1 -or $entry[0].type -ne 'directory' -or
            $entry[0].mode -ne '0040755' -or $entry[0].uid -ne 0 -or
            $entry[0].gid -ne 0) {
        throw "Required directory validation failed: $required"
    }
}

Write-Output "CANDIDATE=$candidate"
Write-Output "BYTES=$($item.Length)"
Write-Output "SHA256=$candidateHash"
Write-Output "RAMDISK_SHA256=$ramdiskHash"
Write-Output "DIAGNOSTIC_SHA256=$diagnosticHash"
Write-Output "STOCK_KERNEL_SHA256=$stockKernelHash"
Write-Output "STOCK_DTB_SHA256=$stockDtbHash"
Write-Output "MANIFEST=$manifest"
Write-Output 'DIRECTORY_CLOSURE=PASS'
Write-Output 'STRUCTURAL_VALIDATION=PASS'
