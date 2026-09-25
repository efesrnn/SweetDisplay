# Recovery baseline — exact stock artifacts verified

The host-side stock-image gate is VERIFIED in
[the immutable stock manifest](device/STOCK_IMAGE_MANIFEST.md). No stock/custom
image has been flashed. This document is a preparation checklist, not an
executable recovery recipe. Do not execute a generic restore script.

| Required recovery input | Current value |
|---|---|
| Exact model/codename/region | VERIFIED: M2101K6G, `sweet`, `sweet_tr`, Turkey |
| Installed build/fingerprint/MIUI | VERIFIED: Android 13 / MIUI 14 `V14.0.2.0.TKFTRXM`; see device inventory |
| Running kernel | VERIFIED: `4.14.190-perf-g6d6db67fd446`; pinned 4.14.180 source is not exact |
| Bootloader | VERIFIED unlocked; `secure:yes`; Android AVB state unlocked/orange, verity enforcing |
| Partition names, slots, active slot | VERIFIED non-A/B, unsuffixed `boot`/`recovery`; no active slot |
| Dynamic partitions / AVB / rollback constraints | Dynamic `super`, descriptor topology and package rollback-index values VERIFIED; bootloader enforcement details beyond observed state remain unknown |
| Boot header, DTB/DTBO, vendor boot arrangement | VERIFIED header v2, 4 KiB pages, embedded DTB and separate DTBO; no `vendor_boot.img`/`init_boot.img` in either archive |
| Matching official firmware archive and URL | VERIFIED exact Turkey Fastboot and recovery archives from Xiaomi-controlled host |
| SHA-256 archive / boot / recovery / vbmeta / dtbo / super | VERIFIED in `docs/device/STOCK_IMAGE_MANIFEST.json` |

## Preserve the verified unlock state

Do not repeat unlock, rebind, factory reset, sign out, relock, erase, or touch
EFS/modem/persist/fsg. The owner has already completed the official unlock. Keep a
user-controlled personal-data backup before any separately approved future boot
experiment. Do not automatically collect personal data.

## Enter and leave Fastboot

When needed and manually chosen: power off, hold Power + Volume Down to enter Fastboot. To leave, hold Power for more than 10 seconds if necessary, or use `fastboot reboot` deliberately. The hardware-button procedure was not exercised in this session; a normal `fastboot reboot` was. See [official Xiaomi Fastboot guidance](https://www.mi.com/my/support/faq/details/KA-550626/) and [official unlock instructions](https://www.miui.com/unlock/done.html). Do not select recovery wipe/reset options.

## Verified matching stock firmware

The installed identity is `sweet_tr`, Turkey, `V14.0.2.0.TKFTRXM`. The exact
matching Fastboot package is
`sweet_tr_global_images_V14.0.2.0.TKFTRXM_20230803.0000.00_13.0_tr_7ab6f47e67.tgz`
(5,934,114,812 bytes; SHA-256
`8367f915a4e8ca7018bc0fa3fe1a70048b38154dc0c56ec3e5870f4993dae40d`).
The exact matching recovery package is
`miui_SWEETTRGlobal_V14.0.2.0.TKFTRXM_4dd7b72cd7_13.0.zip`
(3,988,152,798 bytes; SHA-256
`60d2828c30a421b2e83b7cc4975d9ece33ae642c2b225f6eba304d8c9c5b409d`).

Both were downloaded from exact objects on Xiaomi-controlled `miui.com` hosts.
Xiaomi's [support page](https://www.mi.com/service/support) directly references
the `bigota.d.miui.com` distribution host. Exact URLs, HTTP metadata, local hashes,
image hashes and parsed structures are in the stock manifest. Public indexes were
used only for discovery/corroboration. The archives and extracted proprietary
images remain ignored/private.

Never substitute a similarly named region/build. Before relying on a later copy,
recalculate SHA-256 and require an exact manifest match. Do not run `flash_all`,
`flash_all_lock`, `flash_all_except_storage` or any other included script.

## Failed temporary boot after unlock

Unlock and identity are verified in `docs/device/DEVICE_PHASE1_RESULTS.md`. Temporary
boot is assessed LIKELY but remains unexecuted/unverified. The exact stock envelope
is now known: Android boot header v2, 4 KiB pages, gzip kernel/ramdisk, encoded load
addresses, full command line and embedded DTB. If a separately authorized future
experiment boots only a RAM image and its userspace makes no persistent writes,
return to Fastboot and reboot to the unchanged stock image. If unsupported, STOP
before any permanent flash.

## Restore stock boot after an explicitly approved flash

Not executable yet: the baseline provides exact known-good `boot`, `recovery`,
`dtbo`, `vbmeta`, `vbmeta_system` and `super` artifacts, but no restore is authorized.
Identify precisely what a future experiment changed, require the corresponding
manifest SHA-256, verify AVB dependencies and stable Fastboot, then propose only
the affected unsuffixed partition. Do not guess between partitions; the verified
layout has no active slot to change. Explain data impact and limitations before
requesting explicit approval for exact commands.

## Restore the complete phone

Not executable yet: the exact official Fastboot archive is preserved, but a full
restore still requires a separately reviewed partition plan, rollback compatibility,
backup and explicit wipe/flash approval. A full restore may erase data. Never run
the bundled scripts as an automatic recovery shortcut, and reject every option that
relocks. If bootloader Fastboot is unavailable, stop and assess official service
recovery; EDL bypasses and paid unofficial unlock services are excluded.

Every proposed destructive operation must state: what changes, why needed, exact partition(s), how recovery works, and exact stock image/hash required. Relocking with modified or mismatched regional images can prevent boot; never relock as a troubleshooting step. Mixing boot/vendor/DTBO/vbmeta across builds is not a safe recovery shortcut. See [Xiaomi bootloader FAQ](https://www.mi.com/global/support/faq/details/KA-07238/).

