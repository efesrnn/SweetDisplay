# SWEETDISPLAY FINAL-BOOT 1 results

Date: 2026-09-24  
Classification: **FAILED — stock recovery succeeded**

## Scope and outcome

This was the first and only authorized live RAM-only temporary-boot attempt.
The Android bootloader accepted both the download and the `boot` command, but
the owner observed normal stock Android instead of the SweetDisplay diagnostic.
No SweetDisplay-owned pixels or other custom-environment milestone was observed.
The phone returned automatically to the expected stock Turkey build and did not
require physical recovery.

This is FAILED under the phase definition: the bootloader accepted the command,
the custom environment produced no useful visible milestone, and stock recovery
succeeded. It is not evidence that the image was flashed or persisted.

## Owner and recovery gate

Before any device command, the owner explicitly confirmed sufficient battery,
a stable USB cable, physical access, a normally behaving phone, no important
operation in progress, readiness for a black screen or frozen boot logo, and
knowledge of the greater-than-ten-second Power-button forced-reboot procedure.
The owner then powered the phone into the visible Fastboot screen with the
physical buttons. `adb reboot bootloader` was not used.

## Candidate validation

The exact ignored/private PREP 1 candidate was resolved and checked immediately
before the live command.

| Property | Observed |
|---|---|
| File | `SWEETDISPLAY-TEMP-BOOT-v1.img` |
| Size | 25,673,728 bytes |
| SHA-256 | `af7ff18d9fa87929b6eff1949180f9ea5788fa0d079897222d218c356852af9e` |
| Magic / header | `ANDROID!`, version 2, header size 1,660 |
| Page size | 4,096 bytes |
| OS / patch | 13.0.0 / 2023-08 |
| Kernel | 18,156,887 bytes; stock SHA-256 `764b10ccaf74d2fb0a4569d1254bfc21251a3351afe296d149f639c3d65c7bfc` |
| Ramdisk | 4,228,394 bytes; SHA-256 `d8e3af020ecba694cc9bb1ab99a463de9e6ab5f82a09f7954b5f82ed3c985ef5` |
| Embedded DTB | 3,276,863 bytes; stock SHA-256 `8f1b614d87eb9c6835ecf56cfe6f7a338855a5317dad1eab6391222e04804d10` |
| AVB footer | absent, as designed |

No substitution or metadata mismatch was found. The candidate remains private,
ignored, unsigned and prohibited from flashing.

## Tool and identity gate

The previously documented Android SDK Platform-Tools binary was used without a
download or update: Fastboot `36.0.2-14143358`, executable SHA-256
`561bec25f791d2ee75ed57ac84a09960c9fadb62f531240a6896fba1cb091d1d`.

The bounded read-only gate observed exactly one Fastboot device. Its unique
identifier was not retained in public text. Product was `sweet`, unlocked state
was `yes`, and maximum download size was 805,306,368 bytes. The 25,673,728-byte
candidate was within that bound.

## Single live attempt

The reviewed semantic operation was **TEMPORARY DOWNLOAD + BOOT ONLY**. The one
live command was equivalent to:

```text
fastboot boot <exact validated SWEETDISPLAY-TEMP-BOOT-v1.img>
```

There was no partition name and no `flash`, `erase`, `format`, `update`, slot,
lock, unlock, OEM, vbmeta or verification operation. The candidate size and
SHA-256 were checked again in the same command block before Fastboot ran.

Host result:

```text
Sending 'boot.img' (25072 KB)  OKAY [0.614s]
Booting                       OKAY [0.146s]
Finished. Total time: 0.771s
```

Fastboot exit code was zero. This proves command acceptance; it does not by
itself prove how far the downloaded kernel/userspace executed. Exactly one boot
attempt occurred. No retry, alternate image, rebuild or recovery image was used.

## Physical observation

During the bounded first observation, the owner reported that the normal Android
interface opened. The SweetDisplay custom-environment heading and diagnostic UI
were never seen. No useful black-screen or boot-logo timing was retained.

| Gate | Result |
|---|---|
| SweetDisplay UI within 60 seconds | FAILED / not observed |
| Display diagnostic | FAILED / no SweetDisplay-owned pixels observed |
| SELinux diagnostic | UNKNOWN / UI absent |
| Goodix discovery | UNKNOWN / UI absent |
| Touch coordinates | NOT TESTED; touch was correctly not attempted |
| DWC3 diagnostic | UNKNOWN / UI absent |
| Automatic diagnostic reboot | UNKNOWN; stock Android returned automatically |
| Physical recovery used | NO |

No ADB command was issued while the temporary image could have been running.
No USB gadget, network, media or later-phase test was attempted.

## Stock return

After the owner confirmed the normal Android interface, bounded read-only ADB
queries observed exactly one authorized device in `device` state, Android 13,
`boot_completed=1`, and build `V14.0.2.0.TKFTRXM`. This matches the expected
stock Turkey build and no SweetDisplay environment persisted across the return.

The post-return USB properties were `sys.usb.config=adb`,
`sys.usb.state=adb` and `persist.sys.usb.config=adb`. The expected historical
`mtp,adb` development baseline was therefore not observed. The corresponding
pre-Fastboot values were not captured in this phase, so causation or a persistent
change cannot be attributed to the temporary-boot attempt. No USB property was
written. A read-only `svc usb getFunctions` query was unusable because Xiaomi
framework initialization raised a missing theme-configuration-file error; it
made no state change.

Stock OS/build/ADB recovery is VERIFIED. Exact MTP-baseline equivalence remains
UNKNOWN. No physical recovery, install, push or settings change occurred.

## Safety and persistence statement

**NO PARTITION-WRITE COMMAND WAS ISSUED.** The complete live Fastboot command
history for this phase contains only read-only identity queries followed by the
single `fastboot boot` operation. No flash/erase/format/update/slot/lock/OEM/AVB
command occurred. The private candidate has no fstab, persistent mount path or
partition tool, and the expected stock build returned. These facts support the
bounded no-write-command statement, not a stronger claim about every internal
bootloader implementation detail.

No root, Magisk, custom recovery installation, SELinux change, ConfigFS/UDC,
NCM/IP/TCP, HID, media, camera, Windows driver/security mutation, commit or push
occurred. FINAL-INPUT 1 and FINAL-USB 1 were not begun.

## Remaining unknowns and next gate

The point of failure is UNKNOWN. The evidence does not distinguish a handoff
failure, kernel/init/policy failure, diagnostic startup failure, DRM timeout,
watchdog/reboot path or another early-boot cause. Fastboot's `Booting OKAY` is
not proof that custom userspace reached `init`, and the deliberately absent ADB
or console path produced no live diagnostic log.

The one recommended next phase is a separately authorized, host-only
**FINAL-BOOT 1A offline early-boot failure diagnosis**. It should inspect the
existing candidate's init/SELinux/reboot/DRM failure paths and design a bounded
non-persistent evidence channel before proposing any new live attempt. It must
not begin FINAL-INPUT 1 or FINAL-USB 1 and must not authorize a retry by itself.

## Files and hygiene

This result created this document and updated `STATUS.md`, `docs/ROADMAP.md`,
`docs/TEST_LOG.md` and `docs/ARCHITECTURE.md`. Raw/private artifact and device
identity data remain ignored. `git diff --check` passed with only normal
line-ending notices. The publication checker passed for 234 publishable
working-tree files, the index and 125 reachable history blobs. No unique
Fastboot identity, personal path, MAC address, ADB key, raw private evidence or
firmware/image payload entered publishable content. No commit or push was
performed.
