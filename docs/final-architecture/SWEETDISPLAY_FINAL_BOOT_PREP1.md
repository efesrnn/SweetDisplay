# SWEETDISPLAY FINAL-BOOT PREP 1 — offline artifact and recovery gate

Date: 2026-09-24. Result: **VERIFIED** for the offline preparation scope.
Decision: **GO FOR SEPARATELY AUTHORIZED FINAL-BOOT 1**.

This result does not claim that the phone has booted the image. No ADB command,
Fastboot device command, `fastboot boot`, flash, partition write, phone reboot,
USB mutation or other live-device operation occurred. The candidate and all
proprietary inputs remain ignored/private.

> **MIUI + Android receiver APK is the development/validation platform. It is
> not the final SweetDisplay product architecture.**

## Safety result

The prepared design credibly preserves the intended invariant:

```text
no partition write + downloaded RAM-only boot + reset/reboot
  = bootloader loads the unchanged stock boot partition
```

The candidate ramdisk contains no block device, fstab, `/data`, `/metadata`,
`/cache`, updater, shell, ADB, fastbootd, vold, filesystem repair/format tool or
partition utility. Its rc graph mounts only tmpfs at `/tmp`. The diagnostic does
not open block devices and has no persistence feature. It uses the reboot syscall
after a bounded three-minute observation. A physical Power-button reset is the
independent fallback. No slot or boot-control metadata is used; the verified
layout is non-A/B.

This invariant still depends on a future command being a genuine downloaded
temporary boot. If the exact bootloader rejects that operation, asks for a write,
or behaves ambiguously, FINAL-BOOT 1 must stop without substituting flash.

## Exact source provenance and stock envelope

The source is the already verified official Xiaomi Turkey build for `sweet_tr`:

- build: Android 13 / MIUI 14 `V14.0.2.0.TKFTRXM`;
- Fastboot archive:
  `sweet_tr_global_images_V14.0.2.0.TKFTRXM_20230803.0000.00_13.0_tr_7ab6f47e67.tgz`;
- archive SHA-256:
  `8367f915a4e8ca7018bc0fa3fe1a70048b38154dc0c56ec3e5870f4993dae40d`;
- stock `boot.img`: 134,217,728 bytes, SHA-256
  `0a9dfe4d30b600115a5484fde873cdd293c3cc99683aebcdbcc3b9176b17a8f3`;
- source recovery ramdisk: 18,412,603 bytes, SHA-256
  `cbcc0220289d4959a32fa558baf04ff2c9756a53265d5a587349cee9c3ed7258`.

No other region or build was substituted. The exact stock boot layout is:

| Field | Value |
|---|---|
| Magic / header | `ANDROID!`, header v2, header size 1,660 |
| Page size | 4,096 bytes |
| OS / patch | 13.0.0 / 2023-08 |
| Kernel | gzip, 18,156,887 bytes |
| Kernel load address | `0x00008000` |
| Ramdisk | gzip, 812,101 bytes |
| Ramdisk load address | `0x01000000` |
| Second stage | absent |
| Tags address | `0x00000100` |
| Embedded DTB | 3,276,863 bytes at `0x01f00000` |
| Stock boot body / AVB original image size | 22,257,664 bytes |
| Partition-padded stock image | 134,217,728 bytes |

The complete preserved command line is:

```text
androidboot.hardware=qcom androidboot.memcg=1 lpm_levels.sleep_disabled=1 video=vfb:640x400,bpp=32,memsize=3072000 msm_rtb.filter=0x237 service_locator.enable=1 swiotlb=1 androidboot.usbcontroller=a600000.dwc3 loop.max_part=7 buildvariant=user
```

`vendor_boot.img` and `init_boot.img` do not apply to this package. The external
`dtbo` partition remains untouched. The candidate embeds the exact stock DTB
bundle, while the unchanged device DTBO is expected to keep selecting runtime
entry 12 (`SWEET`, `qcom,sdmmagpie-qrd`, MSM ID 365). Exact panel SKU remains
unknown and is not hardcoded by the diagnostic.

