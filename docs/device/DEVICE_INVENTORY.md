# Sweet device inventory

Inventory date: 2026-09-20 (Europe/Istanbul)

This document records DEVICE PHASE 1 read-only evidence for the connected Xiaomi
Redmi Note 10 Pro. Raw command output is under the ignored
`docs/evidence/private/` tree. Serial numbers, USB instance identifiers and other
unique identifiers are intentionally omitted here.

## Evidence labels

- **VERIFIED**: observed from the connected device or the pinned local Git object.
- **SOURCE**: present in Xiaomi's pinned source; this does not prove that the
  running phone uses the same revision or configuration.
- **LIKELY**: supported by converging evidence but not directly measured.
- **UNKNOWN**: the available read-only interfaces did not establish the value.

## Host tooling and connection

| Item | Result | Classification |
|---|---|---|
| Fastboot | `31.0.3-7562133`, installed at the known local unlock-tool path | VERIFIED |
| ADB | `1.0.41`, platform-tools `36.0.2-14143358` | VERIFIED |
| Fastboot USB transport | Exactly one transport was returned by `fastboot devices -l` | VERIFIED |
| ADB after owner authorization | Exactly one device in `device` state | VERIFIED |
| Windows PnP name | The bounded Android/Xiaomi/Qualcomm filters returned no matching friendly-name record | UNKNOWN |

The stock reboot was allowed by the phase instructions and succeeded. The owner
later enabled USB debugging and authorized this PC. All subsequent ADB operations
were unprivileged, read-only property, sysfs/procfs, `dumpsys`, directory-listing or
configuration-file queries. No `adb root`, `su`, remount or write command was used.

## Connected bootloader evidence

| Query/fact | Result | Classification |
|---|---|---|
| `product` | `sweet` | VERIFIED |
| `unlocked` | `yes` | VERIFIED |
| `secure` | `yes` | VERIFIED; this is a bootloader variable, not proof of Android's runtime verified-boot color/state |
| `is-userspace` | `no` | VERIFIED; the observed mode was bootloader Fastboot, not fastbootd |
| `max-download-size` | 805,306,368 bytes (768 MiB) | VERIFIED |
| `anti` | `3` | VERIFIED raw value; its precise vendor rollback-policy meaning is not inferred |
| `board_version` | `10.19.6` | VERIFIED raw bootloader value; not treated as the Android build |
| `variant` | `SM7 UFS` | VERIFIED raw bootloader value; consistent with the expected Qualcomm family, but not an exact SoC ID |
| `kernel` | `uefi` | VERIFIED raw bootloader value; it describes the bootloader environment, not the running Linux kernel |
| `current-slot` | unsupported (`GetVar Variable Not found`) | UNKNOWN as a variable |
| `slot-count` | unsupported | UNKNOWN as a variable |
| `has-slot:boot` | unsupported | UNKNOWN as a variable |
| `has-slot:vendor_boot` | unsupported | UNKNOWN as a variable |
| `has-slot:init_boot` | unsupported | UNKNOWN as a variable |
| `has-slot:recovery` | unsupported | UNKNOWN as a variable |

`getvar all` exposed 97 unsuffixed partition-size keys and zero keys ending in
`_a` or `_b`. It exposed dedicated `boot` and `recovery` partitions, and did not
expose `vendor_boot` or `init_boot` partitions.

## Partition and boot architecture

| Partition | Bootloader size/type evidence | Interpretation |
|---|---|---|
| `boot` | `0x08000000` (128 MiB), raw | Dedicated unsuffixed boot partition |
| `recovery` | `0x08000000` (128 MiB), raw | Dedicated unsuffixed recovery partition |
| `dtbo` | `0x02000000` (32 MiB), raw | Separate DTBO partition |
| `vbmeta` | `0x00020000` (128 KiB), raw | AVB top-level metadata partition is present |
| `vbmeta_system` | `0x00020000` (128 KiB) | Separate system AVB metadata is present |
| `vbmeta_vendor` | `0x00020000` (128 KiB) | Separate vendor AVB metadata is present |
| `super` | `0x220000000` (8.5 GiB), raw | Dynamic-partition container is present |
| `vendor_boot` | query unsupported and absent from `getvar all` | Not exposed; exact absence is not asserted beyond this evidence |
| `init_boot` | query unsupported and absent from `getvar all` | Not exposed; exact absence is not asserted beyond this evidence |

### Architecture conclusions

