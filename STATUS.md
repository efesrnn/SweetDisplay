# Status — 2026-09-25

Statuses: VERIFIED / ASSUMED / SUSPECTED / BLOCKED / NOT YET TESTED.
PHASE 3 subphase summaries additionally use the owner's PARTIAL category.
VERIFIED records a measured result or explicitly identified source/owner evidence.
Runtime performance is not inferred from source compatibility or an advertised mode.

| Milestone | Status | Evidence / next action |
|---|---|---|
| Original Microsoft sample build | VERIFIED | Pinned upstream unchanged; EWDK 26100.6584, Debug x64, zero build warnings/errors |
| SweetDisplay driver and Host build | VERIFIED | PHASE 2 GPU handoff; Debug x64 driver, Release x64 Host; WDK/API/catalog checks and 2,033 metadata/queue checks pass |
| Development signing / installed driver | VERIFIED | Existing certificate reused; PHASE 2 driver oem83.inf active; monitor oem81.inf unchanged; PHASE 1 rollback retained |
| Windows indirect-display baseline | VERIFIED | Owner confirmed Settings Display 3; runtime API confirms active SWT0001 at 2400x1080@60 on separate desktop coordinates |
| PHASE 1: real IddCx frame reception | VERIFIED | 771 real frames / 19.996027 s; 38.507650 FPS; all 2400x1080 BGRA8; ordered IDs/QPC; zero ETW loss |
| PHASE 1: exactly one diagnostic frame | VERIFIED | One BMP; test nonce and paint counter 117 match Display 3/SWT0001 test pattern; visual inspection passed |
| 60 FPS sustained reception capacity | BLOCKED | New VERIFIED soak: Host 55.662408 FPS, distinct pattern 55.660198 FPS. Upstream target presentation cadence skips refresh intervals even without Host; precise scheduling cause remains SUSPECTED. PHASE 3A acceptance does not require exactly 60 FPS |
| PHASE 2: driver-to-host continuous frame transfer | VERIFIED | Complete acceptance-4 + independent verifier: 35 s at 38.77 FPS, reconnect 35 s at 38.60 FPS; bounded slow-consumer/crash/reconnect/security checks pass |
| PHASE 3A: capacity + soak | VERIFIED | Fresh 1810.0007551-second soak: 100,749 received, 55.662408 FPS, A100749/B0/C0/D0/E0, exact 149-drop accounting, queue peak 3, 15-second fresh-process reconnect and clean shutdown. Resource/health review passes; old soak UNKNOWN. See docs/PHASE3A_RESULTS.md |
| Historical soak investigation | PARTIAL | Original 174 nonce mismatches and counter regression remain historically UNKNOWN. Controlled cover/old-content tests establish legitimate compositor mechanisms; old evidence is not relabelled |
| Corrected A–E acceptance / PHASE 3A.1 | VERIFIED | Both B/C pilots and 300.0063274-second observation pass: A16435/B0/C0/D0/E0, source 54.852176 / Host 54.782178 FPS; exact accounting, queue peak 2, clean 15-second fresh-process reconnect, flat resource medians and unchanged health. Historical soak UNKNOWN |
| PHASE 3B: real hardware H.264 | VERIFIED | Latest bounded-admission scope: recovered1810.0008772s sustained run,source101797/56.241409FPS,Host93168/51.474008FPS,93069 submitted=encoded=decoded/51.419312FPS.99 explicit transient pressure drops; strict all-Host assertion FAIL preserved. Exact accounting,queue3/1,stable observed resources/latency. Clean654+509 and forced-reconnect1093 frames decode/content verified; latter1 explicit pressure drop. PnP0/0/security unchanged. Not lossless-all-Host or60FPS proof. See docs/PHASE3B_RESULTS.md |
| PHASE 3C: versioned transport | VERIFIED | flow-content-3:35.0015254s normal1782 sent=ACKed=decoded;45.0023031s receiver reconnect with new session/IDR/SPS/PPS,1506 recovered frames byte/decode/content verified. Exact bounded accounting and clean shutdown; PnP0/0/security unchanged. Historical failures preserved. See docs/PHASE3C_RESULTS.md |
| PHASE 3D: live device simulator | VERIFIED | Owner-observed `visual-flow-3` plus independent review: normal35.0282436s,1278 encoded/received,1191 Presented,64/64 source matches; one bounded WM_NCLBUTTONDOWN window-drag stall overflow+82 resync,automatic IDR recovery. Real reconnect45.0069927s:planned WM_CLOSE/fresh process;848 captured=decoded,847 Presented,48/48 matches,zero fresh-receiver overflow/resync. Owner confirms motion before/after reconnect. Measured old-shutdown→new-first-Present8.6608s (owner perceived15–20s), retained as latency limitation. Resources within unchanged limits;hardware D3D11/NV12 decode,no fallback;PnP0/0,SecureBoot/HVCION,TESTOFF,clean shutdown. Historical visual-flow-1 stays UNKNOWN. See docs/PHASE3D_RESULTS.md |
| PHASE 3D diagnostic investigation | VERIFIED | Bounded asynchronous64-pixel diagnostic path validated live: normal sparse poll max0.0236ms; reconnect max0.0149ms. Normal receiver overflow/resync0. The reconnect-only cold-start loss is independently localized to first-frame decoder/render startup and fully bounded/accounted; no acceptance limit changed. Historical +36/late overflow attribution remains UNKNOWN |
| PHASE 3E: simulated touch | BLOCKED | PHASE3D hard-stop reached; not authorized or started. Implementation NOT YET TESTED |
| PHASE 3F: pipeline automated tests | PARTIAL | PHASE2/3A/3B retained;58,161 protocol/parser/queue/resync checks,slow receiver,real transport/reconnect and shutdown pass. Future unrelated tests NOT YET TESTED |
| DEVICE PHASE 2C-T: real touch transport | PARTIAL | Profile-1 real MotionEvent link reaches correctly mapped Windows target; single/two-contact lifecycles, fresh reconnect, stale-session rejection and target-side release observed. Release update returned timeout then invalid-parameter around controlled disconnect, so no universal release claim. `mtp,adb` unchanged; no HID/gadget/native-input/security change. See docs/device/DEVICE_PHASE2CT_RESULTS.md |
| DEVICE PHASE 2C-T1: disconnect release reliability | VERIFIED | Old timeout proven to originate at `InjectTouchInput`; old invalid-parameter value localized to late/unreliable error capture. Single worker now owns all injection/release. 180 Windows scenarios and repeated real single/MOVE/two-contact disconnects plus Host shutdown end cleanly with zero accepted API failures/stuck contacts. 2C-T remains PARTIAL. See docs/device/DEVICE_PHASE2CT1_RESULTS.md |
| DEVICE PHASE 2C-PERF: sustained cadence diagnosis | VERIFIED | Two real 2400x1080/60/30 Mbps ADB/Android runs reproduce source 53.92–54.86 FPS versus Host/downstream 26.23–28.01 FPS. Source-only, hardware-encode-only and local-transport controls sustain 50.38–54.12 FPS. First divergence is IddCx source -> Host admission when the real target is active; exact ADB/receiver/Host mechanism remains UNKNOWN. See docs/device/DEVICE_PHASE2C_PERF_RESULTS.md |
| DEVICE PHASE 2C-PERF1: Host/ADB micro-timing | PARTIAL | Normal full/reduced-evidence, real ACK-only and uninstrumented legacy-Host controls all sustain source/Host about 56.3 FPS; the historical 26–28 FPS state did not reproduce. Current socket writes, ACK worker and measured locks do not block Host admission; the first component responsible for the degraded state remains UNKNOWN. See docs/device/DEVICE_PHASE2C_PERF1_RESULTS.md |
| DEVICE PHASE 2C-PERF2: degradation-triggered flight recorder | VERIFIED | A bounded real normal-Android observation triggered after 1,604 s of healthy state and retained 61.360385 s pre/60.070676 s post with zero trace loss. Post-trigger source 51.972114 FPS versus Host 20.425940 FPS. The first changed blocking boundary is synchronous AMD hardware-MFT `HaveOutput` event service on the Host admission thread; no fix was implemented. See docs/device/DEVICE_PHASE2C_PERF2_RESULTS.md |
| DEVICE PHASE 2C-PERF3: encoder event isolation | VERIFIED | One MTA worker owns every AMD MFT call. The 1800.004732-second real run sustained source/Host 53.358749/51.527087 FPS, ratio 0.965673, without a PERF2 trigger; Android decoded 92,370 frames with zero protocol/queue/decoder error. See docs/device/DEVICE_PHASE2C_PERF3_RESULTS.md |
| DEVICE PHASE 2C-1: native USB architecture gate | VERIFIED | Read-only exact-ROM audit proves stock NCM and `ncm,adb` init branches, compiled FunctionFS/NCM/HID, Windows inbox NCM/WinUSB/HID support and a level-2 userspace ownership boundary. Selected first prototype direction is NCM+ADB with the unchanged TCP protocol; no live composition or gadget change occurred. See docs/device/DEVICE_PHASE2C1_ARCHITECTURE.md |
| DEVICE PHASE 2C-1E1: stock NCM+ADB enumeration | DENIED | The authorized stock framework request `svc usb setFunctions ncm` returned 255 and never changed `mtp,adb`; no phone NCM interface or Windows NCM device appeared. Same-path rollback/final verification passed, persistent USB properties and Windows driver inventory were unchanged, and ADB/MTP remained healthy. See docs/device/DEVICE_PHASE2C1E1_RESULTS.md |
| DEVICE PHASE 2C-1E1A: stock NCM control-path diagnosis | VERIFIED | Read-only exact-framework/runtime reconstruction shows the E1 request reached `UsbHandlerLegacy`, briefly selected `ncm,adb`, and reconnected ADB; a later `mtp,adb` request is strongly localized to an interaction in Xiaomi's USB details activity. Exact clicked row and Windows NCM enumeration remain UNKNOWN. Historical 2C-1E1 stays DENIED. See docs/device/DEVICE_PHASE2C1E1A_DIAGNOSIS.md |
| DEVICE PHASE 2C-1E1B: hands-off NCM+ADB enumeration replay | PARTIAL | Stock Level-2 control reached `ncm,adb`; phone `usb0`, healthy Microsoft inbox `UsbNcm` and concurrent ADB were observed while MTP was absent. A recorded DOWN/UP in Xiaomi USB details then requested `mtp,adb`, so 30-second stability/no-second-actor acceptance did not pass. Stock rollback and final ADB/MTP health passed; speed remains UNKNOWN. See docs/device/DEVICE_PHASE2C1E1B_RESULTS.md |
| DEVICE PHASE 2C-1E1C: USB UI input-source isolation / NCM hold | PARTIAL | Real `ncm,adb`, phone `usb0`, healthy Microsoft inbox `UsbNcm` and concurrent ADB were reproduced. The 120-second hold failed after a 103 ms DOWN/UP from the physical touchscreen kernel path caused Xiaomi USB details to request `mtp,adb`; real contact versus ghost/electrical input remains UNKNOWN. Autonomous return and final ADB/MTP health passed. See docs/device/DEVICE_PHASE2C1E1C_RESULTS.md |
| DEVICE PHASE 2C-1E1D: screen-off NCM stability gate | VERIFIED | Owner-initiated locked/noninteractive screen-off state suppressed the E1C interaction path: zero kernel input events, no USB-details activity launch and no autonomous MTP request. Real `ncm,adb`, `usb0`, inbox `UsbNcm` and ADB remained healthy for 121.364 s; stock rollback restored healthy `mtp,adb`. A USB notification briefly activated low-power AOD/DOZE without interactive wake. See docs/device/DEVICE_PHASE2C1E1D_RESULTS.md |
| DEVICE PHASE 2C-1E2: ephemeral NCM IP + TCP data plane | BLOCKED | Real `ncm,adb`, phone `usb0`, healthy Microsoft inbox `UsbNcm` and ADB were established. Stock `ndc interface setcfg usb0 10.77.77.2 30 up` returned 1 and created no address under the authorized shell boundary. Windows IP, ping and TCP were therefore not attempted. Cleanup and stock rollback restored healthy `mtp,adb` with no persistent USB/network/security change; the authorized updated project APK remains installed. See docs/device/DEVICE_PHASE2C1E2_RESULTS.md |
| DEVICE PHASE 2C-1E2A: stock `usb0` address ownership | VERIFIED | Read-only source/runtime evidence identifies NetworkStack/Tethering plus `netd` as the stock downstream-address owner. Direct `ndc` requires `NETWORK_STACK` or `MAINLINE_NETWORK_STACK`, which shell lacks; the ordinary APK has only `INTERNET`. This ROM's tethering owner is configured for RNDIS and exposes no NCM-specific path, so the result is stock owner present but no accessible path. E2 remains BLOCKED. See docs/device/DEVICE_PHASE2C1E2A_DIAGNOSIS.md |
| SWEETDISPLAY FINAL-ARCH PHASE 1 | VERIFIED | Offline architecture correction selects the exact stock kernel/DTB plus a custom Android-derived minimal userspace: selected native graphics/media services and exact vendor components, SweetDisplay-owned native EGL/GLES UI, NCM/TCP first transport and protocol touch before HID. MIUI+APK remains validation-only. Temporary boot is technically plausible but live-unverified; next gate is offline FINAL-BOOT PREP 1. See docs/final-architecture/SWEETDISPLAY_FINAL_ARCH_PHASE1.md |
| SWEETDISPLAY FINAL-BOOT PREP 1 | VERIFIED | Offline stock round-trip is byte-identical across the complete 22,257,664-byte boot body. A deterministic private 25,673,728-byte header-v2 candidate preserves the exact stock kernel/DTB/cmdline, carries a 66-entry no-persistence enforcing-recovery-policy ramdisk and a static SweetDisplay DRM/Goodix/DWC3/SELinux diagnostic. Structural/audit checks pass. `fastboot boot` remains live-unverified; decision is GO only for separately authorized FINAL-BOOT 1. See docs/final-architecture/SWEETDISPLAY_FINAL_BOOT_PREP1.md |
| SWEETDISPLAY FINAL-BOOT 1 | FAILED | The single exact-hash `fastboot boot` attempt passed the `sweet`/unlocked/768-MiB identity gate and Fastboot reported download+boot `OKAY`, but the owner observed stock Android rather than any SweetDisplay UI. Stock Android 13 / `V14.0.2.0.TKFTRXM`, `boot_completed=1` and ADB `device` returned automatically; physical recovery was not used. Post-return USB was `adb`, while pre-Fastboot USB properties were not captured, so exact `mtp,adb` baseline equivalence remains UNKNOWN. No partition-write command or prohibited operation occurred. See docs/final-architecture/SWEETDISPLAY_FINAL_BOOT1_RESULTS.md |
| SWEETDISPLAY FINAL-BOOT 1A | VERIFIED | Host-only review found v1's previously silent approximately 40-second DRM-failure reboot path and confirmed that the 180-second UI timer does not credibly explain the initial 60-second stock observation. A deterministic private v2 preserves the exact stock kernel/DTB, restores stock ueventd rules, starts directly at `late-init`, keeps SELinux enforcing and adds pmsg/kmsg, pulse, exact DRM-stage and tagged-reboot checkpoints. Structural/static/no-persistence checks pass; FINAL-BOOT 1 remains FAILED. Decision is GO only for separately authorized FINAL-BOOT 1B. See docs/final-architecture/SWEETDISPLAY_FINAL_BOOT1A_ANALYSIS.md |
| SWEETDISPLAY FINAL-BOOT 1B | VERIFIED | The one exact-v2 RAM-only attempt returned to Fastboot in about five seconds. Stock `SYSTEM_LAST_KMSG` then proved exact-kernel and ramdisk/PID-1 execution through Android first-stage init. Init aborted at about 1.72 s because required empty `/mnt` and `/debug_ramdisk` directories were missing, then explicitly rebooted to bootloader. Highest proven checkpoint is C3 entry; C3 completion failed and C4 was not reached. Owner physical recovery restored healthy Android 13 / `V14.0.2.0.TKFTRXM`, ADB and the exact pre-boot `mtp,adb` state. No partition-write command or second attempt occurred. See docs/final-architecture/SWEETDISPLAY_FINAL_BOOT1B_RESULTS.md |
| Phone daemon interface contract | NOT YET TESTED | Native custom-environment NCM plus inbox UsbNcm enumeration is proven; phone IP/TCP/SWDP execution remains untested beyond the first denied address ioctl. FunctionFS is outside current scope |
| SWEETDISPLAY FINAL-BOOT BRINGUP | VERIFIED | One V11 RAM-only boot proves stable SweetDisplay-owned AMOLED pixels, enforcing, changing physical-touch coordinates and read-only DWC3 detection. V10's wave/fade/text-loss defect is absent. Kernel records normal reboot at 184.522 s; same-device healthy V14.0.2.0.TKFTRXM/mtp,adb/Enforcing return verified. No flash/persistent change; do not repeat. HARD STOP before FINAL-USB. See docs/final-architecture/SWEETDISPLAY_FINAL_BOOT_BRINGUP.md |
| SWEETDISPLAY FINAL-USB | PARTIAL | Candidate 3 live reached healthy custom NCM/UsbNcm and exact `FU3_ADDR`, but enforcing ioctl xperm denied `SIOCSIFADDR` command `0x8916`; owner photo shows NCM READY / IP ERROR / TCP WAITING. Normal stock return and Windows cleanup passed. Candidate 4 adds only xperm `0x8916` and `0x891c`, remains enforcing and is reproducible offline at SHA-256 `cf0158c10fc083c2fc4e71e75a0fa828fb3b17e0b4d5b7277ce9dcd389fb5938`; fresh owner gate required. See docs/final-architecture/SWEETDISPLAY_FINAL_USB.md |
| Phone hardware work | PARTIAL | FINAL-BOOT BRINGUP remains VERIFIED. Three FINAL-USB RAM-only attempts failed safely and returned healthy stock; custom NCM/UsbNcm and the first address-ioctl boundary are live-proven, with evidence-backed Candidate 4 ready offline. No persistent installation or partition-write command occurred |
| Repository publication hygiene | VERIFIED | FINAL-USB Candidate 4 gate passed `git diff --check` and the publication checker for 279 publishable files, the index and 125 history blobs. Public material contains no unique device/USB identity, MAC, personal path, raw private evidence or generated image/binary; private artifacts are ignored. No commit/push |

