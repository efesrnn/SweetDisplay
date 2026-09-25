# Architecture — implemented baseline and future design

## Verified display and encoding path

PHASE 2 is VERIFIED: SweetDisplay desktop -> IddCx -> one GPU copy -> three
same-adapter shared D3D11 textures -> SweetDisplayHost. Restricted documented
IOCTLs carry metadata/control. Keyed mutexes protect GPU ownership; bounded
newest-frame replacement never overwrites a held texture. The driver handles
monitor/swapchain lifetime and fast bounded GPU copies. It runs in Session 0;
no arbitrary desktop/process-handle access is assumed. See FRAME_HANDOFF.md.

PHASE 3A is VERIFIED by the fresh 1810-second soak and reconnect. Actual source
cadence is measured, not inferred from the 60-Hz display mode. Historical soak
anomalies remain UNKNOWN. See PHASE3A_RESULTS.md and CLASSIFIED_ACCEPTANCE.md.

PHASE 3B is VERIFIED under documented bounded admission: shared D3D11 ->
GPU-native BGRA-to-NV12 -> AMD hardware H.264 MFT -> compressed access units.
Normal encoding does not perform full-frame GPU-to-CPU readback or software
fallback. Sustained encode/decode/content verification, exact accounting and
clean/forced Host reconnect passed; explicit admission drops remain visible.
This is not lossless-all-Host or 60-unique-FPS proof. See PHASE3B_RESULTS.md.

## PHASE 3C implementation — VERIFIED for bounded localhost transport

An optional compressed-output callback in the encoder feeds an EncodedSink.
The separate transport Host build enables this hook; previous binaries are
preserved. The callback copies only compressed AU bytes into a bounded queue,
without socket I/O or retaining a shared GPU texture. One transport worker owns
network I/O. Pending capacity is three AUs plus one in flight; the receiver ACKs
one FRAME at a time. Encoder admission and transport loss have separate ledgers.
Callback overhead is included in the existing wall-clock encode latency metric.

Protocol code is independent of Winsock and native C++ structure layout.
ByteStream provides partial deadline-aware reads/writes; TcpStream implements
loopback TCP. Connection state is HELLO -> CAPABILITIES -> READY -> drain/close.
Each reconnect gets a new session, parser, sequence and ACK counters. Any lost
encoded continuity triggers suppression until IDR/SPS/PPS. Queues never silently
replay dependent old-session frames. See PROTOCOL.md for exact bytes and bounds.

SweetDisplayDeviceSimulator currently receives, validates and acknowledges
protocol/H.264 payloads and can retain bounded compressed evidence. It does not
render video. The independent offline H.264 decoder checks captured evidence;
that diagnostic does not implement the PHASE 3D visible simulator.

Deterministic parser and normal/slow-receiver fixture gates pass. Fresh35s normal
and45s real receiver reconnect runs pass exact accounting, hardware encode,
independent decode/content and captured-byte integrity, fresh HELLO/CAPABILITIES,
IDR/SPS/PPS resynchronization and clean final process shutdown. Four/nine encoder
pressure drops and160 disconnected/38 resync/one unconfirmed transport outcome
remain explicit. Source-content counter regressions are distinct from strict
wire sequence validation. Historical474-frame E/UNKNOWN and controller shutdown
failure remain preserved. See PHASE3C_RESULTS.md for scope and denominators.
No visible renderer or USB implementation exists. Stop before PHASE 3D.

## Final device path — selected; first live diagnostic failed safely

[SWEETDISPLAY FINAL-ARCH PHASE 1](final-architecture/SWEETDISPLAY_FINAL_ARCH_PHASE1.md)
selects the exact stock kernel/DTB plus a custom Android-derived minimal
userspace. Selected native SurfaceFlinger/HWC/gralloc/EGL and media/vendor
components preserve the strongest known acceleration path,while the MIUI Java
product shell/launcher and manual receiver-APK workflow are excluded. The
SweetDisplay-owned native core/UI will initially use NCM/TCP and the unchanged
bounded SWDP byte stream. Direct Goodix evdev will feed the existing touch
profile before any later HID migration. The final accelerated service closure
remains future work.

[SWEETDISPLAY FINAL-BOOT PREP 1](final-architecture/SWEETDISPLAY_FINAL_BOOT_PREP1.md)
implements the smaller first-boot probe offline. The deterministic private boot
candidate keeps the exact stock kernel/DTB/header/cmdline and uses a bounded
ramdisk containing stock recovery init/ueventd plus its exact library/policy
closure and one static native SweetDisplay diagnostic. That process owns a DRM
dumb buffer,reads Goodix evdev coordinates,observes DWC3 and displays SELinux
enforcement. It deliberately mounts no logical or persistent partition and
contains no ADB,network,USB gadget or partition tool. Direct DRM is a disposable
bring-up path;the selected final UI remains SurfaceFlinger/ANativeWindow plus
vendor EGL/HWC. The offline PREP 1 gate remains VERIFIED.