## STOCK-ROUNDTRIP

The stock image was unpacked with the pinned AOSP Android 13 r36 tool and rebuilt
without changing kernel, ramdisk, DTB, addresses, page size, version fields,
board field or command line.

| Component | SHA-256 |
|---|---|
| Stock compressed kernel | `764b10ccaf74d2fb0a4569d1254bfc21251a3351afe296d149f639c3d65c7bfc` |
| Stock compressed ramdisk | `5e02454615d31c838d47e2877e66beea0b141fb01a7d3eecac907c6428e100b1` |
| Embedded DTB bundle | `8f1b614d87eb9c6835ecf56cfe6f7a338855a5317dad1eab6391222e04804d10` |
| `STOCK-ROUNDTRIP` body | `c810a47ed05a06e0c06792e65184a453c402f6ff52c71816d73e92bb7f050e12` |

`STOCK-ROUNDTRIP` is 22,257,664 bytes and is byte-identical to the first
22,257,664 bytes of the original stock image. The original then has 111,960,064
additional bytes: its unsigned AVB descriptor/footer placement and partition
padding. The host round-trip intentionally did not copy/sign/recreate that tail.
This fully explains the file-size and whole-image hash difference while proving
that `mkbootimg` reproduced the complete semantic boot body exactly.

`STOCK-ROUNDTRIP` is a private host-tool validation artifact and must never be
flashed or booted.

## Host toolchain

| Tool | Pin / version | SHA-256 where recorded |
|---|---|---|
| AOSP `mkbootimg` | `android-platform-13.0.0_r36`, commit `3d9fe8f98b6a9ae9e9e79ab5a8157a73c7fe9b23` | `mkbootimg.py`: `d99136f30bda966e8820c8ae53a82c659ca36e6d1aaf49a4cd63ae4795a6845a`; `unpack_bootimg.py`: `57164c0a980f4140e7206f056532c758357e7cfb961154d81aeb099d1dad817d` |
| AOSP AVB | `android-platform-13.0.0_r36`, commit `7d1b74cba970b64c83a01f43be257fa8f9096d19` | `avbtool.py`: `bfa03219fd84c44e864b6b249d8e125219e780e97e7e9fb5d8c4f5c37c329919` |
| AOSP libufdt | `android-platform-13.0.0_r36`, commit `12d91faf02975b5d00676ab938e1ea94ab3aaee9` | used for retained DTBO evidence, not candidate mutation |
| Python | 3.12.14 | local bundled runtime |
| Android NDK | r28c / 28.2.13676358, Clang 19.0.1 | `clang++.exe`: `1c7792ef6df195b6af5919071a9ca7850bcb850153f02bed9c7d642e266c4ff3`; `llvm-readelf.exe`: `2646e382d8b2559b5c4ae91e9a98e187ed9bd59f23936054d424049ef795ba5e`; `llvm-strip.exe`: `2d411db64e0b45508775e9e8fd436fb6e0af814a9dfbf8aea8fc37ce436c435c` |

No opaque binary was fetched. No Windows security, driver or trust setting was
changed.

## Minimum first-boot environment

### Required now

| Area | Exact first-boot requirement |
|---|---|
| Early boot | exact stock kernel/DTB/header/cmdline; exact stock recovery `init`, linker and recursive 33-library init closure |
| Display | built-in DRM/MSM/SDE/DSI kernel path; `/dev/dri/card*`; one DRM dumb XRGB8888 buffer; observed panel-backlight sysfs path |
| Touch | built-in Goodix evdev; passive `/dev/input/event*` discovery by name and MT capabilities |
| USB detection | read-only DWC3/UDC sysfs presence only |
| Security | exact compiled stock recovery SELinux policy and contexts; explicit recovery service domain; UI reads enforcement state |
| Temporary state | ramdisk plus tmpfs `/tmp` only |
| Exit | direct normal reboot syscall; physical long-Power fallback |