Current acceptance is defined in docs/CLASSIFIED_ACCEPTANCE.md. Both new live
classification pilots passed without D/E; legitimate B/C content no longer fails
pipeline integrity. The new 300-second observation and fresh-process reconnect
are VERIFIED. The same UAC controller completed the fresh 1810.0007551-second
soak, reconnect and clean shutdown in the background. Independent artifact
verification and resource review passed without another soak or live test.
PHASE 3A is VERIFIED at the measured achievable rate; 60 FPS is not proven.
The old soak remains UNKNOWN. See docs/PHASE3A_RESULTS.md. The latest attached
request now authorizes PHASE 3D only, stopping before3E. Recovered sustained hardware encode and both
reconnect paths are now VERIFIED under its bounded-admission criteria; explicit
99 sustained and1 post-crash admission drops prevent a lossless-all-Host claim.
All admitted outputs independently decode/content match. Early slowdown and
underlying transient token timing remain UNKNOWN; see docs/PHASE3B_RESULTS.md.
PHASE 3C is VERIFIED for bounded localhost hardware-H.264 transport and real
receiver reconnect. Normal35.0015254s run:source56.483253FPS,Host51.026348FPS,
1782 encoded=received=ACKed=independently decoded at50.912067FPS;4 explicit encoder
pressure drops. Reconnect45.0023031s:2106 encoded/Host-decoded,1907 received/ACKed,
401 before termination and1506 after fresh handshake/IDR recovery.160 disconnected,
38 resync-skipped and1 unconfirmed attempt remain explicit;9 encoder pressure
admission drops. Shared/encoder/transport peaks3/1/1,invalid/D/E0,clean final exits0,
PnP0/0,SecureBoot/HVCION,TESTOFF. No60uniqueFPS or long transport soak claim.
The fixed receiver-first shutdown branch passed live. Old474-frame E/UNKNOWN and
flow-content-2 controller ERROR are preserved. See docs/PHASE3C_RESULTS.md.
PHASE3D is VERIFIED. `visual-flow-3` passes the >=35-second real live path and
45-second real reconnect with independent decode/content/render review, bounded
accounting, resource limits, health, clean shutdown and owner-confirmed motion
before/after reconnect. The measured8.6608s reconnect blackout remains a documented
latency limitation. Stop after3D; no touch/USB/phone work.

