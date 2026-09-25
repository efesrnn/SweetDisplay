# Milestones and gates

> **MIUI + Android receiver APK is the development/validation platform. It is
> not the final SweetDisplay product architecture.**

## Final-product architecture sequence (2026-09-24)

SWEETDISPLAY FINAL-ARCH PHASE 1 is **VERIFIED** as an offline decision gate. The
selected product foundation reuses the exact stock kernel/DTB with a custom
Android-derived minimal userspace: selected native graphics/media services and
exact vendor components, a SweetDisplay-owned native EGL/GLES UI,NCM/TCP first,
and direct-evdev touch over the existing SWDP profile before any later HID.
MIUI Launcher/SystemUI/manual APK launch and normal-use ADB are not part of the
product. That architecture gate built no custom artifact. See
`docs/final-architecture/SWEETDISPLAY_FINAL_ARCH_PHASE1.md`.

SWEETDISPLAY FINAL-BOOT PREP 1 is **VERIFIED** for its host-only scope. The
stock boot body round-trips byte-for-byte and one deterministic private
header-v2 candidate preserves the exact stock kernel/DTB/cmdline while using a
66-entry no-persistence ramdisk and static direct-DRM/Goodix/DWC3 diagnostic.
No image was sent to the phone. The decision is **GO FOR SEPARATELY AUTHORIZED
FINAL-BOOT 1**; exact-bootloader `fastboot boot` behavior remains live-unverified.
See `docs/final-architecture/SWEETDISPLAY_FINAL_BOOT_PREP1.md`.

SWEETDISPLAY FINAL-BOOT 1 is **FAILED with successful stock recovery**. The
single exact-hash RAM-only command passed the identity gate and Fastboot reported
download plus boot `OKAY`,but no SweetDisplay UI appeared;the owner instead saw
normal stock Android. Android13 / `V14.0.2.0.TKFTRXM`,boot completion and ADB
returned without physical recovery. No partition-write command or retry occurred.
See `docs/final-architecture/SWEETDISPLAY_FINAL_BOOT1_RESULTS.md`.

SWEETDISPLAY FINAL-BOOT 1A is **VERIFIED for its host-only scope**. Static
review found v1's silent approximately 40-second DRM-failure reboot path and
built a deterministic private v2 with direct `late-init` launch, exact stock
ueventd rules, enforcing-policy gate, pmsg/kmsg checkpoints, visible pulse codes,
exact DRM-stage errors and tagged reboot reasons. No device command ran. The
decision is **GO FOR SEPARATELY AUTHORIZED FINAL-BOOT 1B**; FINAL-BOOT 1 remains
FAILED. See
`docs/final-architecture/SWEETDISPLAY_FINAL_BOOT1A_ANALYSIS.md`.

SWEETDISPLAY FINAL-BOOT 1B is **VERIFIED for its observability objective**. One
exact-v2 RAM-only boot returned to Fastboot;post-return `SYSTEM_LAST_KMSG`
proved kernel,ramdisk and Android first-stage init execution. Init aborted at
about1.72s because the minimal ramdisk omitted the required empty `/mnt` and
`/debug_ramdisk` directories,then explicitly rebooted to bootloader. Owner
physical recovery restored healthy stock Android and exact pre-boot `mtp,adb`.
No second attempt or partition-write command occurred. See
`docs/final-architecture/SWEETDISPLAY_FINAL_BOOT1B_RESULTS.md`.

Current bringup update (2026-09-25): V10 has produced real SweetDisplay-owned
AMOLED pixels, enforcing SELinux, physical-touch coordinate updates and read-only
USB-controller detection, followed by healthy stock return. FINAL-BOOT BRINGUP
is VERIFIED: V10 established the first pixels but exposed active-scanout redraw;
one V11 RAM-only boot verified stable double-buffered presentation, changing
physical-touch coordinates, enforcing/read-only controller status and bounded
healthy stock return. Do not repeat V10/V11. HARD STOP before FINAL-USB.
See [current results](final-architecture/SWEETDISPLAY_FINAL_BOOT_BRINGUP.md).
The iterative bringup supersedes the old recommendation for separate 1C/input
micro-phases; it does not change historical outcomes.