### Not required yet

Telephony, Wi-Fi, Bluetooth, audio, camera, Java, Zygote, `system_server`,
PackageManager, Launcher, SystemUI, Binder services, SurfaceFlinger, HWC,
gralloc, EGL/GLES, MediaCodec, VIDC userspace, NCM, DHCP, TCP, HID, ADB, MTP,
FunctionFS, userdata, FBE unlock and persistent settings are excluded.

## First-boot display decision

The selected path is **direct DRM/KMS dumb-buffer diagnostic rendering**.

- A legacy framebuffer path is rejected: the exact runtime inventory had no
  `/dev/fb*` and exposed DRM/SDE DSI through `/dev/dri/card0` and DSI-1.
- Minimal SurfaceFlinger plus vendor HWC/gralloc/EGL is still the selected final
  accelerated UI, but its service/property/vendor closure is not yet proven and
  would make the first boot larger.
- The exact stock kernel has DRM/MSM/MIPI-DSI/SDE enabled. The stock Xiaomi
  recovery environment independently contains graphics/input recovery support,
  making direct early display ownership credible without mounting `super`.

The diagnostic requests DRM master, selects the first connected connector and
mode, resolves its encoder/CRTC, allocates a 32-bpp dumb buffer, performs legacy
KMS `ADDFB`/`SETCRTC`, and renders a built-in bitmap font. Qualcomm downstream
KMS behavior remains live-unverified; failure to acquire master, create a dumb
buffer or modeset is a bounded FINAL-BOOT 1 failure, not a reason to flash.

## Dependency closure and private ramdisk

The private ramdisk is built deterministically from the exact stock recovery
ramdisk but does **not** retain the recovery UI or tool suite. It has 66 entries:

- exact stock recovery `init`, `ueventd` symlink and `linker64`;
- the exact recursive 33-library dependency closure of stock `init`;
- exact monolithic recovery `sepolicy`, root file/property contexts and default
  recovery properties;
- reviewed SweetDisplay rc and ueventd rules;
- one statically linked AArch64 SweetDisplay diagnostic at the policy-compatible
  `/system/bin/recovery` path; and
- only the directories needed for `/dev/dri`, `/dev/input`, proc/sys exposure,
  linker configuration and tmpfs.

The stock recovery `init` hash is
`8bb1fd497c77112f9cc4fb7a3d85a656c8243917712c0af6b2bd224d8e8b520d`.
Every retained shared library exists solely because it is in that executable's
recursive ELF `NEEDED` closure. The ramdisk has exactly three executable regular
files: `init`, `linker64` and the SweetDisplay diagnostic. It has no device node.

The stock recovery policy is proprietary and remains private. Reusing its
`u:r:recovery:s0` domain is an explicitly bounded first-diagnostic exception,
not the final product policy. It is chosen because the exact policy already
supports recovery display, input and reboot while remaining enforcing. A later
product artifact must define a dedicated least-privilege SweetDisplay domain.
No permissive switch or phone-policy mutation exists.

The runtime UI displays `SELINUX: ENFORCING`, `NOT ENFORCING` or `UNKNOWN` from
the read-only selinuxfs enforcement flag. Only `ENFORCING` can satisfy the live
gate.

## Dynamic partitions, userdata and firmware

The first diagnostic needs no logical-partition mapping. It does not mount
`system`, `vendor`, `product`, `odm` or `system_ext` from `super`; its small
`/system` tree is entirely within the ramdisk. Therefore no dm-linear, dm-verity,
metadata-slot selection or read-only super mount is required for FINAL-BOOT 1.
Those mechanisms return in FINAL-MEDIA/FINAL-UI when the exact vendor graphics
and media stacks are introduced.

There is no fstab in the candidate. `/data`, `/metadata`, `/cache` and persistent
mount points are absent. No FBE key or user app is accessed. `/tmp` is tmpfs.
No stock partition is mounted even read-only in this milestone.

