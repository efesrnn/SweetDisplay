# DEVICE PHASE 2C-PERF2 — Degradation-triggered flight recorder

Date: 2026-09-22  
Result: **VERIFIED / TRIGGERED**

This phase captured a real recurrence of the previously verified source/Host
cadence divergence and localized the first materially changed blocking scope.
It did not implement a performance correction. Historical DEVICE PHASE
2C-PERF remains VERIFIED and DEVICE PHASE 2C-PERF1 remains PARTIAL.

## Recorder and trigger

The opt-in recorder reuses the PERF1 QPC event model. It has a preallocated
200,000-record rolling pre-trigger ring and a 350,000-record post-trigger area;
records are fixed at 48 bytes. There is no per-event allocation or disk write.
The pre-trigger ring is frozen once and the completed bounded trace is written
only during finalization. A healthy non-trigger run writes summary metadata but
no raw trace.

The detector samples monotonic source and Host counters. It triggers exactly
once when a rolling interval of approximately 10 seconds has all of these
properties:

- source rate is at least 45 FPS;
- Host/source is below 0.75;
- the condition is not a one-sample transient.

It therefore does not trigger when source and Host slow together. Synthetic
tests cover healthy/healthy, slow-source/matching-Host, one transient,
healthy-source/approximately-50%-Host, single trigger, pre-ring retention,
bounded post capture and fixed capacity. They pass 74 checks.

## Accepted real observation

`perf2-observe-8` used the normal Android receiver at 2400x1080, nominal 60 FPS,
30 Mbps, AMD hardware AVC, ADB-forwarded TCP, Qualcomm hardware decode and
Surface presentation. The requested bound was 1,800 seconds. The real trigger
occurred earlier, and the run stopped after the bounded post-trigger interval.

| Measurement | Result |
|---|---:|
| Host observation | 1,664.569967 s |
| Whole-run source | 55.595741 FPS |
| Whole-run Host | 53.878180 FPS |
| Whole-run Host/source | 0.969106249 |
| Trigger window | 10.195552 s |
| Trigger-window source | 52.179618 FPS |
| Trigger-window Host | 37.761565 FPS |
| Trigger-window Host/source | 0.723684211 |
| Post-trigger source | 3,122 frames / 51.972114 FPS |
| Post-trigger Host | 1,227 frames / 20.425940 FPS |
| Post-trigger Host/source | 0.393017297 |
| Pre-trigger coverage | 61.360385 s |
| Post-trigger coverage | 60.070676 s |
| Captured records | 278,084 |
| Observed events | 6,342,368 |
| Trace loss | 0 |
| Recorder calibration | mean 21.385 ns; p95 100 ns; max 6,500 ns |
| Final trace write | 6.812 ms |

The whole-run ratio is intentionally not used to classify the late state: more
than 26 minutes of healthy operation dilute the final minute. In the detector
window the threshold is crossed, and the post-trigger interval reproduces an
even stronger source/Host divergence.

Encoder input was 89,684; 89,675 were accepted and output, with nine explicit
backpressure drops. Hardware-only AMD encoding, GPU surface input and clean
drain remained true; no software fallback occurred. Transport sent 89,561
frames and received 89,560 ACKs, with bounded accounting for seven disconnected,
103 resync-skipped, one overflow, three aborted and one unconfirmed attempt.
There were zero socket or protocol errors and no pending frame at shutdown.

Android recorded 89,561 frames, 89,558 decoder inputs and outputs, 84,139 render
callbacks, zero protocol/CRC/sequence/session/queue-overflow/decoder errors and
one clean drain. Thermal status remained 0; battery temperature changed from
34.2 C to 35.0 C. PSS changed from 57,678 KiB to 71,404 KiB. These bounded
observations do not establish a leak or thermal throttle.

## Healthy versus degraded micro-timing

The healthy segment ends 10 seconds before the trigger, excluding the detector
transition. The degraded segment starts at the exact trigger QPC. All values
below are milliseconds and show p50 / p95 / p99 / maximum.

| Scope | Healthy | Degraded |
|---|---:|---:|
| Frame-bearing Host iteration | 7.7342 / 33.3448 / 56.3944 / 185.0200 | 48.4691 / 69.4523 / 82.6407 / 102.8562 |
| Main-loop encoder pump | 0.0032 / 7.5853 / 8.2719 / 173.4104 | 27.1339 / 34.2986 / 35.2206 / 49.0152 |
| Submit-internal encoder pump | 0.0021 / 0.0050 / 0.5031 / 26.6052 | 0.0022 / 0.0053 / 8.9351 / 27.8806 |
| MFT `HaveOutput` event service | 6.1213 / 7.4794 / 8.1968 / 172.8232 | 26.4200 / 33.7217 / 34.4915 / 48.2868 |
| GPU conversion wait | 1.1522 / 9.0519 / 24.2672 / 48.4805 | 9.8306 / 22.6629 / 31.9610 / 47.1359 |
| Encoder ProcessInput | 0.3231 / 0.5086 / 0.7301 / 17.9960 | 0.3029 / 0.4212 / 0.5010 / 1.1720 |
| Encoder output total | 0.4440 / 0.7538 / 0.9869 / 4.3286 | 0.6715 / 0.9100 / 1.0091 / 1.1691 |
| Sender consume | 0.1489 / 0.2813 / 0.3394 / 2.7934 | 0.1862 / 0.3171 / 0.3504 / 0.6199 |
| Socket write | 0.1429 / 0.2196 / 0.2631 / 0.3916 | 0.1417 / 0.2153 / 0.2627 / 0.4561 |
| ACK receive | 5.4816 / 7.2254 / 9.4461 / 115.2421 | 5.7579 / 7.3930 / 9.7144 / 20.3989 |
| Keyed mutex | 0.0532 / 0.0982 / 0.1471 / 1.2712 | 0.0467 / 0.0857 / 0.1057 / 0.1698 |
| Host frame acquire | 0.1175 / 0.2076 / 0.3357 / 26.6966 | 0.1408 / 0.2033 / 0.2417 / 0.2971 |
| Host admission checks | 0.0002 / 0.0004 / 0.0005 / 1.8655 | 0.0002 / 0.0004 / 0.0005 / 0.0008 |