Meaningful sequence:

1. **FINAL-BOOT PREP 1 — VERIFIED:** offline exact stock-kernel temporary-boot
   artifact,dependency closure and recovery validation;no device command.
2. **FINAL-BOOT 1 — FAILED / stock recovered:** one accepted RAM-only command
   produced no visible custom-environment milestone;failure point UNKNOWN.
3. **FINAL-BOOT 1A — VERIFIED offline:** v1 failure audit plus deterministic
   v2 checkpoint design/artifact;no phone command.
4. **FINAL-BOOT 1B — GO only when separately authorized:** one exact-v2-hash
   RAM-only attempt completed;VERIFIED observability localized first-stage init.
5. **FINAL-BOOT BRINGUP — VERIFIED:** V3-V11 iterative directory/DRM bringup;
   stable pixels, physical touch, enforcing, DWC3 observation and bounded healthy
   stock return proven. HARD STOP; no automatic new phase.
6. **FINAL-USB — PARTIAL; Candidate 4 ready offline:** Candidate 3 live-proved
   custom NCM/UsbNcm and reached `SIOCSIFADDR`; Android SELinux's separate ioctl
   xperm filter denied exact command `0x8916`. Normal stock return and Windows
   cleanup passed. Candidate 4 remains enforcing and adds only xperm `0x8916`
   and `0x891c`, the two mutating interface commands actually used. Fresh owner
   physical/Fastboot gate required before one RAM-only attempt.
7. **Native touch:** bounded direct-evdev coordinate proof is included in
   bringup; full multitouch/transport behavior is not claimed.
8. **FINAL-USB 1:** custom userspace owns NCM,addressing and the SWDP probe.
9. **FINAL-MEDIA 1:** existing H.264/SWDP pipeline reaches vendor hardware decode
   and accelerated presentation.
10. **FINAL-HID 1:** optional USB multitouch after the data plane is stable.
11. **FINAL-UI 1 / FINAL-FEATURES:** dashboard,modes,settings,authorized media
   controls,telemetry and macros.
12. **FINAL-CAMERA:** camera/HAL/UVC last.

This sequence supersedes earlier *next-action recommendations* to solve MIUI
`usb0` privilege or proceed directly to E2B. It does not relabel any historical
result:2C-1E2 remains BLOCKED and its offline probe remains retained for
FINAL-USB.

## Active PC-only sequence (2026-09-19)

PHASE 1 frame proof and PHASE 2 shared-GPU handoff are VERIFIED. The owner's
current sequence supersedes the older milestone numbering below:

1. 3A: establish actual new-frame capacity and >=30-minute soak, bounded resources,
   queue and healthy reconnect. VERIFIED by the fresh 1810.0007551-second soak
   and reconnect; Host 55.662408 FPS. Historical soak UNKNOWN; 60 FPS not proven.
   See PHASE3A_RESULTS.md. Its stop/report gate was completed.
2. 3B: real GPU-native hardware H.264, progressive modes, decode/content proof.
   Completed under the earlier explicit PHASE 3B authorization.
   VERIFIED under latest bounded-admission acceptance: recovered1810.0008772s
   sustained run,93069 admitted=encoded=decoded frames,source56.241409FPS versus
   output51.419312FPS.99 transient admission drops remain explicit; strict all-Host
   assertion fails. Stable observed resources/latency,exact accounting,clean and
   forced-Host reconnect/content verification pass; latter has1 transient drop.
   Early upstream slowdown and underlying token timing UNKNOWN. Previous short
   results/evidence unchanged. Actual60FPS and lossless-all-Host capacity unproven.
   No driver/deployment/security changes. Its stop/report gate was completed.
   See PHASE3B_RESULTS.md.
