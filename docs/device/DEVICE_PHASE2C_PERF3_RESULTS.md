# DEVICE PHASE 2C-PERF3 — Encoder event isolation and sustained rate

Date: 2026-09-22  
Result: **VERIFIED**

This phase implements the narrow correction justified by the PERF2 trace:
hardware-MFT event and output service no longer executes on the Host frame-
admission thread. A real 30-minute target run passed without the historical
source/Host collapse. This result verifies the isolation architecture over the
observed failure horizon; it does not prove that the AMD MFT can never become
internally slow.

Historical classifications are unchanged: DEVICE PHASE 2C-PERF remains
VERIFIED, PERF1 remains PARTIAL, and PERF2 remains VERIFIED / TRIGGERED.

## MFT API audit and ownership

Microsoft's asynchronous-MFT contract defines `METransformNeedInput` as one
permission to call `ProcessInput`, `METransformHaveOutput` as one permission to
call `ProcessOutput`, and drain completion through output events followed by
`METransformDrainComplete`. `IMFMediaEventGenerator::GetEvent` is synchronous;
`BeginGetEvent` is the asynchronous alternative and permits only one pending
request. The implementation therefore does not assume that arbitrary concurrent
calls into the AMD MFT are safe.

Authoritative references:

- [Asynchronous MFTs](https://learn.microsoft.com/en-us/windows/win32/medfound/asynchronous-mfts)
- [Media Event Generators](https://learn.microsoft.com/en-us/windows/win32/medfound/media-event-generators)
- [`IMFMediaEventGenerator::BeginGetEvent`](https://learn.microsoft.com/en-us/windows/win32/api/mfobjects/nf-mfobjects-imfmediaeventgenerator-begingetevent)
- [`MFT_MESSAGE_COMMAND_DRAIN`](https://learn.microsoft.com/en-us/windows/win32/medfound/mft-message-command-drain)

Exactly one MTA encoder worker owns MFT activation/configuration, event
retrieval and status/type inspection, `ProcessInput`, `ProcessOutput`, token and
pending-sample accounting, drain, end-streaming and shutdown. The Host thread
does not call `GetEvent` or service `METransformHaveOutput`; accepted evidence
records `mft_single_owner=true`, `host_services_mft_events=false` and a distinct
worker thread ID.

The Host thread retains only shared-frame acquisition, keyed-mutex use,
GPU-native BGRA-to-NV12 conversion, construction of a GPU-backed sample and a
bounded enqueue. It never waits for output-event service merely to fetch the
next driver frame.

## Bounded ownership and lifetime

The new encoder input ring has capacity four. The Host is its sole producer and
the encoder worker its sole consumer. Full behavior is reject-newest with an
explicit `backpressure_drops` increment; work is never accumulated without a
bound. Normal completion drains queued and MFT-owned samples. Abort has a
bounded stop deadline. Worker failure is preserved and propagated to the Host.

Each accepted input owns one of four private NV12 D3D11 textures. A texture is
claimed atomically before conversion and attached to an `IMFTrackedSample`; the
tracked-sample callback is the only path that returns that slot after the MFT
releases it. The Host cannot reuse a busy texture. No CPU frame copy or software
encoder fallback was introduced.

Completed access units pass on the worker thread into the existing sender. Its
queue remains capacity three, rejects overflow explicitly, maintains FIFO
frame/session order and performs bounded reconnect/resync accounting. Final
accepted evidence has zero encoder pending work and zero sender pending work.

## Windows-only validation

`perf3-windows-local-1` ran 120.001892 seconds at 2400x1080, nominal 60 FPS and
30 Mbps with a planned receiver disconnect/restart.

| Measurement | Result |
|---|---:|
| Source / Host | 55.699122 / 54.865801 FPS |
| Host/source | 0.985038884 |
| Host input | 6,584 |
| Encoder accepted/output | 5,197 / 5,197 |
| Explicit encoder backpressure | 1,387 |
| Input queue peak/capacity | 4 / 4 |
| Output queue capacity | 3 |
| Encoder worker output | 43.307650 FPS |
| MFT pending peak/final | 1 / 0 |
| Fresh receiver | 4,237 frames; 40.357087 FPS |

The AMD hardware AVC MFT, GPU-surface input, clean drain and clean shutdown all
remained true; software fallback was false. The planned disconnect produced two
connections and expected socket errors, followed by a fresh clean receiver with
zero protocol/truncation/socket errors. Secure Boot, HVCI, boot identity and PnP
health were unchanged.

This run is important even though the worker produced only about 43.3 FPS: Host
admission stayed at 54.865801 FPS, 98.50% of source. Worker slowness therefore
did not reproduce the old source/Host collapse. The fixed input bound converted
the mismatch into 1,387 explicit drops instead of unbounded latency.

## Real short validation

`perf3-phone-short-3` is retained but excluded. The new high-rate path exposed
the same cold Qualcomm decoder-start queue overflow previously documented in
PERF1: five early frames reached a four-entry queue while MediaCodec was being
configured. MFT isolation itself was healthy, but Android recorded one explicit
overflow and the controller correctly rejected the run.

The narrow correction keeps the queue at four and applies backpressure only to
the first frame of each decoder session until that frame is actually submitted
to MediaCodec. Later frames retain the existing asynchronous queue path. The
updated development APK was built and signature-verified; its SHA-256 is
`63c90dd9388a5a96074162a7474342423aaae22f0e2bfdb35a045ee0be4744cd`.
Ordinary authorized `adb install -r` succeeded.

`perf3-phone-short-6` then passed 120.000310 seconds with no trigger:

| Measurement | Result |
|---|---:|
| Source / Host | 53.316529 / 51.483200 FPS |
| Host/source | 0.965614247 |
| Encoder input/output | 6,178 / 6,178 |
| Input queue peak/capacity | 2 / 4 |
| Encoder backpressure drops | 0 |
| Android frames / decoder outputs | 6,059 / 6,059 |
| Android protocol / queue / decoder errors | 0 / 0 / 0 |
| Clean drains | 1 |
| MFT event-service p50 / p95 / max | 4.7 / 7.0 / 21.7659 ms |

`perf3-phone-short-7` repeated the visual gate and passed at source/Host
52.938444/51.263546 FPS, ratio 0.968361405, with zero Android protocol or queue
error and one clean drain. The owner observed the moving pink/black controlled
test pattern. This proves the validation source reaches the phone correctly; it
does not claim that a production-quality interactive Windows third-screen UI is
complete.

## Sustained real-device acceptance

`perf3-phone-sustained-1` ran the real 2400x1080, nominal 60 FPS, 30 Mbps AMD
hardware AVC to the Qualcomm hardware decoder and Surface for 1,800.004732
seconds. The PERF2 detector remained armed and did not trigger. No raw trace was
retained for the healthy run; bounded online timing summaries were retained.

| Measurement | Result |
|---|---:|
| Source | 53.358749 FPS; 96,046 frames |
| Host admission | 51.527087 FPS; 92,749 frames |
| Host/source | 0.965673 |
| Encoder accepted/output | 92,489 / 92,489; 51.382643 FPS |
| Encoder backpressure/rate drops | 260 / 0 |
| Input queue peak/capacity/final | 4 / 4 / 0 |
| Output queue capacity | 3 |
| Transport seen/admitted/wire/ACK | 92,489 / 92,373 / 92,370 / 92,369 |
| Transport queue peak/final pending | 3 / 0 |
| Android frames/decode/render callbacks | 92,370 / 92,370 / 78,416 |
| Android protocol/CRC/sequence/session errors | 0 / 0 / 0 / 0 |
| Android queue-overflow/decoder errors | 0 / 0 |
| Android clean drains | 1 |
| Flight-recorder observed events/loss | 7,833,998 / 0 |
| Degradation trigger | false |

Exact upstream accounting is 96,046 source frames = 92,749 Host admissions +
346 producer replacements + 2,920 Host-queue replacements + 31 contention
drops. Encoder accounting is 92,749 inputs = 92,489 outputs + 260 explicit
bounded backpressure drops. Transport accounting separately records 110
resync-skipped, one bounded overflow, three shutdown-aborted and one final
unconfirmed item; final pending is zero. No unexplained MFT, protocol or session
error occurred.

Whole-run timing summaries in milliseconds:

| Scope | mean / p50 / p95 / p99 / max |
|---|---:|
| Host frame-bearing iteration | 13.441489 / 11.8 / 28.9 / 38.8 / 2599.4449 |
| Worker MFT `HaveOutput` event service | 4.505122 / 4.5 / 6.9 / 7.8 / 35.3408 |
| Worker `ProcessInput` | 0.482029 / 0.2 / 0.4 / 7.7 / 46.8301 |
| Worker `ProcessOutput` | 0.072662 / 0.0 / 0.1 / 0.1 / 0.4219 |
| GPU conversion wait | 4.894346 / 3.8 / 12.3 / 16.6 / 62.9781 |
| Sender consume | 0.157700 / 0.1 / 0.2 / 0.3 / 0.9983 |
| Socket write | 0.110221 / 0.1 / 0.1 / 0.2 / 0.4747 |
| ACK receive | 4.935459 / 4.5 / 7.5 / 13.2 / 50.2856 |

The isolated MFT event path did not enter the historical approximately 26 ms
median state; its sustained median was 4.5 ms. A single 2.599-second Host
iteration outlier is retained rather than hidden, but it did not cause the
rolling source/Host detector to fire or create an unbounded queue. The old
20–28 FPS sustained Host regime did not recur after the prior 26-minute failure
horizon.

Battery temperature changed from 32.8 C to 35.2 C and Android thermal status
remained 0. PSS changed from 52,979 to 65,689 KiB. Host private bytes were
114,716,672 initially, 117,133,312 finally and 118,177,792 maximum; the driver
and helper observations were stable. These observations do not prove lifetime
leak-freedom or GPU-driver internals, but show no unbounded measured growth or
thermal throttle in this run.

## Touch regression

`perf3-touch-regression-2` passed the bounded real-phone PERF3 Host regression.
The owner made one single-finger press, drag and natural release. Android
accepted and sent 100 touch messages with zero touch-queue overflow and final
active mask zero. Host validated 100 contact events and the independent Windows
target observed 118 pointer events; both reconstructed final active-contact
sets were zero. Transport recorded all 100 touch messages, Android protocol
error delta was zero, USB remained `mtp,adb`, and the concurrent 120-second
video path completed without a degradation trigger at source/Host
54.948757/54.215440 FPS, ratio 0.986654530.

The first touch attempt stopped before Host launch because the owner did not
approve the UAC prompt; it is retained privately and is not evidence of a
product failure. No touch architecture was redesigned and historical DEVICE
PHASE 2C-T remains PARTIAL while 2C-T1 remains VERIFIED.

## Verification and safety state

- PERF3 Host/fixture/simulator build: `/W4 /WX`, PASS.
- Encoder worker tests: PASS, 23 bounded queue/lifecycle checks.
- Protocol regression: PASS, 58,235 deterministic checks.
- Flight-recorder regression: PASS, 74 synthetic checks.
- Android receiver build and APK v3 signature verification: PASS.
- Windows-only disconnect/reconnect/drain/shutdown stress: PASS.
- Real short and 30-minute target performance gates: PASS.
- Touch regression: PASS; one contact, drag, natural release and final zero
  active contacts at Host, Android and independent Windows target.
- `git diff --check`: PASS; only normal LF/CRLF notices.
- Publication checker: PASS, 214 publishable working-tree files, the index and
  125 reachable history blobs checked.
- Publication-checker fixture suite: PASS, 9 scenarios.
- Direct unique-device-identifier scan: zero matches in all 214 publishable
  paths. Private PERF3 evidence is ignored and no firmware/image/archive
  extension is tracked.

USB was `mtp,adb` before and after every accepted real run. The existing ADB
server identity was preserved. No root, `su`, `adb root`, remount, SELinux,
Fastboot, flash, erase, partition, AVB, Magisk, recovery, ConfigFS, FunctionFS,
NCM, HID, UDC, Windows driver, certificate, trust-store, boot or security change
occurred. Nothing was written to Android partitions. Raw evidence remains in
the ignored private evidence tree. No commit or push was made.

## Remaining limits

The AMD driver's internal reason for the PERF2 state transition remains
UNKNOWN. One successful 30-minute run verifies this isolation gate, not a
permanent AMD MFT fix. Literal 60.000 FPS and lossless admission are not claimed;
the source supplied about 53.36 unique FPS and all downstream overload was
bounded and explicitly counted. A production interactive third-screen UI is
also outside this phase.

DEVICE PHASE 2C-PERF3 is **VERIFIED** for the architectural correction and its
sustained acceptance gate. No DEVICE PHASE 2C-1, Windows PHASE 3E, NCM, HID or
camera work has begun.
