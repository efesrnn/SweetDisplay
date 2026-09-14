# Recovery preparation — BLOCKED pending exact identity

No stock/custom image has been flashed. This document is a preparation checklist, not a verified recovery recipe. Do not execute a generic restore script.

| Required recovery input | Current value |
|---|---|
| Exact model/codename/region | Target Redmi Note 10 Pro; exact variant must be verified |
| Installed build/fingerprint/MIUI | Unknown |
| Running kernel | Unknown; published candidate source is 4.14.180 |
| Bootloader | Consult private operator state; verify before any hardware work |
| Partition names, slots, active slot | Unknown |
| Dynamic partitions / AVB / rollback constraints | Unknown |
| Boot header, DTB/DTBO, vendor boot arrangement | Unknown |
| Matching official firmware archive and URL | Not selected; exact build must be identified first |
| SHA-256 archive / boot / vbmeta / dtbo / vendor images | None downloaded; hashes not available |

## Preserve the waiting period

Do not rebind, factory reset, sign out, relock, erase, bypass unlock, or touch EFS/modem/persist/fsg. Owner performs official Mi Unlock only when the timer expires; final unlock may erase user data. Arrange a user-controlled personal-data backup before then. Do not automatically collect personal data.

## Enter and leave Fastboot

When needed and manually chosen: power off, hold Power + Volume Down to enter Fastboot. To leave, hold Power for more than 10 seconds if necessary, or use `fastboot reboot` deliberately. These are documented Xiaomi procedures, not executed in this session. See [official Xiaomi Fastboot guidance](https://www.mi.com/my/support/faq/details/KA-550626/) and [official unlock instructions](https://www.miui.com/unlock/done.html). Do not select recovery wipe/reset options.

## Locate matching stock firmware

First capture installed fingerprint, product, region and build incremental ID. Seek that exact fastboot firmware from Xiaomi's official support/download channels; official unlock entry point is [Mi Unlock](https://en.miui.com/unlock/download_en.html). The unlock download is NOT a firmware package. No verified exact stock-ROM URL exists for this device yet. If the exact build is unavailable, stop and resolve compatibility with authoritative evidence; do not choose a similarly named regional ROM or an arbitrary newest build.

Record original URL, download date, exact archive filename, model/region/build, size and SHA-256. Extract locally; inspect archive scripts without running them. Hash the actual boot and relevant companion images using `Get-FileHash -Algorithm SHA256`. Hashes identify local files; they are not authenticity proof without a trusted reference. Keep stock firmware and proprietary blobs ignored/local-only.

## Failed temporary boot after unlock

Verify unlock with read-only queries and update STATUS first. Investigate whether this device actually accepts temporary boot. If an experiment booted only a RAM image and made no persistent writes, return to Fastboot and reboot to the unchanged stock image. This route is conditional on proven temporary-boot support and no userspace writes. If unsupported, STOP before any permanent flash.

## Restore stock boot after an explicitly approved flash

Not executable yet: the affected partition/slot and exact stock boot image are unknown. Identify precisely what was modified, obtain that installed build's matching image set, verify hashes and AVB/vendor compatibility, confirm stable Fastboot, and propose a partition-specific restore. Do not guess `boot`, `boot_a` or `vendor_boot`; do not change slots to experiment. Explain data impact and recovery limitations before requesting explicit approval for exact commands.

## Restore the complete phone

Not executable yet: requires correct official fastboot archive, inspected flash manifest/scripts, partition/slot map, verified region and rollback compatibility, backup and explicit wipe/flash approval. A full restore may erase data. Reject any tool/script option that automatically relocks. If bootloader Fastboot is unavailable, stop and assess official service recovery; EDL bypasses and paid unofficial unlock services are excluded.

Every proposed destructive operation must state: what changes, why needed, exact partition(s), how recovery works, and exact stock image/hash required. Relocking with modified or mismatched regional images can prevent boot; never relock as a troubleshooting step. Mixing boot/vendor/DTBO/vbmeta across builds is not a safe recovery shortcut. See [Xiaomi bootloader FAQ](https://www.mi.com/global/support/faq/details/KA-07238/).