3. 3C: VERIFIED bounded localhost protocol.35s normal hardware stream1782 AUs
   received/ACKed/decoded;45s real receiver reconnect with fresh handshake/session,
   IDR/SPS/PPS recovery and1506 post-reconnect AUs independently decoded/content
   matched. Exact bounded accounting,clean final exits,unchanged PnP/security.
   Source counters remain separate from wire sequence;historical failures kept.
   See PHASE3C_RESULTS.md. Report/stop gate completed;3D is now separately authorized.
4. 3D: VERIFIED by owner-observed `visual-flow-3`. The gated controller passes
   35.0282436s normal real Display3 rendering and45.0069927s real receiver-close/
   fresh-process reconnect, independent decode/content/render review, unchanged
   resource limits, exact bounded accounting, PnP/security and clean shutdown.
   Normal has1191 Presents and64/64 source matches; one window-drag UI stall causes
   a bounded overflow and automatic IDR recovery. Fresh receiver has848 decoded,
   847 Presents,48/48 matches and zero overflow/resync. The owner confirms visible
   motion before and after reconnect. Measured reconnect blackout8.6608s remains
   a documented optimization target. Historical `visual-flow-1` remains UNKNOWN.
   See PHASE3D_RESULTS.md. That hard stop was completed before later, separately
   authorized device work.
5. 3E: same-protocol absolute touch loop, coordinate and disconnect cleanup tests.
6. 3F: codec/protocol/reconnect/input tests and logical future daemon contracts.