The direct kernel display, Goodix and DWC3 paths are expected not to require a
new userspace firmware load. Xiaomi's stock recovery already operates its early
UI/touch without the normal MIUI service graph, which supports this design, but
exact firmware requests remain a live observation. A missing firmware request
is a failure to document and redesign offline; it does not authorize mounting
or copying more vendor content during the live test.

## Native diagnostic implementation

`sweetdisplay_diag.cpp` is implemented and builds cleanly with `-Wall -Wextra
-Werror` as a stripped, static ELF64/AArch64 executable:

- size: 433,096 bytes;
- SHA-256: `dc0832544cf5108e50cae7df69cb090b94a14ebe2807224c4e280c3bbc188ddc`;
- no dynamic section or shared-library dependency;
- no Java, APK, Binder, vendor library or network dependency.

The screen is SweetDisplay-owned and contains:

```text
SWEETDISPLAY
CUSTOM ENVIRONMENT
DISPLAY: OK
TOUCH: WAITING / x / y / UNKNOWN
USB CONTROLLER: DETECTED / UNKNOWN
SELINUX: ENFORCING / NOT ENFORCING / UNKNOWN
BUILD: FINAL-BOOT-PREP1
AUTO REBOOT: <seconds>
```

It waits up to 30 seconds for DRM nodes, retries Goodix and DWC3 discovery,
auto-reboots after 180 seconds, and reboots after a bounded display-init failure.

Touch discovery does not hardcode `event4`. It enumerates `event0` through
`event63`, requires `EV_ABS` plus `ABS_MT_POSITION_X/Y`, and prefers a name
containing `goodix` or `touch`. It reads per-axis min/max with `EVIOCGABS` and
scales to the active DRM mode. It consumes MT position/tracking/SYN events
without grabbing the device or injecting input. Exact slot ordering, wake and
orientation behavior remain live unknowns.

DWC3 observation uses only read-only presence checks for the known
`a600000.dwc3` platform/sysfs paths or an entry in `/sys/class/udc`. It does not
mount ConfigFS, create a function, bind a UDC or expose any USB gadget.

## SWEETDISPLAY-TEMP-BOOT-v1

The private candidate was created and re-unpacked successfully:

| Item | Size | SHA-256 |
|---|---:|---|
| `SWEETDISPLAY-TEMP-BOOT-v1.img` | 25,673,728 | `af7ff18d9fa87929b6eff1949180f9ea5788fa0d079897222d218c356852af9e` |
| Custom gzip ramdisk | 4,228,394 | `d8e3af020ecba694cc9bb1ab99a463de9e6ab5f82a09f7954b5f82ed3c985ef5` |
| Diagnostic ELF | 433,096 | `dc0832544cf5108e50cae7df69cb090b94a14ebe2807224c4e280c3bbc188ddc` |

Two consecutive builds produced identical ramdisk and candidate hashes. The
candidate parses as header v2 with the exact original page size, addresses,
OS/patch fields, command line, kernel hash and DTB hash. Its 25.7-MB size is well
inside both the 128-MiB stock boot envelope and verified 768-MiB Fastboot maximum
download size.

The validator confirms 66 allowlisted ramdisk entries, exact diagnostic hash,
no unexpected executable, no device node, no persistent path, no block image,
no private-key/ADB-key marker and no personal host path. The candidate contains
no AVB footer, private key or signature. It is not a stock-signed image and must
never be flashed.

## AVB and temporary-boot feasibility

### KNOWN

- The bootloader is unlocked; `secure:yes` and unlocked/orange Android state
  were observed.
- The device is non-A/B, has a dedicated unsuffixed 128-MiB `boot`, separate
  `dtbo`, signed top-level `vbmeta`, and a 768-MiB Fastboot download limit.
- Stock top-level vbmeta binds the **partition** `boot` hash. It was not modified.
- The candidate is unsigned and has no AVB footer. No vbmeta, flag, rollback
  index, key, partition or boot-control metadata was changed.
- A host `fastboot boot <image>` operation, if supported as specified, downloads
  and boots RAM content rather than flashing a named partition.

