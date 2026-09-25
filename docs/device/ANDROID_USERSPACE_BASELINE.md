# Android stock-userspace baseline

This baseline describes the least-privileged device endpoint established by
DEVICE PHASE 2B. It is an implementation contract, not permission to change the
live USB gadget.

## Verified application envelope

The first receiver can be an ordinary Android application with no dangerous or
special permissions. It can:

1. receive a bounded byte stream through an already available transport;
2. parse SweetDisplay protocol 1.0 incrementally;
3. validate FRAME size, metadata, CRC and Annex-B structure;
4. submit complete access units through MediaCodec ByteBuffer input;
5. render Qualcomm hardware-decoder output to a SurfaceView;
6. capture foreground touch through MotionEvent; and
7. return protocol telemetry and future negotiated touch messages.

The measured application domain was normal `untrusted_app`. Hardware decode,
fullscreen presentation and touch capture do not require shell, system-app,
root, custom SELinux policy or a custom kernel.

## Display and codec baseline

| Item | Verified value |
|---|---|
| Physical display | 1080x2400; 60 and 120 Hz modes |
| Probe mode | 1080x2400 at approximately 120 Hz |
| Surface sizes | 1080x2307 portrait; 2307x1080 landscape |
| Hardware decoder | `OMX.qcom.video.decoder.avc` |
| Decoder identity | vendor=true, hardware-accelerated=true, software-only=false |
| Tested workloads | 1280x576 nominal 30 FPS; 2400x1080 nominal 60 FPS |
| Tested AVC result | 90/90 and 120/120 decoder outputs, no decoder error |
| Profiles/level | Baseline, Main, High, Constrained Baseline/High through Level 5.1 |
| Feature flags | adaptive=true; secure=false on normal component; tunneled=false; low-latency=false |
| Surface output | vendor color format 2141391878; 2400x1080 visible crop from 2432x1088 coded output |

The reported framework color formats also expose standard planar, semi-planar
and flexible YUV420 buffer formats. ByteBuffer **input** was exercised. Raw
ByteBuffer **output** was not exercised because the primary path deliberately
used Surface output. Surface use alone is not evidence of zero-copy.

## Receiver constraints

- Keep the protocol's 4 MiB AU bound and one-outstanding-FRAME policy. The tested
  AUs were much smaller, so the existing bound remains conservative.
- Configure the decoder with FRAME visible width/height; honor output crop and
  coded stride/slice height. Do not treat 2432x1088 allocation as visible size.
- Feed complete Annex-B AUs in order. On a fresh session or continuity loss,
  suppress dependent frames until one AU contains IDR+SPS+PPS.
- Use the Surface callback/presentation signal separately from decoder output.
  The short tests produced fewer render callbacks than decoder outputs.
- Keep decoding and protocol I/O off the UI thread. Bound input, parser and
  presentation queues; never build an unbounded latency backlog.
- Use independent-clock mode. Android `System.nanoTime` and Windows QPC cannot be
  subtracted until an explicit clock synchronization model exists.
- Treat rotation as a Surface/layout change. The physical mode remained
  1080x2400 while the app Surface changed between 1080x2307 and 2307x1080.

## Touch baseline

Goodix is mapped by InputReader to logical X 0..1079 and Y 0..2399. The app must
consume MotionEvent action, pointer ID, coordinates, pressure and event time,
then normalize against the current physical orientation and negotiated target
range. It must not read or grab `/dev/input/event*`.

The probe verified single-contact pointer ID 0 with DOWN/MOVE/UP. Multi-touch,
palm rejection, stylus/buttons and Windows absolute-HID mapping remain later
work. TOUCH is reserved and rejected in protocol 1.0, so capture does not yet
create an interoperable touch channel.

## Privilege boundary

An ordinary APK should not attempt DRM master, raw VIDC, ION, ConfigFS, UDC or
input-node access. These interfaces are owned by graphics/media/init/vendor
services and SELinux policy. Framework APIs already provide the verified display,
decode and touch paths.

For the first distributed prototype, ADB-forwarded TCP is the minimum-risk level
B bridge and can reuse the existing ByteStream transport contract. A dedicated
production USB function is not an APK feature: it requires a separately designed
privileged service/composition policy and Phase 2C evidence.

## Disposable probe

`device/android-userspace-probe` contains auditable source. The offline build
script derives a bounded private fixture from existing Windows evidence, creates
a temporary local signing key and leaves the APK, fixture and key under ignored
`.local` storage. It performs no download. The manifest requests no Android
permissions and changes no setting.

The operator manually enabled MIUI `Install via USB` to allow installation. That
operator action is not a requirement of the eventual production receiver and is
not evidence of application privilege.

