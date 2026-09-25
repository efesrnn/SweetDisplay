param(
    [string]$Python = ''
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($Python)) {
    $pythonCommand = Get-Command python -ErrorAction SilentlyContinue
    if (-not $pythonCommand) {
        throw 'Python 3 not found on PATH; pass -Python with an explicit host path'
    }
    $Python = $pythonCommand.Source
}
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$privateRoot = Join-Path $repo 'docs\evidence\private\final-boot-prep1'
$stockRoot = Join-Path $repo 'docs\evidence\private\stock\V14.0.2.0.TKFTRXM'
$stockBoot = Join-Path $stockRoot 'extracted\fastboot\sweet_tr_global_images_V14.0.2.0.TKFTRXM_13.0\images\boot.img'
$stockRecoveryRamdisk = Join-Path $stockRoot 'analysis\recovery\ramdisk'
$unpacked = Join-Path $privateRoot 'stock-roundtrip\unpacked'
$diagnostic = Join-Path $repo 'out\final-device\Release\sweetdisplay-diag'
$ramdisk = Join-Path $privateRoot 'candidate\SWEETDISPLAY-TEMP-BOOT-v1.ramdisk.gz'
$candidate = Join-Path $privateRoot 'candidate\SWEETDISPLAY-TEMP-BOOT-v1.img'
$validated = Join-Path $privateRoot 'candidate\validated-unpacked'
$mkbootimg = Join-Path $repo 'third_party\upstream\aosp-mkbootimg-android13-r36\mkbootimg.py'
$unpackBootimg = Join-Path $repo 'third_party\upstream\aosp-mkbootimg-android13-r36\unpack_bootimg.py'
$ramdiskBuilder = Join-Path $repo 'device\final\tools\build_ramdisk.py'

foreach ($path in @($Python, $stockBoot, $stockRecoveryRamdisk,
        (Join-Path $unpacked 'kernel'), (Join-Path $unpacked 'dtb'),
        $diagnostic, $mkbootimg, $ramdiskBuilder)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Required input missing: $path" }
}

& $Python $ramdiskBuilder `
    --stock-recovery-ramdisk $stockRecoveryRamdisk `
    --diagnostic $diagnostic `
    --init-rc (Join-Path $repo 'device\final\diagnostic\init.sweetdisplay.rc') `
    --ueventd-rc (Join-Path $repo 'device\final\diagnostic\ueventd.sweetdisplay.rc') `
    --output $ramdisk
if ($LASTEXITCODE -ne 0) { throw "Ramdisk build failed: $LASTEXITCODE" }

$cmdline = 'androidboot.hardware=qcom androidboot.memcg=1 lpm_levels.sleep_disabled=1 video=vfb:640x400,bpp=32,memsize=3072000 msm_rtb.filter=0x237 service_locator.enable=1 swiotlb=1 androidboot.usbcontroller=a600000.dwc3 loop.max_part=7 buildvariant=user'
& $Python $mkbootimg `
    --header_version 2 --os_version 13.0.0 --os_patch_level 2023-08 `
    --kernel (Join-Path $unpacked 'kernel') --ramdisk $ramdisk `
    --dtb (Join-Path $unpacked 'dtb') --pagesize 4096 --base 0 `
    --kernel_offset 0x8000 --ramdisk_offset 0x01000000 `
    --second_offset 0 --tags_offset 0x100 --dtb_offset 0x01f00000 `
    --board '' --cmdline $cmdline --output $candidate
if ($LASTEXITCODE -ne 0) { throw "Boot image build failed: $LASTEXITCODE" }

& $Python $unpackBootimg --boot_img $candidate --out $validated --format info
if ($LASTEXITCODE -ne 0) { throw "Candidate re-unpack failed: $LASTEXITCODE" }

$stockKernelHash = (Get-FileHash -LiteralPath (Join-Path $unpacked 'kernel') -Algorithm SHA256).Hash.ToLowerInvariant()
$stockDtbHash = (Get-FileHash -LiteralPath (Join-Path $unpacked 'dtb') -Algorithm SHA256).Hash.ToLowerInvariant()
$validatedKernelHash = (Get-FileHash -LiteralPath (Join-Path $validated 'kernel') -Algorithm SHA256).Hash.ToLowerInvariant()
$validatedDtbHash = (Get-FileHash -LiteralPath (Join-Path $validated 'dtb') -Algorithm SHA256).Hash.ToLowerInvariant()
$validatedRamdiskHash = (Get-FileHash -LiteralPath (Join-Path $validated 'ramdisk') -Algorithm SHA256).Hash.ToLowerInvariant()
$candidateHash = (Get-FileHash -LiteralPath $candidate -Algorithm SHA256).Hash.ToLowerInvariant()
$ramdiskHash = (Get-FileHash -LiteralPath $ramdisk -Algorithm SHA256).Hash.ToLowerInvariant()
$item = Get-Item -LiteralPath $candidate
if ($validatedKernelHash -ne $stockKernelHash) { throw 'Candidate kernel hash mismatch' }
if ($validatedDtbHash -ne $stockDtbHash) { throw 'Candidate DTB hash mismatch' }
if ($validatedRamdiskHash -ne $ramdiskHash) { throw 'Candidate ramdisk hash mismatch' }
if ($item.Length -gt 134217728) { throw 'Candidate exceeds stock boot envelope' }
$footer = [System.IO.File]::ReadAllBytes($candidate)
$footerMagic = [System.Text.Encoding]::ASCII.GetString($footer, $footer.Length - 64, 4)
if ($footerMagic -eq 'AVBf') { throw 'Unexpected AVB footer on unsigned temporary candidate' }
Write-Output "CANDIDATE=$candidate"
Write-Output "BYTES=$($item.Length)"
Write-Output "SHA256=$candidateHash"
Write-Output "RAMDISK_SHA256=$ramdiskHash"
Write-Output "STOCK_KERNEL_SHA256=$stockKernelHash"
Write-Output "STOCK_DTB_SHA256=$stockDtbHash"
Write-Output 'STRUCTURAL_VALIDATION=PASS'
