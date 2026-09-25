# DEVICE PHASE 1 results

Result: **VERIFIED**
Date: 2026-09-20

The read-only device/boot/kernel inventory meets DEVICE PHASE 1 acceptance. The
owner authorized ordinary ADB, allowing stock build, running-kernel and runtime
subsystem identity to be collected without root or writes. Remaining image-format
details are explicitly UNKNOWN and gated before any later boot experiment.

## Result summary

| Acceptance item | Result |
|---|---|
| Connected phone identity | VERIFIED: Xiaomi Redmi Note 10 Pro, bootloader product `sweet` |
| `sweet` versus `sweetin` | VERIFIED for this bootloader: `sweet`; `sweetin` is ruled out as the connected product identity |
| Bootloader state | VERIFIED: `unlocked: yes`; `secure: yes` also reported |
| Slot architecture | VERIFIED non-A/B from unsuffixed partition inventory, dedicated recovery, zero `_a`/`_b` keys, and absent slot variables |
| Dynamic partitions | VERIFIED: 8.5 GiB `super`; official TWRP independently identifies dynamic partitions |
| Fastboot mode | VERIFIED bootloader Fastboot (`is-userspace:no`), not fastbootd |
| Boot arrangement | VERIFIED separate raw `boot` and `recovery` (128 MiB each), separate `dtbo` (32 MiB), and `vbmeta*` partitions |
| `vendor_boot` / `init_boot` | Not exposed by the bootloader inventory; precise status remains UNKNOWN |
| Installed Android/MIUI/build | VERIFIED: Android 13 / MIUI 14 Turkey, `V14.0.2.0.TKFTRXM`, model `M2101K6G`, product `sweet_tr` |
| Running kernel | VERIFIED: `4.14.190-perf-g6d6db67fd446`, built 2023-08-03 |
| Pinned source | VERIFIED exact commit `758bb7ef50af360e728662a1ed3b3a1b977a2f13` |
| Source compatibility | VERIFIED not exact: pinned source is `4.14.180` from 2021; running kernel is `4.14.190` from 2023; architecture remains relevant |
| Runtime subsystems | VERIFIED inventory: DRM/SDE DSI, Goodix touch, DWC3 UDC/gadget, ConfigFS/FunctionFS/NCM/HID, KGSL, MSM VIDC, DMA-BUF/ION and ARM SMMU |
| Temporary boot | LIKELY from generic Fastboot plus sweet community evidence, but UNVERIFIED on this bootloader |
| Persistent device writes | NONE performed by the Phase 1 commands; the owner separately enabled USB debugging and authorized this PC |

Detailed evidence and subsystem inventory: [DEVICE_INVENTORY.md](DEVICE_INVENTORY.md).

## Safest next path (future authorization required)

1. Obtain the exact matching Turkey Fastboot package candidate:
   `sweet_tr_global_images_V14.0.2.0.TKFTRXM_20230803.0000.00_13.0_tr_7ab6f47e67.tgz`.
   Public indexes report 5.9 GB and MD5 `7ab6f47e67f2679e5974bbad86238ecc`.
   The matching recovery package candidate is
   `miui_SWEETTRGlobal_V14.0.2.0.TKFTRXM_4dd7b72cd7_13.0.zip`, with reported MD5
   `4dd7b72cd76aee890762b98c031a04f7`. Neither was downloaded in Phase 1.
   References: [firmware index](https://xmfirmwareupdater.com/miui/sweet/stable/V14.0.2.0.TKFTRXM/)
   and [LineageOS device metadata](https://github.com/LineageOS/lineage_wiki/blob/main/_data/devices/sweet_variant1.yml).
2. Verify that any future download comes from Xiaomi's official host, then record
   URL/date/name/size and calculate local SHA-256; do not trust the filename or MD5
   alone.
3. Extract locally without running its scripts. Hash the matching `boot.img`,
   `recovery.img`, `dtbo.img`,
   `vbmeta*.img`, and the `super`/logical-partition payloads. Record
   `vendor_boot.img` or `init_boot.img` only if the exact package actually contains
   them.
4. Parse those stock boot and recovery headers to establish header version, page
   size, kernel/ramdisk compression, load addresses, command line, DTB placement
   and AVB footer. Inspect DTBO entries and select by actual board identifiers.
5. Reproduce a stock-aligned kernel/config and ramdisk before adding SweetDisplay
   changes. Preserve regulator, charging, thermal and display sequencing.
6. Only after those recovery gates, propose one exact temporary image and an exact
   `fastboot boot` command for separate approval. If the bootloader rejects
   temporary boot, stop before proposing a permanent flash.

This is a plan only. DEVICE PHASE 2 did not start.

## Recovery plan for a future temporary-boot experiment

### Before the experiment

- Keep the verified USB Fastboot path and known cable/port available.
- Preserve the exact matching stock firmware archive and hashes locally.
- Have the exact stock `boot`, `recovery`, `dtbo`, and `vbmeta*` artifacts available
  for diagnosis; also retain the `super` payload and flash manifest. Their presence
  does not authorize flashing them.
- Record anti-rollback/AVB metadata from the matching package and compare it with
  the device before any persistent proposal.
- Ensure personal data is backed up by the owner. Do not treat Fastboot access as a
  data-backup mechanism.

### If temporary boot is rejected or the guest kernel fails

If and only if the image was loaded with `fastboot boot` and neither it nor its
userspace wrote persistent storage, return to bootloader Fastboot with the hardware
key sequence if necessary and use a normal reboot to the untouched stock boot
partition. A failed RAM boot does not by itself justify flashing, erasing, changing
slots, disabling AVB or relocking.

### If persistent recovery is ever needed later

Stop and identify exactly what a separately authorized future action changed. A
partition-specific restore proposal must name the affected unsuffixed partition,
the exact stock image and SHA-256, AVB/DTBO dependencies, data impact, and rollback
constraints. Never relock with modified/mismatched images. Never touch EFS-related
partitions, `persist`, modem/radio or bootloader firmware as a troubleshooting
shortcut. A full stock restore is a separate destructive operation and may wipe
data; it requires explicit authorization.

## Remaining blockers and unknowns

- Exact boot/recovery header and ramdisk format, DTB placement and DTBO selection.
- Exact AVB descriptor/key/rollback topology beyond the visible `vbmeta*`
  partitions.
- Exact connected AMOLED panel SKU and matching proprietary firmware set.
- Exact public source corresponding to running kernel ID `g6d6db67fd446`.
- Whether this exact bootloader accepts and successfully starts `fastboot boot`.
- Locally downloaded official stock archive and independently calculated SHA-256
  hashes; downloading was intentionally outside Phase 1.

## Safety record

Fastboot commands were limited to device listing, read-only `getvar` queries and
the explicitly allowed normal reboot into the existing stock OS. Authorized ADB
used only unprivileged read-only property, procfs/sysfs, `dumpsys`, configuration
and directory queries. A plain-shell read of the first 4 KiB of `boot` was denied;
no privilege escalation or bypass followed. Source inspection and web research
were host-only.

No image was booted or flashed. No partition was written, erased or formatted. The
Phase 1 commands changed no slot, lock state, AVB state, OEM setting or user data.
The owner separately enabled USB debugging and authorized this PC; that explicit
operator action is the only disclosed Android-setting change. No root, Magisk or
custom recovery was installed. The hard stop after DEVICE PHASE 1 remains in force.