- **VERIFIED: non-A/B layout.** The connected bootloader reports only unsuffixed
  partition keys, a dedicated `recovery`, zero `_a`/`_b` size keys, and no slot
  variables. This is also consistent with the official TWRP `sweet` instructions,
  which target the unsuffixed `recovery` partition.
- **VERIFIED: dynamic partitions.** The connected device exposes an 8.5 GiB
  `super` partition. Android reports `ro.boot.dynamic_partitions=true`; mapper
  entries expose `system`, `vendor`, `product`, `odm`, `system_ext` and their
  verity devices. The official TWRP page independently identifies `sweet` as a
  dynamic-partition device.
- **VERIFIED: normal bootloader Fastboot was observed.** `is-userspace:no`.
  AOSP distinguishes this from fastbootd with the same variable. Logical
  partitions inside `super` would be managed through fastbootd/recovery tooling,
  but DEVICE PHASE 1 did not enter it.
- **VERIFIED: AVB-related partitions exist.** `vbmeta`, `vbmeta_system`, and
  `vbmeta_vendor` are exposed. Android reports AVB `1.1`, device state `unlocked`,
  verified-boot state `orange` and verity mode `enforcing`. Descriptors, keys,
  rollback indexes and chained-partition contents were not read or modified.

References: [AOSP partitions overview](https://source.android.com/docs/core/architecture/partitions),
[AOSP fastbootd](https://source.android.com/docs/core/architecture/bootloader/fastbootd),
[AOSP AVB](https://source.android.com/docs/security/features/verifiedboot/avb), and
[official TWRP sweet page](https://twrp.me/xiaomi/xiaomiredminote10pro.html).

## Boot-image and device-tree expectations

Connected evidence establishes separate raw `boot`, `recovery` and `dtbo`
partitions. The pinned source's `AndroidKernel.mk` supports ARM64 `Image`,
`Image.gz`, optional appended DTB, or a separately generated `dtb.img`. Its DTS
Makefile builds `sweet-sdmmagpie-overlay.dtbo` over `sdmmagpie.dtb`.

The installed software has first/vendor API level 30, a 4.14 kernel, a dedicated
recovery and no exposed `vendor_boot` or `init_boot`; this is a non-GKI-style
Android 11 launch architecture subsequently upgraded to Android 13. The exact boot
header version, page size, load addresses, kernel/ramdisk compression, command
line, DTB placement and AVB footer remain **UNKNOWN**. An unprivileged ADB read of
the first 4 KiB of `boot` correctly failed with `Permission denied`; no root or
bypass was attempted. Android's generic
documentation permits several non-A/B/non-GKI layouts, so the header is not
guessed. See [AOSP boot image headers](https://source.android.com/docs/core/architecture/bootloader/boot-image-header).

## Stock software and running-kernel identity

| Item | Result |
|---|---|
| Model/product | Xiaomi `M2101K6G`; `sweet`; product name `sweet_tr`; SKU `pro` |
| SoC/platform | QTI `SM7150`; board platform `sm6150`; ARM64 |
| Installed Android | Android 13, API 33; first product/vendor API 30 |
| Installed MIUI/region | MIUI 14 (`V140`), Turkey (`tr`), `V14.0.2.0.TKFTRXM` |
| Build fingerprint | `Redmi/sweet_tr/sweet:13/TKQ1.221013.002/V14.0.2.0.TKFTRXM:user/release-keys` |
| Boot/vendor fingerprint | `Redmi/sweet_tr/sweet:13/RKQ1.210614.002/V14.0.2.0.TKFTRXM:user/release-keys` |
| Security patch | System and vendor: `2023-08-01` |
| Running kernel | `4.14.190-perf-g6d6db67fd446`, SMP PREEMPT, built 2023-08-03 14:51:04 UTC |
| Kernel toolchain | Clang `10.0.7` for Android NDK; GNU ld `2.27.0.20170315` |
| Running kernel config | `/proc/config.gz` readable; relevant options inventoried below |
| Active DTBO index | `12`; underlying DT model/board properties are SELinux-denied to unprivileged shell |
| Runtime display/touch | DRM/SDE DSI display; active input device `goodix_ts` |
| Bootloader/baseband version strings | Returned blank by this Fastboot implementation |

`ro.build.display.id` reports `TKQ1.221013.002 test-keys` while the system,
boot/vendor fingerprints report `release-keys`. Both observations are preserved;
neither is used alone to infer modification or authenticity.

### Connected runtime subsystem inventory

| Area | Connected Android evidence | Result |
|---|---|---|
| Display | `/dev/dri/card0`, `renderD128`, SDE CRTCs and `card0-DSI-1`; no `/dev/fb*` | VERIFIED DRM/SDE path, not legacy framebuffer |
| Display modes | Physical 1080x2400; 60.000004 and 120.00001 Hz modes; 60 Hz active during capture | VERIFIED |
| Touch | Input Reader reports `goodix_ts`; I2C input is at `2-005a` | VERIFIED active Goodix controller path |
| USB device controller | `/sys/class/udc/a600000.dwc3` | VERIFIED Qualcomm DWC3 UDC exists |
| USB gadget config | Runtime config enables DWC3/MSM, Gadget, ConfigFS, FunctionFS, NCM and HID | VERIFIED kernel support |
| USB gadget exclusions | Runtime config explicitly disables ConfigFS ACM, ECM, RNDIS, EEM and UVC | VERIFIED; these would require a future config change, not Phase 1 work |
| Video codec | `video32`-`video34` map to `aa00000.qcom,vidc1`; config enables `MSM_VIDC_V4L2`; vendor codec XML exposes Qualcomm AVC decoder/secure decoder | VERIFIED inventory support; no decode workload was run |
| GPU | Runtime config enables `QCOM_KGSL` and KGSL IOMMU; `/dev/dri/renderD128` exists | VERIFIED inventory support; no GPU workload was run |
| Memory sharing/IOMMU | Runtime config enables `DMA_SHARED_BUFFER`, `ION`, `ARM_SMMU` and KGSL IOMMU | VERIFIED |
| Touch build options | Runtime config enables Goodix GTX9896 K6, FTS K6 and Xiaomi touch feature; active input is Goodix | VERIFIED; alternate compiled support is not active-hardware proof |

## Pinned Xiaomi source

The ignored filtered Git object store resolves `sweet-r-oss` exactly to:

`758bb7ef50af360e728662a1ed3b3a1b977a2f13`

Commit date/title: `2021-03-05`, “Kernel: Xiaomi kernel changes for Redmi Note 10
Pro Android R”. The source Makefile reports Linux `4.14.180`. The root
`sweet_user_defconfig` is a symlink to `vendor/sweet_user_defconfig`.

Upstream: [MiCode pinned source tree](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/758bb7ef50af360e728662a1ed3b3a1b977a2f13).

### SweetDisplay-relevant source inventory

| Area | Pinned-source evidence | Conservative conclusion |
|---|---|---|
| Platform | `CONFIG_ARCH_QCOM=y`, `CONFIG_ARCH_SDMMAGPIE=y`; sweet overlay has MSM ID 365 and `qcom,sdmmagpie*` compatibles | SOURCE supports the Qualcomm SDMMAGPIE/SM7150-family platform used by this tree |
| Display/DRM | `CONFIG_DRM=y`, SDE event/RSC options, `mdss_mdp` SDE connectors, Qualcomm DRM/SDE/DSI sources | SOURCE aligns with the connected DRM/SDE/DSI topology |
| AMOLED/DSI panels | Sweet overlay activates `dsi_sw43404_amoled_video_display`; sweet DTS also references Xiaomi F4 and K6 DSI panel definitions and LAB/IBB supplies | SOURCE contains device-specific AMOLED/DSI support; exact connected panel SKU remains UNKNOWN |
| Touch | Goodix GTX9896 I2C/update/gesture options and Xiaomi touch-feature option are enabled; DTS gives 1080x2400 bounds and GPIO/regulator data | SOURCE aligns with active `goodix_ts`, but the config symbol differs from the running K6 symbol |
| USB controller/peripheral | `CONFIG_USB_DWC3=y`, `CONFIG_USB_DWC3_MSM=y`, `CONFIG_USB_GADGET=y`; DWC3 gadget and MSM glue sources exist | SOURCE aligns with connected `a600000.dwc3` UDC |
| Configfs/FunctionFS | `CONFIG_USB_CONFIGFS=y` and `CONFIG_USB_CONFIGFS_F_FS=y`; `configfs.c` and `f_fs.c` exist | SOURCE and running config both enable the likely userspace USB transport foundation |
| USB networking/HID | `CONFIG_USB_CONFIGFS_NCM=y`, `CONFIG_USB_CONFIGFS_F_HID=y`, `CONFIG_USB_USBNET=y`; ECM/RNDIS/EEM source files exist | NCM and HID are enabled in source/runtime; runtime explicitly disables ECM/RNDIS/EEM |
| UVC | `f_uvc.c` exists, but the sweet defconfig does not explicitly enable it | Runtime explicitly disables ConfigFS UVC; later camera/UVC work requires a config change |
| GPU | `CONFIG_QCOM_KGSL=y`; KGSL source and SDMMAGPIE GPU DTS are present | SOURCE aligns with runtime KGSL/IOMMU support; no workload was tested |
| Video codec/media | `CONFIG_MEDIA_SUPPORT=y`, `CONFIG_MSM_VIDC_V4L2=y`, `CONFIG_MSM_VIDC_GOVERNORS=y`; MSM VIDC and Venus HFI sources exist; SDMMAGPIE includes a VIDC DTS | SOURCE aligns with runtime VIDC nodes and Qualcomm AVC declarations; actual decode remains untested |
| IOMMU | `CONFIG_ARM_SMMU=y`, `CONFIG_IOMMU_IO_PGTABLE_FAST=y`; SDMMAGPIE display/video/GPU nodes carry SMMU/IOMMU relationships | SOURCE aligns with the running ARM SMMU configuration |
| DMA-BUF/ION | `CONFIG_ION=y`; DMA-BUF framework source exists | Runtime confirms `DMA_SHARED_BUFFER=y` and `ION=y`; zero-copy interoperability remains untested |
| Regulators/power | RPMh and QPNP LCDB regulator options are enabled; sweet DTS supplies panel, touch, USB and camera rails | SOURCE contains device-specific power dependencies; a minimal userspace must preserve their sequencing |

The source inventory proves presence, not that all features work outside Xiaomi's
Android userspace or without proprietary firmware.

## Source/device compatibility assessment

- **VERIFIED:** the connected bootloader identifies the product as `sweet`; the
  pinned branch and sweet DTS/defconfig are specifically for Redmi Note 10 Pro.
- **VERIFIED NON-MATCH:** the phone runs `4.14.190-perf-g6d6db67fd446`, built in
  August 2023, while the pinned commit is a March 2021 `4.14.180` tree. The pinned
  defconfig uses `GOODIX_GTX9896_I2C`; the running config uses
  `GOODIX_GTX9896_K6`. The pinned commit is therefore not the exact running source.
- **SOURCE:** the commit contains device-specific SDMMAGPIE DTS/DTBO, AMOLED,
  Goodix touch, DWC3 gadget, Qualcomm display/GPU/video, IOMMU and power support.
- **LIKELY:** this is a useful architectural baseline for the product family, but a
  matching 4.14.190 source/config or a carefully reconciled downstream tree is
  required before building.
- **UNKNOWN:** the public source corresponding exactly to embedded ID
  `g6d6db67fd446`, exact panel SKU and complete proprietary firmware compatibility.
  Boot-header, ramdisk, DTBO and AVB package facts were subsequently resolved by
  the [stock image gate](STOCK_IMAGE_MANIFEST.md).

Therefore the source is a **device-relevant candidate, not an exact installed-build
match**.

## Temporary boot feasibility (not executed)

Evidence is deliberately separated:

1. **Connected device:** unlocked bootloader, 768 MiB maximum download, dedicated
   128 MiB boot/recovery partitions, and working download-capable Fastboot. No
   capability query proves acceptance of the `boot` command. Android confirms
   SM7150, API-30 launch/vendor level, non-A/B state and DTBO index 12.
2. **AOSP tooling:** the Fastboot client defines `boot` as downloading and booting
   a kernel/image from RAM. This only describes the protocol/client; OEM
   bootloader support is still required. See [AOSP fastboot source](https://android.googlesource.com/platform/system/core/+/android10-release/fastboot/fastboot.cpp).
3. **Device-maintainer evidence:** official TWRP supports `sweet`, confirms dynamic
   partitions, and publishes 128 MiB recovery images, but its official Fastboot
   installation method documents `fastboot flash recovery`, not temporary boot.
4. **Community evidence:** multiple `sweet` guides and user reports describe
   `fastboot boot twrp.img`, including the
   [Xiaomi.eu sweet guide](https://miuipolska.pl/jak-zainstalowac-xiaomi-eu-na-redmi-note-10-pro/).
   This is credible supporting evidence, not machine verification for this phone.

Conclusion: a genuinely temporary `fastboot boot <image>` workflow is **LIKELY but
UNVERIFIED**. It was not attempted. Before any future attempt, an image must match
the installed `V14.0.2.0.TKFTRXM` boot/recovery header version, addresses, kernel
compression, ramdisk format, command line and DTB/DTBO expectations, fit the
bootloader's download constraints, and include a minimal read-only initramfs. An
unlocked state alone is not proof that the bootloader will accept or start it.
