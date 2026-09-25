# Android receiver

`device/android-receiver` is the ordinary stock-Android endpoint used by DEVICE
PHASE 2C-0 and the bounded DEVICE PHASE 2C-T touch experiment. It is deliberately
not a system app, accessibility service, VPN, device administrator, overlay or
USB gadget manager.

## Build and install envelope

`scripts/windows/Build-AndroidReceiver.ps1` builds offline with the installed
Android SDK/JDK using aapt, javac, d8, zipalign and apksigner. Build output and
the development signing key live below ignored `.local/device-phase2c0`.

The manifest declares only:

```text
android.permission.INTERNET
```

That normal permission allows a loopback TCP listener. There are no dangerous,
special, USB, storage, input, accessibility, overlay or administration
permissions. The owner manually enabled MIUI `Install via USB` before this phase
and intentionally retains it as a development-device baseline. The receiver was
installed only through an ordinary authorized `adb install`; the application
does not read or modify that setting.

## Threads and bounds

- The Activity/UI thread owns SurfaceView lifecycle, status text and normal
  MotionEvent capture.
- A listener thread owns the loopback ServerSocket, byte-stream framing,
  validation, acknowledgement and reconnect accept loop.
- A decoder thread owns MediaCodec configuration, ByteBuffer input, Surface
  output and drain.
- Protocol payload is capped at the 4 MiB AU limit plus 64 bytes of FRAME
  metadata.
- Decoder admission uses a four-item `ArrayBlockingQueue`; admission waits at
  most 100 ms and codec input at most 250 ms.
- DRAIN waits at most 2.5 seconds and codec EOS at most 2 seconds.
- The Android socket read timeout and Windows sender deadlines prevent an
  abandoned peer from blocking forever.
- Disconnect clears queued compressed work. A fresh session cannot decode until
  the sender admits an AU containing IDR+SPS+PPS.

No unbounded queue, per-frame thread or accumulated Android evidence file exists.
Periodic counters are emitted through logcat and retained only in ignored test
evidence when a controller explicitly captures them.

## Protocol behavior

The parser treats TCP as a byte stream. It repeatedly reads until the exact
header/payload length is assembled and rejects truncated EOF. It validates:

- `SWDP`, version 1.0 and the 48-byte header;
- known type and exact/bounded payload sizes;
- nonzero session, sequence and timestamp;
- one session and contiguous sequence order per connection;
- nondecreasing sender timestamps;
- host/device roles, capability bounds and independent Android clock mode;
- FRAME identity, dimensions, AU length and source tick/frequency association;
- strictly increasing frame ID, source nanoseconds and PTS after the first AU;
- Annex-B start codes, legal NAL headers, a VCL NAL and exact IDR/SPS/PPS flags;
- first-frame IDR+SPS+PPS and CRC32 over the encoded AU; and
- exact TELEMETRY frame/sequence/byte accounting and CONTROL drain semantics.

Supported live flow is HELLO, CAPABILITIES, FRAME/TELEMETRY, HEARTBEAT and
CONTROL DRAIN/DRAIN_ACK. When both endpoints explicitly negotiate profile 1,
TOUCH configuration/ready/contact records are also accepted; otherwise TOUCH is
rejected rather than silently assigned protocol-1.0 meaning.

## Decode and Surface behavior

The receiver enumerates AVC decoders and rejects secure, software-only or
non-hardware candidates. Verified runs selected
`OMX.qcom.video.decoder.avc`. FRAME visible width/height configure MediaCodec;
complete Annex-B AUs enter through ByteBuffer input and decoded output goes
directly to the Activity Surface.

`VIDEO_SCALING_MODE_SCALE_TO_FIT` preserves aspect ratio. The landscape Surface
measured 2307x1080 on the 1080x2400@120 Hz physical mode. The operator confirmed
correct orientation/aspect/crop for real 2400x1080 content. Codec output-format
events expose coded stride/slice/crop; visible geometry is not inferred from the
coded allocation. Render callbacks are counted separately from decoder output.

Surface destruction closes the active socket and decoder. Surface recreation
starts a new listener, forcing normal transport reconnect and fresh codec session
state rather than preserving stale compressed dependencies.

## Touch boundary

The Activity records ordinary `MotionEvent` actions, pointer ID, coordinates,
pressure and event time. Under the opt-in profile 1 it sends bounded normalized
contact records only after the Windows host has supplied the current content
geometry and accepted the ready response. It does not open `/dev/input`, grab
input, use an accessibility service or inject into Windows. It creates no USB
HID device and changes no Android input, USB or security setting. Legacy
protocol-1.0 peers still reject TOUCH.

## Verified runtime summary

The target live run decoded 3,250 2400x1080 AUs with a clean session drain. The
accepted 600-second run used two transport sessions around a controlled process
restart. Its fresh post-restart Android process decoded 12,210/12,210 AUs and
reported 12,006 frame-render callbacks, zero protocol/CRC/sequence/session/queue
overflow/decoder errors, final queue depth zero and approximately 68,622 KiB PSS
at drain. See [DEVICE PHASE 2C-0 results](DEVICE_PHASE2C0_RESULTS.md) for complete
host, transport and limitation accounting.

For DEVICE PHASE 2C-T, real one- and two-contact MotionEvents reached the
independent Windows target with expected per-contact down/move/up transitions.
The reconnect experiment established a fresh session and target-side release,
but a host release API acknowledgement was ambiguous; see
[DEVICE PHASE 2C-T results](DEVICE_PHASE2CT_RESULTS.md). The phase is therefore
PARTIAL, not a claim of a production input path.

The separate DEVICE PHASE 2C-T1 follow-up changed no Android privilege or USB
behavior. Using the same ordinary receiver and ADB forward, repeated active
single-contact, MOVE, two-contact and normal Host-shutdown cases ended with
independent Windows target release, fresh-session touch state and zero accepted
injection API failures. See
[DEVICE PHASE 2C-T1 results](DEVICE_PHASE2CT1_RESULTS.md). The original 2C-T
label remains PARTIAL.