Earlier investigation: second grey cover, old-counter overlay and minimize controls
all stopped on their first bad content with one small private crop per session.
The corrected PID-checked producer-window close is now VERIFIED: normal exit 0,
no forced kill, repeated in four sessions. Metadata/slot identity checks pass.
The follow-up short series stopped at 17.354111 s (frame 408355, slot 1); its BMP
exactly matches the minimize control. Explorer was foreground and the owner
confirms Show Desktop during the test. Legitimate desktop replacement is strongly
corroborated for that latest event; no pipeline bug is proven. Controlled foreground
activation itself returned 0 and is not VERIFIED. At that earlier checkpoint the
five-minute observe run had not started. The newer corrected observation above
supersedes that pending gate; historical soak cause remains UNKNOWN. No driver,
security or phone changes were made.
See docs/PHASE3_FIRST_FAIL.md for evidence, acceptance limits and next bounded steps.

DEVICE PHASE 2C-T is PARTIAL. A negotiated touch profile carried real Android
MotionEvents through the existing ADB forward to the dynamically discovered
SweetDisplay target. The independent target saw tested control and two-contact
lifecycles. During a controlled receiver restart, it saw terminal UP for the
active contact and a fresh session followed; a protocol test rejects a retired
session's touch record. A Windows release-update call nevertheless returned a
timeout followed by invalid parameter, so the disconnect gate is not promoted to
VERIFIED. USB remained `mtp,adb`; no HID/gadget, root, security, partition or
Fastboot action occurred. Stop after 2C-T.