### LIKELY

- An unlocked Xiaomi bootloader may permit a downloaded unsigned custom image
  and a physical reset should then return to the unchanged stock boot partition.
- The exact stock kernel/DTB/header envelope reduces incompatibility risk.

### UNKNOWN

- Whether this exact bootloader implements and accepts `fastboot boot` for this
  header-v2 unsigned image.
- Whether it applies a downloaded-image AVB policy that rejects the absent
  footer or custom ramdisk.
- Whether downstream DRM accepts this minimal legacy KMS modeset before vendor
  userspace, and whether recovery SELinux permissions cover every selected ioctl.

Classification: **TECHNICALLY PLAUSIBLE / LIVE UNVERIFIED**. These bounded
unknowns are permitted by the offline GO rule. Rejection is a clean failure;
adding a signature, disabling AVB or flashing is not an allowed workaround.

## Future temporary-boot procedure — every command NOT EXECUTED

This is a procedure for a separately authorized FINAL-BOOT 1 only. It is not an
instruction to execute in this phase.

1. Owner is present; phone is physically accessible; battery/cable are stable;
   no unrelated phone or USB task is running. Keep the exact stock manifest and
   Power-button recovery checklist open.
2. Owner powers off and physically holds **Power + Volume Down** to enter
   Fastboot. Do not enter recovery menus or select wipe/reset.
3. From the repository root, prepare host variables:

   ```powershell
   # FUTURE / NOT EXECUTED
   $Fastboot = Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\fastboot.exe'
   $Image = (Resolve-Path 'docs\evidence\private\final-boot-prep1\candidate\SWEETDISPLAY-TEMP-BOOT-v1.img').Path
   ```

4. Verify the private candidate exactly:

   ```powershell
   # FUTURE / NOT EXECUTED
   (Get-FileHash -LiteralPath $Image -Algorithm SHA256).Hash.ToLowerInvariant()
   # Require: af7ff18d9fa87929b6eff1949180f9ea5788fa0d079897222d218c356852af9e
   ```

5. Perform bounded, read-only Fastboot presence/identity checks. Raw output is
   private because `devices` may contain unique identity:

   ```powershell
   # FUTURE / NOT EXECUTED
   & $Fastboot devices
   & $Fastboot getvar product
   & $Fastboot getvar unlocked
   & $Fastboot getvar max-download-size
   ```

   Require exactly one intended physical phone, product `sweet`, unlocked `yes`,
   and a sufficient download limit. Any ambiguity is a hard stop.

6. The only mutating-in-RAM operation proposed is:

   ```powershell
   # FUTURE / NOT EXECUTED
   & $Fastboot boot $Image
   ```

   There is no flash/erase/format/set-active/lock command.
7. Allow at most 60 seconds for the diagnostic UI. Do not issue another command,
   move the cable or interact with an unexpected recovery/MIUI screen.
8. If the UI appears, require `DISPLAY: OK`, `SELINUX: ENFORCING`, changing
   Goodix coordinates and `USB CONTROLLER: DETECTED`. Observe only. Do not expect
   ADB/MTP/NCM; the candidate exposes no USB gadget.
9. Allow the 180-second auto-reboot, or use the physical long-Power reset after
   evidence is complete. Require normal untouched MIUI, then stop FINAL-BOOT 1.

## Failure and recovery matrix

