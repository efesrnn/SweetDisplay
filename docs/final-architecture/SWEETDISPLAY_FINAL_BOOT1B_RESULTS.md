# SWEETDISPLAY FINAL-BOOT 1B — second RAM-only boot results

Result: **VERIFIED for the observability objective; stock recovery succeeded**.

Exactly one authorized v2 RAM-only boot was attempted. The experiment proved
kernel execution, ramdisk discovery and Android first-stage init, then localized
the failure to missing required empty ramdisk directories before SELinux and
second-stage init. No second attempt occurred. FINAL-BOOT 1 remains FAILED and
FINAL-BOOT 1A remains VERIFIED.

## 1. Owner safety confirmation

The owner explicitly confirmed adequate battery, a stable cable, physical
access, knowledge of the forced-reboot procedure, normal pre-test phone health,
readiness for a black screen/logo/backlight pulses and absence of an important
active phone operation. The owner then entered Fastboot physically and visibly
confirmed the Fastboot screen.

Authorization was limited to read-only baseline collection, one exact-v2
RAM-only `fastboot boot`, bounded physical observation and read-only post-return
capture. It did not authorize flashing.

## 2. Pre-boot stock baseline

The bounded read-only baseline began at
`2026-09-24T21:22:52.6770798+03:00` and completed at
`2026-09-24T21:22:53.4224416+03:00`.

- exactly one authorized ADB device was present;
- product was `sweet`;
- Android was 13 and MIUI was `V14.0.2.0.TKFTRXM`;
- `sys.boot_completed=1`;
- `sys.usb.config=mtp,adb` and `sys.usb.state=mtp,adb`;
- persistent USB config was `adb`;
- both exposed boot-reason properties reported `bootloader`.

No property, USB mode, setting, file, package, mount or log was changed.

## 3. Pre-existing pstore/ramoops state

`/sys/fs/pstore` existed, but ordinary enforcing ADB shell was denied directory
read access. `/proc/last_kmsg` was absent or inaccessible. The pre-boot record
inventory/content was therefore **INACCESSIBLE** and an exact byte-level
pre/post delta cannot be claimed.

No permission, SELinux state or ownership was changed and no record was cleared.

## 4. Exact v2 and identity gate

Immediately before boot, the only authorized image revalidated as:

- file: `SWEETDISPLAY-TEMP-BOOT-v2.img`;
- size: 25,673,728 bytes;
- SHA-256:
  `764c8886547dd2f3010f643ecb5789c57af3694aa4dadff91093476facc9cbd1`.

The previously validated companion hashes remained:

- diagnostic ELF, 435,848 bytes:
  `3f6d08f45de5d3878738cfbec012f3b19ae1025672bd1ecbdcf3bdf662bec174`;
- gzip ramdisk:
  `1cdd0b95f9f2bfbb1f0791783a4c1d824c4b9a03442a40d394b1280a4be0959f`;
- exact stock kernel:
  `764b10ccaf74d2fb0a4569d1254bfc21251a3351afe296d149f639c3d65c7bfc`;
- exact embedded DTB bundle:
  `8f1b614d87eb9c6835ecf56cfe6f7a338855a5317dad1eab6391222e04804d10`.

The read-only Fastboot gate observed exactly one device, product `sweet`,
`unlocked=yes` and a download bound of 805,306,368 bytes. The unique transport
identity is intentionally omitted.

## 5. T0-T11 timeline