DEVICE PHASE 2C-T1 is independently VERIFIED and does not relabel the historical
2C-T result. The old 1460 was captured directly from failed `InjectTouchInput`;
the later 87 came from a post-unwind `GetLastError` read and cannot be attributed
to a second API call. The corrected transport-worker-owned state machine records
every call immediately, retries only bounded `ERROR_NOT_READY`, sends matching
UPDATE then UP for all active contacts, and retains state on uncertain release.
The 180-case local matrix, nine accepted real disconnect cycles and active-touch
normal Host shutdown all ended without API failure or stuck target contact.
Video recovered through bounded reconnect accounting, USB stayed `mtp,adb`, and
Windows security/PnP health was unchanged. Hard stop after 2C-T1.

DEVICE PHASE 2C-PERF is VERIFIED as a diagnostic localization, not as a
performance fix. The 900-second real-target run averaged source54.857580FPS
versus Host28.012097FPS; a separate120-second real-target control reproduced
source53.924370FPS versus Host26.228973FPS. The first formal adjacent-stage
divergence is IddCx source to Host admission. Source-only, AMD-hardware-encode
only and local-transport controls remain approximately50–54FPS. Downstream
encoder, sender, ACK, Android receive, Qualcomm decode and render rates match the
already-reduced Host rate with bounded queues and zero protocol/decoder errors.
Thermal status stayed0; startup resource growth plateaued and does not establish
a leak. The exact mechanism inside the real ADB/Android target interaction is
UNKNOWN. No fix or later phase was started; see docs/device/DEVICE_PHASE2C_PERF_RESULTS.md.

