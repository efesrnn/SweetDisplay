# DEVICE PHASE 2B results — stock-kernel userspace feasibility

Date: 2026-09-21. Technical feasibility result: **VERIFIED** for the first
display-and-touch prototype. Strict procedural phase result: **PARTIAL** because
the operator manually enabled MIUI's persistent `Install via USB` security
setting after two ordinary ADB installs were rejected. That owner-authorized
change permitted the probe, but it means original acceptance item 7 (no
device-security setting change during the phase) was not literally met. This is
not hidden or reclassified.

No custom kernel is required for the first prototype if USB initially means the
existing ADB data path (for example, ADB-forwarded TCP). A production custom USB
function and USB HID composition still require the separately authorized Phase
2C experiment and a privileged stock-userspace integration. They were not tested
or configured here.

## Decision

| Path | Classification | Evidence and boundary |
|---|---|---|
| Hardware H.264 decode | **VERIFIED** | An ordinary `untrusted_app` selected vendor hardware decoder `OMX.qcom.video.decoder.avc` and decoded both real Windows Annex-B fixtures without decode errors |
| Display presentation | **VERIFIED** | Decoder output was scheduled to a fullscreen `SurfaceView`; 81 and 113 frame-render callbacks were observed while the activity was visible and resumed |
| Touch capture | **VERIFIED** | Normal Android input delivered DOWN/MOVE/UP, pointer ID, X/Y, pressure and event time; no `/dev/input` node was opened or grabbed |
| USB video/control transport | **SUPPORTED-BY-INVENTORY / REQUIRES-EXPERIMENT** | Current `mtp,adb` composition works for an ADB-forwarded prototype; NCM and FunctionFS are built into the running kernel but no new live function was configured |
| USB HID | **SUPPORTED-BY-INVENTORY / REQUIRES-EXPERIMENT** | Running config has `USB_F_HID` and `USB_CONFIGFS_F_HID`; configfs is inaccessible to shell and no HID function/device was created |
| Zero-copy | **UNKNOWN** | Surface decode uses an opaque codec/gralloc path, but this does not prove end-to-end DMA-BUF zero-copy or Windows-to-panel buffer sharing |
| Custom kernel required for first display+touch prototype | **NO** | Real stock-runtime decode, Surface presentation and app-level touch all passed; USB gadget privilege is a userspace/policy problem at this gate, not evidence of a missing kernel feature |

The `NO` conclusion is limited to the first ADB-transported display+touch
prototype. It is not a claim that an ordinary APK can configure a production
composite gadget or emit `/dev/hidg*` reports.

## Read-only runtime inventory

The phone remained in the installed stock Android 13 / MIUI 14 OS. All inventory
commands were bounded `getprop`, `dumpsys`, `ls`, `cat`, `lshal` or process-list
reads.

- SurfaceFlinger owns the physical display through the stock graphics stack.
  The display exposes 1080x2400 modes at approximately 60 and 120 Hz. The probe
  ran with the 120 Hz mode active.
- The active composer service is
  `android.hardware.graphics.composer@2.4-service`; allocator 3.0/4.0 and mapper
  3.0/4.0 declarations are present. Vendor gralloc is `gralloc.sm6150.so`.
- AVC decoders exposed through `MediaCodecList` are the Qualcomm normal and
  secure hardware decoders, the Android C2 software decoder and its legacy OMX
  alias. The normal Qualcomm decoder is accessible to an ordinary application.
- The Qualcomm decoder reports Baseline, Main, High, Constrained Baseline and
  Constrained High through AVC Level 5.1. It reports adaptive playback, not
  tunneled playback, not secure playback on the normal component, and no exposed
  `FEATURE_LowLatency` flag. The reported color-format values are
  `2141391878, 2135033992, 2141391876, 21, 19, 2141391877`; 19/21 are standard
  planar/semi-planar YUV420 and 2135033992 is flexible YUV420. Surface output
  selected vendor format 2141391878.
- `/dev/dri/card0` and `renderD128` are DAC world-readable/writable but carry
  `graphics_device` SELinux labels. Direct access was not attempted. `/dev/ion`
  is `system:system` mode 0664 with `ion_device`; no `/dev/dma_heap` directory is
  present. Camera/video nodes are `system:camera` mode 0660 with `video_device`.
  Ordinary apps should use framework services rather than these nodes.
- SELinux remained Enforcing. SurfaceFlinger, media codec and the probe ran in
  their normal `surfaceflinger`, `mediacodec` and `untrusted_app` domains.
- ConfigFS is mounted at `/config`; the shell domain is denied access to
  `/config/usb_gadget`. FunctionFS mounts for ADB and vendor diagnostic functions
  exist. `/dev/usb-ffs/adb` has a `functionfs` label.
- USB properties report `sys.usb.config=mtp,adb` and
  `sys.usb.state=mtp,adb`; the USB service reports user-facing MTP and the
  controller is Qualcomm DWC3. No function, descriptor, UDC binding or endpoint
  was changed.
- Running stock config independently verifies ConfigFS FunctionFS, NCM and HID
  gadget support. ECM, RNDIS, EEM and UVC ConfigFS functions are disabled.
- InputReader maps `goodix_ts` to `/dev/input/event4` with logical X 0..1079 and
  Y 0..2399. Input nodes are `input_device`; direct open/grab was deliberately
  avoided.

## Disposable APK results