| Case | Bounded action | Expected return |
|---|---|---|
| A. Fastboot refuses image | Record error privately; do not alter/sign/flash; select Start or hold Power >10 s | Normal stock boot |
| B. Download fails | Stop; do not loop retries until cable/state is known; long Power | Fastboot or normal stock boot |
| C. Temporary boot returns error | Treat as unsupported; no alternate boot/flash command | Normal stock boot |
| D. Black screen | Wait at most 60 s; then hold Power >10 s | PMIC reset then stock boot |
| E. Boot logo freezes | Wait at most 60 s; long Power | Stock boot |
| F. Diagnostic UI absent | Same bounded wait/reset; preserve only host error | Stock boot |
| G. Touch unavailable | Record `UNKNOWN`; do not add firmware/mounts live; auto/physical reboot | Stock boot; redesign offline |
| H. Display corrupted | Do not touch screen; physical long Power | Stock boot; DRM path remains failed |
| I. Environment hangs | Physical long Power; if needed re-enter Fastboot with Power + Volume Down | Select normal Start; no flash |
| J. USB unavailable after boot | Expected because no gadget is configured; use physical controls, not USB recovery | Auto/physical reboot |
| K. Host loses Fastboot | Expected after a successful RAM boot; if no UI by timeout, physical reset | Stock boot |
| L. Phone appears unresponsive | Hold Power >10 s; if necessary Power + Volume Down for Fastboot; stop if hardware reset does not recover | Normal boot/official service assessment; never flash as gate recovery |

None of these cases has a planned flash recovery. If the owner cannot reliably
reach normal boot or Fastboot with hardware keys, the live gate is not ready and
must not begin.

## FINAL-BOOT 1 acceptance and hard stops

FINAL-BOOT 1 can pass only if all of the following are directly observed:

1. exact candidate hash is verified before the command;
2. bootloader accepts the single downloaded temporary-boot command without a
   write prompt or partition operation;
3. SweetDisplay-owned UI appears within 60 seconds;
4. UI reports display OK and SELinux enforcing;
5. Goodix touch produces plausible changing coordinates;
6. DWC3 is detected read-only;
7. no stock partition, userdata, metadata or USB/network function is touched;
8. bounded auto/physical reboot occurs; and
9. unchanged stock MIUI boots normally.

Hard stop on any hash mismatch, wrong product/device count, locked/ambiguous
bootloader state, request for flash/write/verification disable, image rejection,
unexpected stock recovery/wipe UI, non-enforcing SELinux, missing/corrupt display,
unsafe thermal/power behavior, timeout, failed physical recovery, or uncertainty
about the final stock state. A failure never authorizes flash, vbmeta change,
alternate signing, custom recovery or a second experimental image.

## Result boundaries and remaining unknowns

This offline gate verifies the stock round-trip toolchain, exact-kernel/DTB
preservation, a deterministic private candidate, a bounded ramdisk/service
closure, no-persistence design, offline structure and owner recovery procedure.
It does not verify a single live boot behavior.

Important remaining unknowns are the exact bootloader's RAM-boot/AVB decision,
early enforcing-policy success, downstream DRM master/dumb-buffer/modeset
behavior, panel brightness/order, Goodix raw MT orientation and direct reboot
permission. Final SurfaceFlinger/HWC/vendor graphics closure, read-only dynamic
partition access, NCM, media decode, HID and camera remain later phases.

## Files and hygiene

Public source/documentation added or updated by this gate:

- `device/final/diagnostic/sweetdisplay_diag.cpp`;
- `device/final/diagnostic/init.sweetdisplay.rc`;
- `device/final/diagnostic/ueventd.sweetdisplay.rc`;
- `device/final/diagnostic/README.md`;
- `device/final/tools/build_ramdisk.py`;
- `scripts/windows/Build-FinalDiagnostic.ps1`;
- `scripts/windows/Build-FinalTempBoot.ps1`;
- this document, `STATUS.md`, `docs/ROADMAP.md`, `docs/TEST_LOG.md` and
  `docs/ARCHITECTURE.md`.

Generated ELF, stock-roundtrip image, ramdisk, candidate image, extracted stock
files and raw validation outputs are under ignored `out/` or
`docs/evidence/private/`. Git ignore checks explicitly resolve them to existing
ignore rules. No proprietary blob, compiled policy, firmware, boot image, raw
private evidence or unique device identity is publishable.

Final `git diff --check` passed; its output contained only the repository's
existing normal LF-to-CRLF notices. The publication checker passed for 233
publishable working-tree files, the index and 125 reachable history blobs. No
commit or push is part of this phase.