Same-QPC logical-send-to-exact-ACK changed only from
5.6400/7.4024/9.9018/115.4787 to 5.9301/7.5517/9.8743/20.6050 ms. Outstanding
ACK depth remained one by policy. Socket writes, ACK processing, sender locks,
keyed mutex, frame acquire, admission checks and evidence work do not show a
frame-period-scale degraded shift.

## First changed component and ordering

The transition is sharp. Six seconds before the trigger, the frame-bearing Host
iteration still processed 52 frames in its one-second bin; its median was
9.1899 ms. In the next bin it processed 27 frames and its median rose to
33.7091 ms. In that same first slow bin:

- the main-loop encoder pump median rose from effectively zero to 20.7086 ms;
- the MFT `METransformHaveOutput` event-service median rose from 5.8314 ms in
  the preceding bin to 26.6056 ms;
- the Submit-internal pump remained at 0.0025 ms median;
- conversion-wait median remained near baseline at 1.2410 ms, although its p95
  began rising to 14.5432 ms;
- Host admissions fell from 57 in the last healthy counter interval to 31,
  then 22 per interval while source remained 46–56.

The `EncoderEventServiceHaveOutput` scope covers the synchronous
`IMFMediaEventGenerator::GetEvent(MF_EVENT_FLAG_NO_WAIT)` call and its event
status/type inspection; it ends before encoded-output processing begins.
Encoded-output processing itself remained below 1 ms at p95. Thus the first and
dominant changed blocking boundary is the AMD hardware MFT `HaveOutput` event
retrieval/status/type path executed synchronously by the Host main loop. The
later GPU conversion-wait increase is real but occurs downstream of that first
wait and is not the first missed-admission cause.

The supported causal explanation is narrow: a formerly approximately 6 ms
hardware-MFT `HaveOutput` event-service operation changes to approximately
26–34 ms on the Host admission thread, delaying the next driver fetch and
causing the source/Host divergence. The trace cannot distinguish time inside
`GetEvent` from time inside `GetStatus`/`GetType`, and it does not establish why
the AMD MFT changes state. It also does not prove an ADB cause.

The smallest evidence-supported correction to evaluate in a separately
authorized phase is to remove hardware-MFT event retrieval from the frame-
admission thread—using one controlled encoder event owner/worker and passing
completed output/tokens through a bounded queue. That proposal is not
implemented here. Before such a change, a still smaller diagnostic refinement
could time `GetEvent`, `GetStatus` and `GetType` independently.

## ADB, connection and safety state

The existing ADB server process and localhost listener were unchanged before,
at trigger, at capture completion and after cleanup. The controller did not
kill or restart ADB. USB configuration was `mtp,adb` before and after. A short
transport setup connection ended about 1,604.21 seconds before the trigger;
the second connection then remained active through the captured degradation.
That ordering excludes the recorded reconnect as the immediate trigger.

Setup runs `perf2-observe-1` through `perf2-observe-6` stopped before accepted
evidence because of controller/ADB-context, receiver-command or secure-desktop
pattern setup faults. `perf2-observe-7` remained healthy for roughly ten minutes
but hit the original bounded evidence-row cap, so it is excluded; the flight-
recorder-only cap was enlarged while remaining fixed. Every failed run is
preserved privately and was not overwritten.

No root, `su`, `adb root`, remount, SELinux, Fastboot, flash, erase, partition,
AVB, Magisk, recovery, ConfigFS, FunctionFS, NCM, HID, UDC, driver, certificate,
trust-store, Windows boot or Windows security operation occurred. Nothing was
written to Android partitions. Touch remained idle and DEVICE PHASE 2C-T1 was
not changed. No commit or push was made.

Raw traces, video, logs, topology/session data and device evidence remain under
the ignored private evidence tree. Public documentation contains no device
serial or other unique device identifier.

## Verification and repository hygiene

- Protocol build: `/W4 /WX`, PASS.
- Protocol regression: PASS, 58,235 deterministic parser/protocol/queue/resync
  checks.
- Flight-recorder synthetic build: `/W4 /WX`, PASS.
- Flight-recorder synthetic regression: PASS, 74 checks.
- PERF2 Host/fixture/simulator transport build: `/W4 /WX`, PASS.
- Triggered-trace analyzer: PASS; record count, file length, zero loss, minimum
  pre/post coverage and event identities validated.
- `git diff --check`: PASS; only normal LF/CRLF conversion notices.
- Publication checker: PASS, 210 publishable working-tree files, the index and
  125 reachable history blobs checked.
- Publication-checker fixture suite: PASS, 9 scenarios.
- Direct unique-device-identifier scan: zero matches in the 210 publishable
  paths.
- Private evidence ignore check: PASS. No firmware/image/archive extension is
  tracked.

## Conclusion

DEVICE PHASE 2C-PERF2 is **VERIFIED / TRIGGERED**. A real degraded state was
captured without a recovery action, trace loss or ADB-state change. The first
responsible blocking boundary is localized to synchronous AMD hardware-MFT
`HaveOutput` event service on the Host main/admission thread. The internal split
among `GetEvent`, `GetStatus` and `GetType`, and the state transition that makes
that operation slow, remain UNKNOWN. No correction or later phase was started.