[SWEETDISPLAY FINAL-BOOT 1](final-architecture/SWEETDISPLAY_FINAL_BOOT1_RESULTS.md)
made one exact-hash RAM-only attempt. Fastboot accepted the download and boot
command,but the owner saw normal stock Android rather than any SweetDisplay
pixels. The expected stock build and ADB returned automatically;no physical
recovery or partition-write command was used. The result is FAILED,with the
handoff/kernel/init/policy/diagnostic/DRM failure boundary still UNKNOWN. The
architecture is not promoted to FINAL-INPUT or FINAL-USB. A host-only early-boot
diagnosis and non-persistent evidence design is the sole recommended next gate.

[SWEETDISPLAY FINAL-BOOT 1A](final-architecture/SWEETDISPLAY_FINAL_BOOT1A_ANALYSIS.md)
completed that host-only diagnosis without contacting the phone. V1 has a silent
approximately 40-second DRM-timeout/reboot path, while its 180-second UI timer
does not credibly match the initial 60-second stock observation. The private v2
keeps the exact stock kernel/DTB and enforcing recovery policy, restores the
exact stock ueventd rules, launches directly at `late-init`, and adds pmsg/kmsg,
backlight-pulse, exact DRM-stage and tagged-reboot checkpoints. The v2 image is
still a disposable RAM-only diagnostic, not product architecture. 1A is
VERIFIED offline and permits only a separately authorized FINAL-BOOT 1B; it does
not change the FAILED classification of FINAL-BOOT 1.

[SWEETDISPLAY FINAL-BOOT 1B](final-architecture/SWEETDISPLAY_FINAL_BOOT1B_RESULTS.md)
used that v2 exactly once. Fastboot accepted it and post-return ramoops evidence
proved the exact stock kernel,custom ramdisk and Android first-stage init ran.
The minimal ramdisk omitted stock empty mountpoint directories `/mnt` and
`/debug_ramdisk`;first-stage init therefore aborted at about1.72s and explicitly
rebooted to bootloader before enforcing-policy completion,`late-init` or the
diagnostic. Owner physical recovery restored healthy stock Android and the
pre-boot USB state. 1B is VERIFIED for observability,not for UI. The next design
gate is an offline first-stage directory-closure correction;no new live boot is
authorized.

The later [FINAL-BOOT BRINGUP](final-architecture/SWEETDISPLAY_FINAL_BOOT_BRINGUP.md)
supersedes the preceding historical next-action recommendation: V10 has now
produced physical SweetDisplay-owned pixels through direct DRM/KMS legacy
SETCRTC, enforcing SELinux and physical-touch coordinate updates. The private
photo also confirms read-only USB-controller discovery; stock MIUI returned
healthy after a kernel-recorded 184.428-second restart; owner confirms automatic
return. V10's visible wave-like redraw came from repainting active scanout. V11
adds a second buffer and completion-gated normal page flip without changing
modeset or security; one RAM-only run verified stable readable output during
physical touch, no V10 wave/fade, and a normal timer-path return to healthy stock.
FINAL-BOOT BRINGUP is VERIFIED and hard-stopped before FINAL-USB.
This is a minimal CPU-rendered diagnostic, not the final EGL/media
pipeline. Its original hard stop was observed before the separately authorized
FINAL-USB work began.

[SWEETDISPLAY FINAL-USB](final-architecture/SWEETDISPLAY_FINAL_USB.md) now has
an offline-ready first candidate. Init mounts ConfigFS, creates one
stock-aligned NCM function and binds the existing DWC3 UDC. A static enforcing
recovery-domain endpoint assigns only 10.77.77.2/30 to `usb0` and serves the
existing SWDP control handshake on TCP/48231. Windows uses its inbox Microsoft
UsbNcm driver and an ephemeral ActiveStore 10.77.77.1/30 address. There is no
ADB, FunctionFS, DHCP, gateway, DNS, NAT, MTP, HID, media or persistent network
state in this closure. The stable V11 UI exposes transport progress. This
Candidate 1 proved init-side NCM creation but stopped before IP because the
enforcing recovery domain could not search `sysfs_net`; healthy stock return
passed. Candidate 2 removed that read and live-proved custom NCM plus healthy
Microsoft inbox `UsbNcm`, but an exact enforcing UDP-socket denial stopped it
before the phone address ioctl. Candidate 3 used the stock-allowed TCP class and
reached `SIOCSIFADDR`; Android SELinux's separate ioctl xperm filter denied
exact command `0x8916`. Candidate 4 adds only xperm `0x8916`/`0x891c` while
remaining enforcing; successful address/TCP/SWDP remain live-unverified pending
a fresh owner physical gate.

The bounded Android development receiver can send explicitly negotiated profile-1
absolute touch events over the existing ADB-forwarded transport. Windows public
touch injection and its disconnect-release state machine are verified within the
DEVICE PHASE 2C-T1 scope; historical DEVICE PHASE 2C-T remains PARTIAL. A future
native `touchd`, physical HID and USB gadget path are not implemented or implied.
Camera work is outside the current scope. Device services are intended for
appliance startup, but no hardware-specific daemon is claimed.

The Host remains a console development tool. Service packaging, authenticated
remote transport, USB discovery and device capability validation are future
work. No design depends on Android mirroring or replacement SoCs. Reusing
published downstream drivers is a strategy, not proof vendor services can be
removed. Original battery/charging hardware stays intact; batteryless use is
outside V1. See USB_PROTOCOL, TOUCH, CAMERA and BUILD_DEVICE.