DEVICE PHASE 2C-PERF1 is PARTIAL. Opt-in bounded QPC timing, normal
full/reduced-evidence controls, a real Android ACK-only path and the earlier
uninstrumented Host binary all sustained source/Host at approximately56.3FPS.
The historical26–28FPS state did not reproduce after a host ADB-server restart,
so no degraded-state micro-trace exists and the exact first blocking component
remains UNKNOWN. In the current state, socket writes, ACK receive, measured
transport locks, keyed-mutex waits and evidence flushes are not blocking Host
admission at frame-period scale; normal decode/Surface and ACK-only rates are
equivalent. The ADB restart is temporal correlation only, not a causal finding.
No performance correction or later phase was started; see
docs/device/DEVICE_PHASE2C_PERF1_RESULTS.md.

DEVICE PHASE 2C-PERF2 is VERIFIED / TRIGGERED. The bounded recorder preserved
61.360385s pre-trigger and60.070676s post-trigger with278,084 records and zero
loss. Its10.195552s trigger window measured source52.179618FPS,Host37.761565FPS
and ratio0.723684211; the post interval reached source51.972114FPS versus
Host20.425940FPS. The first changed blocking boundary is the AMD hardware-MFT
`HaveOutput` event-service scope in the main Host loop: median rose from6.1213ms
healthy to26.4200ms degraded while output processing, socket/ACK, transport and
admission checks remained sub-frame. ADB process/listener state and USB
`mtp,adb` were unchanged. The scope does not separate `GetEvent` from event
status/type inspection or explain the MFT state transition. No correction or
later phase was started; see docs/device/DEVICE_PHASE2C_PERF2_RESULTS.md.

