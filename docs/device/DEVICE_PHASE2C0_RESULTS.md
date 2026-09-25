# DEVICE PHASE 2C-0 results — first real end-to-end link

Date: 2026-09-21. Result: **VERIFIED** for the ADB-transported development
profile defined by this phase.

Real Windows Display 3 content passed through the existing classified capture
path, AMD hardware H.264 encoder, SweetDisplay 1.0 byte stream, ephemeral ADB
TCP forwarding over the physical USB cable, an ordinary Android APK, Qualcomm
hardware AVC decode and a fullscreen Surface on the Redmi Note 10 Pro AMOLED.
The operator visually confirmed the conservative live image, the 2400x1080
image and controlled reconnect recovery.

This result does not verify sustained 60 FPS, a production USB function,
end-to-end latency, zero-copy, touch transport or HID. One failed long run and
the measured cadence/resource limitations below remain part of the result.

## Security baseline and exact topology

The owner had manually enabled MIUI `Install via USB` before this phase and
explicitly chose to keep it enabled on this dedicated development phone. It was
treated as an existing development baseline, not changed programmatically. The
only installed artifact was the ordinary SweetDisplay receiver APK. Its manifest
requests only `android.permission.INTERNET`, a normal permission.

The selected topology was:

```text
SweetDisplayHost client, Windows 127.0.0.1:48231
  -> adb forward tcp:48231 tcp:48231
  -> physical USB cable using the existing ADB function
  -> Android ServerSocket, 127.0.0.1:48231
  -> protocol parser -> MediaCodec -> SurfaceView
```

The USB composition was `mtp,adb` before and after every controller run. ADB
forwarding was removed after each run and no persistent forward is assumed. No
ConfigFS path, descriptor, FunctionFS endpoint, NCM function, HID function, UDC
binding or live gadget property was changed.

## Receiver implementation

`device/android-receiver` is a bounded ordinary application. Protocol and codec
work run off the UI thread. The parser uses repeated reads to assemble the exact
48-byte header and bounded payload, so TCP/ADB read boundaries have no message
meaning. It validates magic, version 1.0, fixed payload sizes, the 4 MiB AU cap,
session, sequence, monotonic timestamps, source-clock association, geometry,
Annex-B structure, IDR/SPS/PPS flags and CRC32.

The negotiated device clock mode is independent (`0`); Windows QPC is never
subtracted from Android `System.nanoTime`. HELLO, CAPABILITIES, FRAME,
TELEMETRY, HEARTBEAT and CONTROL DRAIN/DRAIN_ACK were exercised. An unexpected
or unnegotiated message, including TOUCH, remains a protocol violation.

The decoder worker has a four-item admission queue and a 250 ms input deadline.
It selects a non-secure hardware AVC decoder, configures FRAME visible geometry,
uses complete Annex-B AUs as ByteBuffer input and renders to a Surface. Disconnect
clears queued dependent work; a new session reconfigures the codec only after a
fresh IDR+SPS+PPS AU. The verified decoder was
`OMX.qcom.video.decoder.avc`, reported hardware-accelerated and not software-only.

The built APK was 24,919 bytes with SHA-256
`1ea308e4f2f41f464248a48984cad2bbeb80fb3894fd82cce30751aa38ef3ece`.
The APK, local signing key and runtime logs remain ignored/private.

## Component and live results

The initial recorded-source component check is not labelled live Display 3. It
sent 100 previously captured real access units: 100 protocol frames, 100 decoder
inputs, 100 decoder outputs and 91 frame-render callbacks. Queue peak was four;
CRC/session/sequence/protocol/decoder errors were zero and DRAIN_ACK completed.

The first 30-second live run completed its image path but its controller result
is deliberately preserved as ERROR because a PowerShell wrapper lost the
`logcat -d` argument and its own log collector had to be stopped. Host exit was
zero, 913/913 frames were ACKed and independent review recovered the Android
drain log. The wrapper was corrected; this run is supporting evidence only.

| Live run | Source / Host | Encoder output | Transport | Android and visual result |
|---|---:|---:|---:|---|
| Conservative visual, 1280x576, 60.00 s | 3,024 / 2,846; 50.40 / 47.43 FPS | 2,846; 47.43 FPS; hardware-only; zero encoder drops | 2,846/2,846 ACK; 95,208,118 bytes sent; queue peak 1; zero socket/protocol error | Qualcomm decode; 2,846-session DRAIN_ACK; operator confirmed correct orientation, aspect, motion and no visible corruption |
| Target, 2400x1080 nominal 60, 60.01 s | 3,342 / 3,272; 55.69 / 54.53 FPS | 3,250; 54.16 FPS; 22 bounded backpressure drops | 3,250/3,250 ACK; 203,513,300 bytes sent; queue peak 3; zero socket/protocol error | Qualcomm 2400x1080 decode; 3,250-session DRAIN_ACK; operator confirmed crop/orientation/aspect, with occasional slight stutter |
| Sustained/reconnect accepted, 800x360 conservative, 600.03 s | 32,983 / 21,190; 54.97 / 35.31 FPS | 21,190; 35.31 FPS; hardware-only; zero encoder drop | 21,190 seen; 21,016 ACK; two ready sessions; queue peak 3; exact disconnect/resync accounting | Post-restart fresh process decoded 12,210/12,210 with 12,006 render callbacks; zero Android protocol/CRC/sequence/session/queue/decoder errors; operator confirmed visual recovery |