The source is under `device/android-userspace-probe`; the offline builder is
`scripts/windows/Build-AndroidUserspaceProbe.ps1`. APKs, signing keys, fixture
bytes and logs stay under ignored/private paths. The manifest requests no Android
permissions, no device administration, no accessibility service, no VPN, no
overlay and no USB permission.

Both fixtures are bounded prefixes of the already verified Windows hardware
encoder evidence. Each begins with an access unit containing IDR+SPS+PPS.

| Probe | Fixture identity | Submitted / decoder output / render callback | Surface result |
|---|---|---|---|
| Conservative | 1280x576, nominal 30 FPS, 90 AUs, 1,875,627 bytes; SHA-256 `8737660e7ac41c432814b293eb086ba802afc8683546eb0b1aafdf2ead358f6e` | 90 / 90 / 81; no decode error | PASS; callback interval avg/min/max 34.063/8.284/83.322 ms |
| Target-compatible | 2400x1080, nominal 60 FPS, 120 AUs, 7,500,766 bytes; SHA-256 `a457bebcd267bde230124930813d65520994dc915fb068a643c92c43075b9129` | 120 / 120 / 113; no decode error | PASS; callback interval avg/min/max 18.601/8.323/33.345 ms |

The second decode returned a 2432x1088 coded buffer with crop 0..2399 by
0..1079, exactly preserving the protocol's 2400x1080 visible geometry. The
fullscreen Surface measured 1080x2307 in portrait. A physical orientation change
produced rotation 1 and a 2307x1080 Surface, then returned to rotation 0 and
1080x2307. The seven missing render callbacks in the 2400x1080 run and nine in
the conservative run show that decoder output count must not be equated with
physical presentation count. No sustained latency or lossless-60-FPS claim is
made from these short fixtures.

The final 2400x1080 probe APK SHA-256 is
`8915d908335cbf86b47ac7b56081408354eb811b9b01f434a9e68662475805cc`.
The earlier conservative APK SHA-256 was
`ff54daad9f21aada1cc98c2e8863c02776495674ec43be8cdabb49321f8c4ffe`.

## Touch result

The final app run recorded 471 normal framework events: 84 DOWN, 293 MOVE and
84 UP, with zero CANCEL. Pointer ID 0, pressure, monotonic event time and the
logical 0..1079 by 0..2399 ranges were present. Observed gesture coordinates
remained within those bounds. This verifies capture and normalization inputs in
an ordinary APK; it does not inject input or prove USB HID emission.

## Minimum apparent privilege

Levels are A ordinary APK, B APK plus ADB development facilities, C shell native,
D privileged/system app, E root/custom SELinux policy and F custom kernel.

| Subsystem | Minimum apparent level | Basis |
|---|---|---|
| Qualcomm hardware AVC decode | A | Actually passed in `untrusted_app` through public MediaCodec APIs |
| Fullscreen presentation | A | Actually passed with SurfaceView/SurfaceFlinger |
| Low-latency rendering | A for a prototype; PARTIAL | Timed Surface release works, but codec low-latency feature is not exposed and end-to-end latency is unmeasured |
| Touch capture | A | Actually passed through normal MotionEvent delivery |
| Development USB video/control | B | ADB-forwarded TCP can carry the unchanged protocol without gadget reconfiguration |
| Dedicated FunctionFS/NCM transport | D/E pending experiment | Kernel support exists, but an ordinary app cannot create/bind functions or obtain endpoint policy merely from config symbols |
| USB HID reports | D/E pending experiment | Kernel function exists; ConfigFS composition and `/dev/hidg*` ownership/policy are privileged and untested |
| Gadget composition changes | D/E | Shell was denied ConfigFS gadget access; init/vendor USB services own the live composition |
| DMA-BUF zero-copy interoperability | UNKNOWN; likely native/privileged integration | Surface proves an opaque hardware path only; raw ION/DRM access was neither needed nor tested |
| Custom kernel | Not required at this gate | Required display, decoder, input and candidate USB function support already exists |

## Protocol compatibility

HELLO, CAPABILITIES, FRAME, CONTROL, TELEMETRY, HEARTBEAT, exact session/sequence
rules, CRC, reconnect state and IDR/SPS/PPS resynchronization can be reused without
changing the Windows 1.0 byte framing. The Android decoder accepted the exact
Annex-B access units produced by the verified Windows encoder. USB transfers must
remain a partial byte stream; USB packet boundaries cannot become message
boundaries. Android uses an independent monotonic clock, so clock mode 0 is
required until synchronization exists.

TOUCH cannot yet be called reusable as an implemented feature: protocol 1.0
reserves and rejects it. A later negotiated profile must define coordinate
orientation, logical range, contact IDs, timestamps, pressure/buttons and HID
mapping. No Windows PHASE 3E work was started.

## Safety and remaining unknowns

No fastboot command, root/su, remount, recovery, module load, AVB/partition write,
SELinux change, direct device-node write, USB reconfiguration, UDC unbind, ADB
disable or reboot occurred. The agent installed only the disposable probe and
removed its first signed variant before installing the second. The operator—not
the agent—manually enabled MIUI `Install via USB`; this must be considered when
applying the original strict acceptance rule.

Remaining unknowns are production FunctionFS endpoint ownership, whether the
stock vendor USB service can safely compose NCM+HID+ADB, actual HID endpoint
policy, sustained 2400x1080 cadence/latency, cross-device clock correlation and
end-to-end zero-copy behavior. They belong to a separately authorized Phase 2C
or later phase.