DEVICE PHASE 2C-PERF3 is VERIFIED. Exactly one MTA worker now owns all AMD MFT
activation, event, input, output, drain and shutdown calls. The Host admission
thread performs no `HaveOutput` service. Its bounded input queue is capacity4
with reject-newest accounting; the existing sender output queue remains
capacity3. GPU-native input and hardware-only encoding remain true.
`perf3-phone-sustained-1` completed1800.004732s at source53.358749FPS and
Host51.527087FPS,ratio0.965673, with no PERF2 trigger. Encoder accepted/output
92489/92489 after260 explicit bounded backpressure drops; Android decoded92370
frames with zero protocol,queue-overflow or decoder error and one clean drain.
USB stayed `mtp,adb`; thermal status remained0. The owner confirmed the moving
pink/black controlled pattern in the accepted short run. This is not a claim
that a production interactive third-screen UI is complete. The bounded real
touch regression passed one contact,drag,natural release and final zero active
contacts at Host,Android and the independent Windows target. See
docs/device/DEVICE_PHASE2C_PERF3_RESULTS.md.

DEVICE PHASE 2C-1 is VERIFIED as an architecture gate only. The exact stock ROM
pre-creates an NCM function and defines `ncm` plus `ncm,adb` init compositions;
the latter retains ADB. FunctionFS and HID are compiled but no stock
SweetDisplay FunctionFS endpoint or HID node/composition exists. Windows 11 has
inbox NCM, WinUSB and HID support. The selected first native prototype direction
is the dormant stock NCM+ADB path with the unchanged TCP protocol, requiring
level-2 system/init control; live enumeration remains UNKNOWN. A custom kernel is
not required. No USB composition, property, service, UDC, driver or phone state
was changed. See docs/device/DEVICE_PHASE2C1_ARCHITECTURE.md.

DEVICE PHASE 2C-1E1 is DENIED without relabeling the verified architecture gate.
The single authorized high-level request, `svc usb setFunctions ncm`, returned255
and the phone remained `mtp,adb`; ADB stayed available and neither a phone NCM
interface nor a Windows NCM PnP/network device appeared. The runtime Gadget HAL
remains unknown and the stock `svc` client exposes NCM only with Gadget HAL1.2.
No property,ConfigFS or UDC bypass was attempted. Same-path rollback through
`svc usb setFunctions mtp` passed; final USB was `mtp,adb`,ADB/MTP problem codes
were0,persistent USB properties and third-party network-driver inventory were
unchanged,and no IP/network operation occurred. See
docs/device/DEVICE_PHASE2C1E1_RESULTS.md.