| Point | Timestamp / evidence |
|---|---|
| T0 preflight begins | `2026-09-24T21:22:52.6770798+03:00` |
| T1 owner Fastboot confirmation recorded | `2026-09-24T21:49:13.6870133+03:00` |
| T2 exact boot command issued | `2026-09-24T21:49:13.9540435+03:00` |
| T3 download complete | approximately T2 + 0.605 s; derived from Fastboot duration, not an independent wall-clock event |
| T4 Booting result/process return | `2026-09-24T21:49:14.7528921+03:00`; `Booting OKAY [0.146s]` |
| T5 Fastboot transport check | `2026-09-24T21:49:19.5075178+03:00`; Fastboot present again |
| T6 first physical screen/pulse | NOT CAPTURED; owner reported not watching |
| T7 SweetDisplay UI | NOT OBSERVED; later evidence proves diagnostic was never executed |
| T8 transition/reboot | host proved Fastboot reappearance by T5; physical transition instant NOT CAPTURED |
| T9 stock Android owner-visible | `2026-09-24T21:53:35.0157533+03:00`, after owner physical forced reboot from Fastboot |
| T10 stock boot complete | `2026-09-24T21:53:35.7378866+03:00` |
| T11 postmortem capture | direct pstore check immediately after T10; DropBox capture began `2026-09-24T21:54:07.4577908+03:00` |

## 6. Single boot command result

The only semantic mutation was temporary download plus boot:

```text
Sending 'boot.img' (25072 KB)  OKAY [0.605s]
Booting                       OKAY [0.146s]
Finished. Total time: 0.763s
```

The process returned zero after 797.567 ms host wall time. Exactly one boot
attempt occurred. The owner's request to repeat after missing the physical
observation was declined because the one-attempt evidence and safety boundary
had already been consumed.

## 7. Physical and visible observations

The owner was not watching during the first seconds after `Booting OKAY`.
Backlight pulses are therefore **NOT OBSERVED**, not negative evidence. No pulse
count is inferred. SweetDisplay UI, partial/corrupt SweetDisplay pixels and the
`FINAL-BOOT-1B-V2` label were not observed. When the owner next looked, the phone
was on the Fastboot screen, consistent with the host-side T5 transport result.

No touch test was permitted because the v2 UI and enforcing status were not
visible. Goodix and DWC3 diagnostic results were not reached.

## 8. Stock return and health

The owner used the prepared physical forced-reboot procedure to leave Fastboot.
Normal stock Android returned successfully. Post-return read-only verification
found exactly one authorized ADB device, product `sweet`, Android 13, MIUI
`V14.0.2.0.TKFTRXM`, `sys.boot_completed=1`, healthy `mtp,adb` config/state and
no persistent SweetDisplay environment. This equals the captured pre-boot USB
baseline. Persistent USB config remained `adb`.

Physical recovery succeeded; no recovery image, flash operation or Windows
mutation was needed.

## 9. Post-return pstore recovery

Direct pstore access remained denied to ordinary shell by enforcing SELinux.
Stock `BootReceiver`, however, recorded that it copied
`console-ramoops-0` into the read-only DropBox tag `SYSTEM_LAST_KMSG`. Ordinary
shell could read that exported DropBox record without privilege changes.

The retrieved DropBox text was 317,273 characters with host-calculated SHA-256
`18ae700cab585fb7ad19d2a5b438d4722f202cecac2fb89441baff31ec85cb17`.
It contained private boot identifiers; only the sanitized failure lines below
are publishable. Because the pre-boot pstore content was inaccessible, the
complete record is not called an exact delta. The tail captured immediately
after 1B nevertheless provides a deterministic current failure sequence.

## 10. Exact failure evidence

The final relevant `SYSTEM_LAST_KMSG` segment records:

```text
[    1.721174] init: mount("tmpfs", "/mnt", ...) failed No such file or directory
[    1.721188] init: mkdir("/mnt/vendor", 0755) failed No such file or directory
[    1.721194] init: mkdir("/mnt/product", 0755) failed No such file or directory
[    1.721200] init: mount("tmpfs", "/debug_ramdisk", ...) failed No such file or directory
[    1.721216] init: Init encountered errors starting first stage, aborting
[    1.722035] init: InitFatalReboot: signal 6
[    1.744778] reboot: Restarting system with command 'bootloader'
```

There is no `SDV2` or `sweetdisplay-v2` marker. There is no relevant kernel
panic or watchdog signature. The reboot source is Android init's first-stage
fatal path, not the diagnostic's SELinux, DRM or 180-second timer path.