DEVICE PHASE 2C-T was separately authorized after this PC-only sequence and is
PARTIAL; see `docs/device/DEVICE_PHASE2CT_RESULTS.md`. It does not authorize
Phase 3E, a USB gadget, native input, camera work or security changes. Report a
sustained-rate shortfall before major architecture changes. Preserve all Windows
security settings.
The later, separately bounded DEVICE PHASE 2C-T1 release-reliability follow-up is
VERIFIED; see `docs/device/DEVICE_PHASE2CT1_RESULTS.md`. It does not relabel 2C-T
or advance any roadmap phase.
DEVICE PHASE 2C-PERF is separately VERIFIED as a diagnostic localization. Two
real-target runs place the first sustained divergence between IddCx source and
Host admission only when the ADB-forwarded Android target is active; source,
hardware-encode and local-transport controls remain healthy. The exact mechanism
within that target interaction is still UNKNOWN. No performance correction,
2C-1 or3E was authorized or implemented. See
`docs/device/DEVICE_PHASE2C_PERF_RESULTS.md`.
DEVICE PHASE 2C-PERF1 is PARTIAL. Bounded QPC timing, normal full/reduced
evidence, real Android ACK-only and an uninstrumented legacy-Host control all
complete at approximately56.3FPS source/Host. The historical26–28FPS state did
not reproduce, so the exact first blocking component remains UNKNOWN and no
correction is justified. Current socket, ACK and measured lock paths are not the
active limiter. A future trace must be armed before any ADB/receiver-state
restart and frozen only on a measured degradation trigger. See
`docs/device/DEVICE_PHASE2C_PERF1_RESULTS.md`.
DEVICE PHASE 2C-PERF2 is VERIFIED / TRIGGERED. Its bounded flight recorder
captured a real late degradation with61.360385s pre-trigger,60.070676s
post-trigger and zero trace loss. The first material wait is synchronous AMD
hardware-MFT `HaveOutput` event service on the Host main/admission thread;
socket/ACK,transport locks,keyed mutex and admission checks remain stable. The
internal `GetEvent` versus status/type split and the MFT state-transition cause
remain UNKNOWN. A separately authorized correction may move MFT event retrieval
to one controlled bounded worker; no correction,2C-1 or3E was implemented. See
`docs/device/DEVICE_PHASE2C_PERF2_RESULTS.md`.
DEVICE PHASE 2C-PERF3 is VERIFIED. One MTA worker exclusively owns all AMD MFT calls, Host no longer
services `HaveOutput`, and the new input queue is fixed at4 while the existing
sender queue remains fixed at3. The real1800.004732s run sustained
source53.358749FPS,Host51.527087FPS,ratio0.965673 with no PERF2 trigger; Android
decoded92370 frames with zero protocol/queue/decoder error. This does not prove
the AMD MFT permanently fixed or complete a production interactive third-screen
UI. Its bounded touch regression passed one contact,drag,natural release and
final zero active contacts without relabeling historical2C-T. See
`docs/device/DEVICE_PHASE2C_PERF3_RESULTS.md`.
DEVICE PHASE 2C-1 is VERIFIED as a read-only native-USB architecture gate. The
exact ROM includes a dormant stock `ncm,adb` init composition and the exact
kernel contains NCM, FunctionFS and HID. The first native direction is therefore
NCM+ADB with the unchanged TCP/SweetDisplay stream; FunctionFS/WinUSB is a later
fallback and HID is a later touch-only hybrid. A custom kernel is not required.
The one selected next experiment is a separately authorized, level-2,
enumeration-only temporary switch to the stock `ncm,adb` branch followed by exact
rollback to `mtp,adb`; that architecture recommendation is retained while its
later live result is recorded separately below. No IP/video/HID stage may be
combined with that first gate. See `docs/device/DEVICE_PHASE2C1_ARCHITECTURE.md`.
DEVICE PHASE 2C-1E1 is DENIED as a separate live gate. The one authorized stock
framework request for NCM returned255 and did not move the phone from `mtp,adb`;
no NCM interface enumerated on either side and ADB remained available. Direct
property,ConfigFS and UDC routes were correctly not used. Same-path rollback and
final ADB/MTP health passed with no persistent property,driver or network change.
Do not infer an NCM descriptor or coexistence failure because NCM was never
selected. See `docs/device/DEVICE_PHASE2C1E1_RESULTS.md`.
DEVICE PHASE 2C-1E1A is VERIFIED as a separate read-only diagnosis without
changing the historical E1 result. Exact Xiaomi code and retained runtime events
prove that the request traversed `IUsbManager` to the legacy property handler,
briefly selected `ncm,adb` and reconnected ADB. A later `mtp,adb` request is
strongly localized to a user interaction in Xiaomi's USB details activity; the
exact clicked row remains UNKNOWN. The legitimate stock Level-2 path is therefore
accessible, Gadget HAL absence is not the blocker, and a custom kernel remains
unnecessary. Windows NCM enumeration was not captured. The only recommended next
gate is a separately authorized, hands-off, pre-instrumented enumeration replay;
no IP/TCP/video work may be combined with it. See
`docs/device/DEVICE_PHASE2C1E1A_DIAGNOSIS.md`.
DEVICE PHASE 2C-1E1B is PARTIAL. A single controlled stock request proved real
Android `ncm,adb`,phone `usb0`,healthy Microsoft inbox `UsbNcm`,concurrent ADB and
MTP disappearance. The composition did not remain for the 30-second hands-off
window: Xiaomi's USB details activity recorded a DOWN/UP and requested `mtp,adb`.
The input source is UNKNOWN. Primary stock rollback and final ADB/MTP,persistent-
property,driver-inventory and active-profile checks passed. Do not progress to
IP,TCP,video,HID or FunctionFS from this PARTIAL result. See
`docs/device/DEVICE_PHASE2C1E1B_RESULTS.md`.
DEVICE PHASE 2C-1E1C is PARTIAL. The same live enumeration was reproduced,but
the required120-second hold again reverted within seconds. Passive kernel input
evidence localizes the triggering103ms DOWN/UP to the physical touchscreen path;
Xiaomi USB details requested `mtp,adb` about99ms later. Real contact versus a
touch-controller/electrical ghost event remains UNKNOWN. Healthy autonomous
return was confirmed; no redundant rollback request was issued. Do not progress
to IP,TCP,video,HID or FunctionFS from this result. See
`docs/device/DEVICE_PHASE2C1E1C_RESULTS.md`.
DEVICE PHASE 2C-1E1D is VERIFIED. In the owner-initiated locked/noninteractive
screen-off state,real `ncm,adb`,`usb0`,inbox `UsbNcm` and ADB remained healthy
for121.364s. Passive kernel input recorded no event,Xiaomi USB details did not
launch,and no autonomous MTP request occurred. A USB notification briefly used
the normal low-power AOD/DOZE substate without an interactive wake. Stock
rollback restored healthy `mtp,adb` with no persistent change. The only next
recommended gate is separately authorized **DEVICE PHASE 2C-1E2 — EPHEMERAL NCM
IP + TCP DATA-PLANE PROOF**; it was not executed here. See
`docs/device/DEVICE_PHASE2C1E1D_RESULTS.md`.
DEVICE PHASE 2C-1E2 is **BLOCKED** at the authorized Android address-control
boundary. Screen-off `ncm,adb`,phone `usb0`,healthy inbox `UsbNcm` and ADB were
re-established,but stock shell `ndc interface setcfg` returned1 and assigned no
IPv4 address. No Windows test address,ping or TCP was attempted,and clean stock
rollback restored `mtp,adb`. The next bounded recommendation is read-only
**DEVICE PHASE 2C-1E2A — STOCK USB0 ADDRESS-OWNERSHIP DIAGNOSIS**. Do not begin
E3 until E2 passes. See `docs/device/DEVICE_PHASE2C1E2_RESULTS.md`.
DEVICE PHASE 2C-1E2A is **VERIFIED** as a read-only diagnosis;2C-1E2 remains
BLOCKED. Stock NetworkStack/Tethering plus `netd` own downstream IPv4,but direct
`ndc` requires a network-stack signature permission and the ordinary receiver
has no address-administration API. Exact-ROM tethering is configured for RNDIS,
not NCM,and composition-only `ncm,adb` intentionally left `usb0` unprovisioned.
Conclusion: a stock owner exists but no accessible current app/shell path exists.
The E2A-era Level-3 recommendation is preserved as history and is now absorbed
by FINAL-ARCH 1;FINAL-BOOT PREP 1 is complete and the current next gate is only
a separately authorized RAM-only FINAL-BOOT 1. Do not begin it implicitly,or
begin E2B/E3. See
`docs/device/DEVICE_PHASE2C1E2A_DIAGNOSIS.md`.
Current measurements and limits: PHASE3_VALIDATION.md and STATUS.md.