DEVICE PHASE 2C-1E1A is VERIFIED as a read-only control-path diagnosis and does
not relabel 2C-1E1. Exact Xiaomi framework bytecode and retained runtime records
show that `svc` parsed NCM, called `IUsbManager`, and the no-Gadget-HAL runtime
used `UsbHandlerLegacy` to request `ncm,adb`. USB disconnected/reconfigured and
the authorized ADB host returned. A later framework request restored `mtp,adb`;
its timing is strongly localized to an interaction in Xiaomi's USB details
activity, although the exact clicked row is UNKNOWN. Existing stock Level-2
control is therefore accessible and a custom kernel remains unnecessary. The
brief Windows interface state was not captured, so NCM enumeration is still
UNKNOWN. The phone stayed `mtp,adb` throughout this read-only diagnosis; see
docs/device/DEVICE_PHASE2C1E1A_DIAGNOSIS.md.

DEVICE PHASE 2C-1E1B is PARTIAL. The single stock NCM request reached and briefly
held `ncm,adb`; Android created `usb0`,Windows bound healthy problem-code-0
Microsoft inbox `UsbNcm`,ADB returned concurrently,and MTP disappeared. Xiaomi's
automatically opened USB details activity then received a recorded DOWN/UP and
immediately requested `mtp,adb`,despite the hands-off procedure. The input source
is UNKNOWN,so full-window stability and no-second-actor criteria are not met.
Mandatory stock rollback returned0 and final `mtp,adb`,ADB/MTP health,persistent
USB properties,driver inventory and active network-profile state all matched
baseline. No IP/TCP or later native-USB phase was run; see
docs/device/DEVICE_PHASE2C1E1B_RESULTS.md.

DEVICE PHASE 2C-1E1C is PARTIAL. The single stock request again established
`ncm,adb`,phone `usb0`,healthy Microsoft inbox `UsbNcm`,concurrent ADB and MTP
absence. Xiaomi USB details opened automatically. Passive kernel input evidence
then captured a103ms DOWN/UP from the physical touchscreen path; the framework
requested `mtp,adb` about99ms after the aligned input-interaction record. This
rules out the observed mouse,button,accessibility and virtual-input paths,but
does not distinguish real conductive contact from a controller/electrical ghost
event. The independently established hold lasted only about3.05s rather than
120s. Because the phone had already returned to `mtp,adb`,no redundant rollback
request was issued; final ADB/MTP/problem-code,driver-inventory and active-profile
checks passed. A local summary-expression fault after final capture lost the
exact request exit and USB-speed values; the ignored controller was corrected.
No IP/TCP or later phase ran; see docs/device/DEVICE_PHASE2C1E1C_RESULTS.md.

DEVICE PHASE 2C-1E1D is VERIFIED. With the owner placing the phone on a dry
nonconductive surface and physically entering the locked/noninteractive
screen-off state,the single stock request established `ncm,adb`,phone `usb0`,
healthy Microsoft inbox `UsbNcm` and concurrent ADB. All acceptance conditions
held for121.364s with zero passive kernel-input events,no USB-details activity
launch and no autonomous `mtp,adb` request. The USB notification briefly put the
panel into low-power AOD/DOZE for about9s,but Android never became interactive
Awake/ON and returned to literal OFF. Required stock rollback converged to healthy
`mtp,adb`; persistent USB properties,driver inventory and active profiles were
unchanged. Speed remains UNKNOWN. The next recommended phase is separately
authorized E2 ephemeral NCM IP+TCP proof; it was not begun. See
docs/device/DEVICE_PHASE2C1E1D_RESULTS.md.

DEVICE PHASE 2C-1E2 is BLOCKED. The screen-off run again established real
`ncm,adb`,phone `usb0`,healthy Microsoft inbox `UsbNcm` problem code0 and
concurrent ADB. The least-invasive stock runtime request,`ndc interface setcfg
usb0 10.77.77.2 30 up`,returned1 with no diagnostic and created no IPv4 address.
No Windows peer address,ping or TCP was attempted. Endpoint cleanup and stock
rollback restored healthy `mtp,adb`;temporary address/route,ADB tunnel,firewall,
gateway,DNS,NAT/bridge,persistent USB/network and driver changes are zero. The
next recommendation is read-only E2A stock usb0 address-ownership diagnosis,not
E3. See docs/device/DEVICE_PHASE2C1E2_RESULTS.md.