## 11. Manifest correlation

The private v2 manifest independently confirms:

- `/mnt`: absent;
- `/debug_ramdisk`: absent;
- `/mnt/vendor` and `/mnt/product`: absent because their parent was absent;
- `/dev`, `/proc`, `/sys`, `/tmp`, `/linkerconfig` and `system/bin/init`: present.

The exact stock recovery ramdisk contains empty root-owned mode-0755 `/mnt` and
`/debug_ramdisk` directory entries. Its init creates `/mnt/vendor` and
`/mnt/product` only after mounting tmpfs on `/mnt`. The minimal ramdisk builder
excluded `/debug_ramdisk` and incorrectly treated the empty `/mnt` mountpoint as
a forbidden persistent path. No persistent partition was involved; the missing
objects are RAM-backed mountpoint directories required by first-stage init.

## 12. Checkpoint localization

Highest proven checkpoint: **C3 ENTRY — Android first-stage init executing**.

Logical proof:

- C0: Fastboot `Booting OKAY` — proven;
- C1: exact stock kernel execution — proven by ramoops kernel console;
- C2: v2 ramdisk discovery and PID-1 path — proven by execution of its retained
  `/system/bin/init` and its missing-directory topology;
- C3: Android first-stage init entered — proven;
- C3 completion: **FAILED** at 1.721 seconds;
- C4 SELinux initialized enforcing: not reached/proven;
- C5 `late-init`: not reached;
- C6 diagnostic exec: not reached;
- C7-C12 diagnostic checkpoints: not reached.

First failed transition is **C3 first-stage init completion**. First unproven
checkpoint is **C4**. DRM, Goodix, DWC3 and tagged diagnostic reboot remain
outside the reached boundary, so no conclusion about those implementations is
drawn from 1B.

## 13. Classification

FINAL-BOOT 1B is **VERIFIED** under its primary observability acceptance:

- exact v2 and intended device gates passed;
- one RAM-only attempt ran;
- reliable post-return ramoops evidence substantially narrowed the boundary;
- the exact first-stage failure and reboot source were recovered;
- stock Android and the pre-boot USB state returned healthy;
- no partition-write command or prohibited operation occurred.

Visible UI was not required for this diagnostic classification. This result does
not promote FINAL-INPUT or FINAL-USB and does not relabel FINAL-BOOT 1.

## 14. Safety and next phase

**Partition-write commands issued: NO.** No flash, erase, format, update, slot,
lock, AVB/vbmeta, verification, alternate image, second boot, live rebuild,
root, SELinux weakening, push/install, ConfigFS, NCM, IP/TCP, HID, media, camera,
Windows driver/network/security, commit or push operation occurred.

The one recommended next phase is:

**SWEETDISPLAY FINAL-BOOT 1C — OFFLINE FIRST-STAGE RAMDISK DIRECTORY-CLOSURE
FIX AND V3 VALIDATION.**

It should remain host-only: restore the exact stock `/mnt` and `/debug_ramdisk`
directory entries, correct the builder's mountpoint policy, audit the remaining
first-stage directory closure and create/validate a new deterministic artifact.
It must not execute another live boot or weaken SELinux. No part of 1C was begun
in this phase.

## 15. Files and hygiene

This phase created
`docs/final-architecture/SWEETDISPLAY_FINAL_BOOT1B_RESULTS.md` and updated only
`STATUS.md`, `docs/ROADMAP.md`, `docs/TEST_LOG.md` and
`docs/ARCHITECTURE.md`. It did not change diagnostic, ramdisk-builder or image
sources and did not create a replacement artifact.

`git diff --check` passed with only normal line-ending notices. The publication
checker passed for 239 publishable working-tree files, the index and 125
reachable history blobs. Targeted sanitization found no unique device identity,
USB instance identity, boot serial/PARTUUID, MAC, ADB key, personal path, private
key, firmware/image payload or raw pstore record in public changes. Generated
images/binaries and prior private evidence remain ignored. No commit or push
occurred.