The target short run proves the required 2400x1080 format on the real panel; it
does not prove sustained 60 FPS. The measured slight stutter is consistent with
54.16 encoder FPS and fewer Android presentation callbacks than decoder outputs.
Decoder output is not relabelled as physical presentation.

## Sustained run and reconnect accounting

The accepted sustained run lasted 600.034 seconds; transport lifetime was
600.239 seconds. The controlled receiver stop began at 180 seconds, remained
down for 3.124 seconds, then relaunched while the same Windows Host process
continued. The first ready session ACKed 8,806 frames. The fresh receiver
process established a distinct ready session and ACKed/drained 12,210 frames.

Exact sender accounting was:

- 21,190 seen = 21,020 admitted + 164 disconnected + 5 resync-skipped + 1
  bounded queue overflow;
- 21,020 admitted = 21,016 ACKed + 3 queue-aborted + 1 unconfirmed; and
- 21,017 wire-complete = 21,016 ACKed + 1 unconfirmed, with zero pending.

Thirty-one socket errors are the bounded connection attempts while the Android
listener was intentionally absent. After the new connection, dependent AUs were
skipped until a valid recovery AU. The device process then reported queue peak
four, final queue depth zero, 305,350,755 AU bytes, 306,741,499 wire bytes and a
clean DRAIN_ACK. The operator confirmed that the image disappeared briefly and
returned moving without restarting the virtual display driver.

An earlier 1280x576 sustained attempt is preserved as a failure. It stopped at
about 451 seconds when the Windows source-content oracle classified one sampled
frame `E: UNCLASSIFIED`; 14,099 earlier source samples were class A. The frozen
evidence shows correct nonce/counter/resource identity but an inconsistent
binary-cell sample after an approximately one-second repeated source pattern.
Android had zero protocol/CRC/sequence/session/decoder errors up to disconnect.
This failure was neither deleted nor promoted.

During both long runs the live rate eventually fell to roughly 23–27 FPS. The
accepted run averaged 35.31 Host/encoder FPS. Cause is **UNKNOWN**; device thermal
or codec behavior, Windows scheduling and the previously observed Windows Host
resource initialization are candidates, not conclusions. Bounded queues and
exact drops prevented latency accumulation.

Windows Host private bytes in the accepted run changed from 33,366,016 to
50,470,912 (peak 50,798,592), and handles from 923 to 1,269 (peak 1,278), then
appeared to plateau. Pattern private bytes remained approximately 26.8 MiB and
handles 794 to 789. This is measured behavior, not proof that the Host has no
resource leak. The growth remains a follow-up item; no guard or threshold was
raised to hide it.

## Touch profile

Real foreground MotionEvent data was captured by the ordinary APK, including
actions, pointer ID, X/Y, pressure and monotonic event time. No negotiated touch
extension was implemented. Protocol 1.0 still reserves/rejects TOUCH, so no
message was silently repurposed and no Windows input injection occurred. A
future profile must negotiate contact IDs, state transitions, coordinate range,
orientation, pressure, device timestamp/session freshness and disconnect contact
release before transport is enabled. USB HID remains out of scope.

## Acceptance and remaining unknowns

All Phase 2C-0 acceptance items were met for the ADB development profile:
real Display 3 content reached the real AMOLED, existing hardware encode and
protocol framing remained in use, Qualcomm hardware decode rendered to Surface,
CRC/session/sequence checks held, USB remained `mtp,adb`, queues were bounded,
controlled reconnect recovered, and the operator supplied visual confirmation.

Remaining unknowns/limitations are the cause of long-run cadence degradation,
the Windows Host handle/private-byte growth, actual scan-out/photons cadence,
cross-device clock correlation and a negotiated touch channel. No end-to-end
latency claim is made. Phase 2C-1, HID and Windows Phase 3E were not started.

No root, su, adb root, remount, SELinux change, fastboot operation, boot/flash,
partition or AVB write, recovery/Magisk action, kernel/module action, ConfigFS
write, UDC action, gadget reconfiguration, NCM configuration or HID creation
occurred.