## Broader project roadmap

| Order | Deliverable | Acceptance / gate |
|---|---|---|
| 0 | Foundation / inventory / recovery plan | Source-attributed facts; no invented device values; repository and evidence saved |
| 1 | Original Microsoft sample | VERIFIED unchanged Debug x64 build with EWDK 26100.6584 |
| 2 | SweetDisplay driver | One valid monitor, preferred 2400x1080@60 and fallback modes; build then explicit signing/configuration review |
| 3 | Windows extension | VERIFIED: owner confirmed Settings Display 3; active SWT0001 2400x1080@60 extended desktop |
| 4 | Driver-to-host proof | VERIFIED PHASE 2: actual changing content, bounded shared GPU queue, crash cleanup and reconnect |
| 5 | Encoder / transport foundations | Enumerate and actually test MFTs; protocol parser conformance; no device required |
| 6 | Manual unlock + recovery ready | Verify unlock and exact stock image set before any custom boot |
| 7 | Temporary minimal boot | Verify support, reliable debug shell, no persistent writes; unsupported temporary boot → stop |
| 8 | Local graphics / input | Color patterns without SurfaceFlinger; reliable local touch |
| 9 | Raw end-to-end | Moving Windows window appears on phone at 800x360@10, measured latency |
| 10 | Compression / scale | Hardware decode capability measured; 1280x576@30 → 2400x1080@30 → 60 |
| 11 | Native touch | Absolute touchscreen mapped to the correct display; Spotify control test |
| 12 | Camera / UVC | Local capture then Windows Camera 720p30; simultaneous bandwidth/thermal tests |

120 Hz and batteryless operation are separate future projects. Maintain STATUS and TEST_LOG at every meaningful attempt; never turn an architectural target into a success claim.
