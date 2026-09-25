# Device facts

DEVICE PHASE 1 is VERIFIED. The connected device is a Xiaomi Redmi Note 10 Pro
`M2101K6G`, bootloader product `sweet`, Turkey product `sweet_tr`. The bootloader is
unlocked. The layout is non-A/B with dedicated 128 MiB `boot` and `recovery`, a
separate 32 MiB `dtbo`, AVB `vbmeta*` partitions and an 8.5 GiB dynamic `super`.

The stock OS is Android 13 / MIUI 14 `V14.0.2.0.TKFTRXM`. The running kernel is
`4.14.190-perf-g6d6db67fd446`. The pinned Xiaomi `sweet-r-oss` commit is exactly
`758bb7ef50af360e728662a1ed3b3a1b977a2f13`, but its Makefile reports 4.14.180;
it is therefore device-relevant source, not an exact running-kernel match.

Runtime inventory confirms DRM/SDE DSI display, 1080x2400 at 60/120 Hz, active
Goodix touch, Qualcomm DWC3 UDC/gadget, ConfigFS/FunctionFS/NCM/HID, KGSL, MSM VIDC
with Qualcomm AVC declarations, DMA-BUF/ION and ARM SMMU. Exact panel SKU and
matching 4.14.190 source remain unknown.

DEVICE PHASE 2A is PARTIAL. The complete running `/proc/config.gz` is byte-identical
to the exact stock kernel's embedded IKCONFIG (decompressed SHA-256
`b8821ce37644106a92be29b1d8350ea517fade58a2d924e4e97bc9624e441ca5`). No later
official Xiaomi `sweet` source was found. The published 4.14.180 tree lacks 20
enabled symbols from the stock 4.14.190 config, including the running Goodix/FTS K6
and newer UFS features. The exact GNU linker family was matched to an official AOSP
prebuilt, but an authoritative Snapdragon LLVM 10.0.7 package was not obtained; a
host kernel build was therefore not forced.

DEVICE PHASE 2B verifies the stock userspace display-and-touch path. An ordinary
`untrusted_app` used the public MediaCodec API to select hardware-backed
`OMX.qcom.video.decoder.avc`, decoded 90/90 1280x576 and 120/120 2400x1080 real
Windows Annex-B access units, and presented them to a fullscreen Surface. The
2400x1080 run produced 113 render callbacks for 120 decoder outputs while the
physical display was 1080x2400@120 Hz; portrait/landscape Surface changes were
observed. Normal MotionEvent input delivered DOWN/MOVE/UP, pointer ID, pressure,
timestamps and 0..1079 by 0..2399 logical coordinates without opening an input
device node. Surface use does not prove DMA-BUF zero-copy.

The running kernel enables FunctionFS, NCM and HID gadget functions, but live
ConfigFS gadget state is denied to the shell domain and no function was changed.
The current composition remains `mtp,adb`. ADB-forwarded TCP is sufficient for a
first stock display+touch prototype; dedicated NCM/FunctionFS and HID remain a
separately authorized experiment. The operator manually enabled MIUI `Install
via USB` for the disposable probe, so the strict procedural Phase 2B result is
PARTIAL even though the technical userspace feasibility gate passed. See
[DEVICE PHASE 2B results](device/DEVICE_PHASE2B_RESULTS.md), the
[Android userspace baseline](device/ANDROID_USERSPACE_BASELINE.md) and the
[USB gadget plan](device/USB_GADGET_PLAN.md).

DEVICE PHASE 2C-0 is VERIFIED for the ADB development profile. Real Display 3
content used the existing Windows hardware H.264 path, an ephemeral
`adb forward tcp:48231 tcp:48231`, an ordinary receiver APK and hardware-backed
`OMX.qcom.video.decoder.avc` Surface output. A 60-second 2400x1080 target run
ACKed and decoded 3,250 frames; the operator confirmed correct orientation,
aspect/crop and motion, with occasional slight stutter. A separate conservative
600-second run completed with 21,016 ACKed frames, bounded queues, two ready
sessions and visually confirmed recovery after a controlled 3.124-second Android
receiver restart. Android reported no protocol, CRC, sequence, session or decoder
errors. Sustained cadence fell to roughly 23–27 FPS late in the long runs and the
Windows Host resource increase remains unexplained; no sustained-60-FPS or
leak-free claim is made. See [DEVICE PHASE 2C-0 results](device/DEVICE_PHASE2C0_RESULTS.md),
the [Android receiver](device/ANDROID_RECEIVER.md) and
[ADB transport](device/ADB_TRANSPORT.md).

MIUI `Install via USB` was manually enabled by the owner before Phase 2C-0 and is
intentionally retained as the dedicated development-device baseline. The phase
did not change that setting programmatically. The live USB composition remained
`mtp,adb`; no gadget, ConfigFS, NCM or HID operation occurred.

DEVICE PHASE 2C-T is PARTIAL. An explicit profile carried real public Android
MotionEvents over the existing ADB-forwarded connection to dynamically mapped
Windows touch injection. The tested target controls and two independent contacts
were received correctly, and a controlled reconnect observed target-side release
of an active contact under a fresh session. The host recorded an ambiguous
Windows release-update API result during that interruption, so a universal
disconnect-release claim is not made. USB remained `mtp,adb`; no native input,
USB gadget, ConfigFS, root, security or partition operation occurred. See
[DEVICE PHASE 2C-T results](device/DEVICE_PHASE2CT_RESULTS.md).

DEVICE PHASE 2C-T1 is VERIFIED without relabelling 2C-T. The preserved timeout
was an old `InjectTouchInput` failure; the following invalid-parameter value was
conclusively localized to late, unreliable error capture rather than proven as a
second API result. A single transport-worker-owned state machine now preserves
last successful coordinates and uncertain release state. Twenty repetitions of
each of nine Windows scenarios plus three real single-contact, three MOVE, three
two-contact disconnects and an active-contact normal Host shutdown ended with
zero unexplained injection failure and zero target contact left active. USB was
`mtp,adb` before and after. See
[DEVICE PHASE 2C-T1 results](device/DEVICE_PHASE2CT1_RESULTS.md).

The host-side stock-image gate is VERIFIED. Exact Turkey Fastboot/recovery archives
are preserved privately with local SHA-256 hashes. Stock `boot` and `recovery` use
Android boot header v2, 4 KiB pages, gzip kernel/ramdisk and embedded DTB; the
separate DTBO table has 40 entries and runtime index 12 identifies model `SWEET`.
AVB and dynamic-super topology are documented in the
[stock image manifest](device/STOCK_IMAGE_MANIFEST.md). No custom image has been
built, booted or flashed.

See [device inventory](device/DEVICE_INVENTORY.md) and
[DEVICE PHASE 1 results](device/DEVICE_PHASE1_RESULTS.md), and the
[DEVICE PHASE 2A results](device/DEVICE_PHASE2A_RESULTS.md). Raw identifiers remain
under ignored private evidence. `Collect-DeviceInventory.ps1` records bounded
read-only queries; never publish its raw output without identifier review.