DEVICE PHASE 2C-1E2A is VERIFIED as a read-only ownership diagnosis and does not
relabel E2. Android 13 `ndc` calls `netd` directly; `interfaceSetCfg` requires
`NETWORK_STACK` or `MAINLINE_NETWORK_STACK`,neither granted to the UID-2000
shell. NetworkStack/Tethering and `netd` are the stock downstream-address owner,
but this ROM selects RNDIS for USB tethering,has no NCM-specific tether regex and
did not claim composition-only `usb0`. The ordinary receiver requests only
`INTERNET`,and no public or registered stock API lets it assign `usb0`. Result:
stock owner exists but no accessible current Level-2 path. E2's prepared probe
remains OFFLINE VERIFIED / LIVE NCM UNVERIFIED. Its then-recommended Level-3
gate is now subsumed by the authoritative final-architecture correction below;
E2B/E3 did not start. See
docs/device/DEVICE_PHASE2C1E2A_DIAGNOSIS.md.

SWEETDISPLAY FINAL-ARCH PHASE 1 remains VERIFIED. **MIUI + the Android receiver
APK is a development/validation platform,not the final product architecture.**
SWEETDISPLAY FINAL-BOOT PREP 1 is also VERIFIED for its offline scope. The stock
host toolchain reproduced the complete 22,257,664-byte stock boot body
byte-for-byte. A deterministic private `SWEETDISPLAY-TEMP-BOOT-v1` was built at
25,673,728 bytes with the exact stock kernel/DTB/header/cmdline,a 66-entry
no-persistence ramdisk and a static native DRM/Goodix/DWC3/SELinux diagnostic.
It contains no ADB,Fastboot service,network,block device,fstab,userdata/metadata/
cache path or partition tool. Its SHA-256 is
`af7ff18d9fa87929b6eff1949180f9ea5788fa0d079897222d218c356852af9e`.
The offline decision is **GO FOR SEPARATELY AUTHORIZED FINAL-BOOT 1**.
That separately authorized gate is now FAILED:one exact-hash RAM-only
`fastboot boot` was accepted,but no SweetDisplay UI appeared and stock Android
returned automatically. No partition-write command was issued and no second
attempt occurred. The failure point before visible custom userspace remains
UNKNOWN;see docs/final-architecture/SWEETDISPLAY_FINAL_BOOT1_RESULTS.md.

PHASE 1 and PHASE 2 remain complete. The historical failed long test reached
30 minutes 10 seconds, but Host exited 1 with ERROR_INVALID_DATA (13 / 0x0000000D),
"integration validation failed; inspect report". Nonce mismatches cluster at
90.7196575–94.3163347 seconds; one counter regression occurs just before them.
Transient desktop dimming/overlay is SUSPECTED, not proven. The runner stopped
before its fresh-process reconnect. No later encoding/protocol/simulator work
was started. Do not bypass this failure or call the run VERIFIED.

Raw throughput: source 55.469001 FPS, Host 54.325356 FPS; 44 producer replacements,
1,996 Host stale drops and 30 contention drops, peak three slots. Source interval
p50/p95/p99/max: 16.7310 / 33.3764 / 33.9865 / 162.0360 ms. The 174 content
mismatches are additional acceptance failures, not queue drops. All metadata IDs
and source timestamps remain ordered and all geometry is 2400x1080 BGRA8.

Resource observations after warm-up show bounded variation, no continuing growth:
driver handles 933–935, returning to 928 after cleanup/settling; private bytes
also fall after cleanup. Both PnP classes remain OK / problem 0, selected failure
events 0, and boot/security state unchanged. Installed driver DLL/INF/CAT and
shared ABI hashes match the PHASE 2 backup. See docs/PHASE3_VALIDATION.md.
No background timer, scheduled phone work or automatic next phase was created.

PHASE 2 deployment history: Secure Boot/HVCI stayed ON, TESTSIGNING OFF and normal signature enforcement ON.
No boot/security setting or phone state was changed. No new certificate or trust
entry was needed for PHASE 2. Only the explicit frame-handoff source change
required a driver-package update; the existing monitor package was preserved.

The existing helper and installed deployment were retained during final tests.
Both device classes remained OK / problem 0; SWT0001 stayed 2400x1080@60. Host
clean disconnect and forced termination did not restart the driver or machine.
The final event query found zero matching driver/system failures. Exactly one
PHASE 1 BMP exists; PHASE 2 only sampled two small pixel rows in memory.

Detailed commands, measurements and corrections: docs/TEST_LOG.md.
Architecture, measurements and reproduction: docs/FRAME_HANDOFF.md.
Raw frame, local topology, certificate and process details remain ignored.
