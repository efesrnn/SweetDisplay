# DEVICE PHASE 2C-PERF1 results — PARTIAL

DEVICE PHASE 2C-PERF1 is **PARTIAL**. The bounded QPC instrumentation, real
Android ACK-only control, socket/ACK measurements, lock timing and evidence A/B
all completed. The previously VERIFIED 26–28 FPS state did not reproduce in any
accepted run, including an uninstrumented run through the earlier 2C-PERF Host
binary. Consequently the current fast state is well characterized, but the
first blocking component in the historical degraded state is still UNKNOWN.
No performance correction was implemented.

The historical results are unchanged: DEVICE PHASE 2C-T remains PARTIAL,
DEVICE PHASE 2C-T1 remains VERIFIED and DEVICE PHASE 2C-PERF remains VERIFIED
as a diagnostic localization.

## Method and bounded evidence

The PERF1 Host writes fixed-size 48-byte records to a preallocated 750,000-entry
private buffer. Each record contains QPC start, QPC duration, frame/session
context, event, Windows thread ID and two event-specific values. The buffer is
written once at shutdown; it does not emit an unbounded per-frame text trace.
The three accepted runs recorded 506,212–544,526 entries with zero dropped
records. The startup calibration measured a 20.060–20.745 ns mean and 100 ns
p95 for the atomic slot reservation and record operation. This is an estimate of
the record primitive, not a claim that every instrumented scope is free.

An uninstrumented control using the earlier `transport-host-content-v2` binary
reached source 56.391191 and Host 56.382858 FPS. The instrumented normal run was
0.126388 FPS lower; run-to-run source cadence also differed. Instrumentation did
not hide or create a 26–28 FPS regime.

All accepted runs used 2400x1080, nominal 60 FPS, 30 Mbps, the AMD hardware AVC
encoder, the existing TCP protocol and `adb forward`. USB was `mtp,adb` before
and after every accepted run.

| Run | Android path | Source FPS | Host FPS | Encoder FPS | Sender FPS | Trace loss |
|---|---|---:|---:|---:|---:|---:|
| `perf1-normal-full-4` | Qualcomm decode + Surface, full evidence flush | 56.298136 | 56.256470 | 56.256470 | 56.169946 | 0 |
| `perf1-normal-reduced-1` | Qualcomm decode + Surface, buffered evidence flush | 56.365343 | 56.290345 | 56.290345 | 56.203846 | 0 |
| `perf1-ack-only-1` | parser/CRC/session/sequence/ACK; no decoder or Surface | 56.323793 | 56.290461 | 56.290461 | 56.215967 | 0 |
| `perf1-legacy-host-control-1` | normal Android, no PERF1 trace | 56.391191 | 56.382858 | 56.382858 | 56.301573 | n/a |

The accepted normal runs reused the already-warm normal receiver process and
compared Android counters by before/after delta. Each added one clean drain and
zero protocol, CRC, sequence, session, queue-overflow or decoder-error delta.
The ACK-only run started a fresh process and added 6,755 validated/ACKed frames,
one clean drain, zero decoder inputs/outputs and zero error delta.

## Actual thread and wait ownership

The graph below was checked against `SweetDisplayHost.cpp`,
`HardwareEncoder.h`, `TransportSender.h`, `TcpStream.h`, `Receiver.java` and
`DecoderWorker.java`.

```text
IddCx/UMDF producer
  -> shared queue + keyed GPU mutex
  -> Host main thread
       FetchIo -> AcquireSync -> sample/classify -> Encoder::Submit
       Encoder::Pump -> Encoder::Output -> Sender::Consume
       Sender::Consume: transport mutex -> CRC/copy -> bounded queue -> notify
  -> transport worker
       condition variable -> transport mutex/pop
       nonblocking select/send -> evidence -> transport mutex/account
       nonblocking select/receive exact ACK -> transport mutex/account
  -> adb forward -> Android listener
       read/parse/CRC/session/sequence -> decoder queue (normal only) -> ACK
  -> Android decoder worker (normal only)
       MediaCodec -> Surface
```

The Host main thread owns frame acquisition, keyed-mutex acquisition, sampling,
encoder submission and all MFT event/output service. `Encoder::Output` invokes
`Sender::Consume` synchronously on that same thread. It never performs socket I/O
or waits for an ACK, but it briefly acquires the transport mutex to construct and
enqueue the encoded frame.

The transport worker alone performs socket send, receive and ACK parsing. It
waits on a condition variable when the queue is empty; that wait releases the
mutex. Socket send/receive occurs without the transport mutex held. The worker
briefly reacquires the mutex to record sent/ACKed state. The protocol permits one
outstanding FRAME in this implementation: send is followed by its exact ACK
before the worker pops another frame. ACK receipt does not wake the Host frame
pump directly.

The Android listener owns socket parsing and ACK generation. Normal mode enqueues
validated frames to a separate MediaCodec worker before ACK. ACK-only mode uses
the identical socket, framing, payload consumption, CRC, session, sequence and
telemetry ACK semantics, but does not construct a decoder or Surface.

## Normal full-evidence QPC distributions

Durations are milliseconds. Counts include idle-poll operations where the named
stage is part of the polling loop. Nested scopes overlap and must not be summed.

| Stage | Count | Mean | p50 | p95 | p99 | Max |
|---|---:|---:|---:|---:|---:|---:|
| Host frame acquire | 24,608 | 0.0935 | 0.0875 | 0.1451 | 0.1922 | 0.4020 |
| Host admission checks | 24,608 | 0.0003 | 0.0001 | 0.0003 | 0.0004 | 2.4257 |
| Shared keyed-mutex wait | 6,751 | 0.0523 | 0.0471 | 0.0840 | 0.1101 | 0.3956 |
| GPU conversion submission | 6,751 | 0.1673 | 0.1571 | 0.2534 | 0.2970 | 0.5409 |
| GPU conversion completion wait | 6,751 | 1.1546 | 1.0826 | 1.4323 | 2.8745 | 10.7123 |
| Encoder `ProcessInput` | 6,751 | 0.2391 | 0.2315 | 0.3193 | 0.4123 | 9.8571 |
| Encoder event/output pump | 31,360 | 1.3794 | 0.0018 | 7.3119 | 8.0754 | 15.1990 |
| Encoder output service | 6,751 | 0.4241 | 0.3680 | 0.7001 | 0.8165 | 3.7500 |
| FRAME construction/CRC/copy | 6,751 | 0.1525 | 0.1184 | 0.2774 | 0.2887 | 0.4315 |
| Transport queue admission | 6,751 | 0.0057 | 0.0051 | 0.0107 | 0.0123 | 0.0708 |
| Logical FRAME send | 6,751 | 0.1426 | 0.1335 | 0.2147 | 0.2490 | 1.0433 |
| Socket write call | 6,873 | 0.1090 | 0.1025 | 0.1661 | 0.1943 | 0.4120 |
| Transport-record flush | 20,253 | 0.0090 | 0.0073 | 0.0144 | 0.0227 | 3.0287 |
| Protocol-ledger flush | 13,746 | 0.0200 | 0.0162 | 0.0305 | 0.0391 | 3.9640 |
| ACK receive | 6,751 | 4.9568 | 4.5476 | 7.4396 | 14.2898 | 34.3556 |
| ACK validation/accounting | 6,751 | 0.0001 | 0.0001 | 0.0002 | 0.0004 | 0.0007 |
| Sample/classification work | 6,751 | 2.0036 | 1.9438 | 4.1927 | 5.3050 | 17.2895 |
| Host frame CSV evidence | 6,751 | 0.0093 | 0.0064 | 0.0163 | 0.0508 | 0.2174 |
| Transport evidence total | 20,253 | 0.0115 | 0.0095 | 0.0188 | 0.0284 | 3.0344 |
| Return/poll delay | 24,608 | 1.8951 | 2.3724 | 3.0982 | 3.3812 | 3.8350 |
| Whole Host iteration | 24,608 | 4.8765 | 3.4191 | 10.2635 | 11.1135 | 34.0886 |

Frame-bearing Host iterations had mean/p50/p95/p99/max
4.2654/4.0485/7.6559/11.0534/34.0886 ms. Idle iterations include the intended
bounded polling sleep and had mean/p50/p95 5.1075/3.1527/10.4109 ms. Current
Host admission therefore did not contain a recurring 27–30 ms blocking stage.

## Locks, socket and ACK

Normal full-evidence transport-mutex waits were negligible:

- Host `Sender::Consume` wait: mean 0.0003 ms, p95 0.0005 ms, p99
  0.0007 ms, max 0.0148 ms; zero definitely contended acquisitions.
- Worker sent-state wait: mean 0.0001 ms, max 0.0424 ms; zero definitely
  contended acquisitions.
- Worker ACK-state wait: mean 0.0002 ms, max 0.1173 ms; four contended
  acquisitions totalling 0.3802 ms.
- Keyed-mutex acquisition had no retry-marked contention; max was 0.3956 ms.

The Windows socket was nonblocking with requested 65,536-byte send and receive
buffers. It made 6,873 writes, requesting and immediately writing all
422,743,612 bytes. There were zero partial writes, zero `WSAEWOULDBLOCK` retries
and zero write-select timeouts. A logical FRAME write had p50/p95/p99/max
0.1335/0.2147/0.2490/1.0433 ms. Socket send cannot occupy the Host/pump thread;
it runs on the transport worker, and measured lock coupling back to the Host was
too small to explain a frame-period loss.

Normal same-QPC FRAME-send-to-ACK mean/p50/p95/p99/max was
5.1094/4.7020/7.5748/14.4585/34.5059 ms. ACK-only reduced this to
3.9800/3.8531/4.5458/6.7364/29.1824 ms. Peak outstanding ACK depth was one by
protocol-worker ownership, and transport queue peaks were two normal and one
ACK-only. ACK intervals followed source admission intervals around the measured
56 FPS cadence; they were not phase-locked at 25–30 FPS.

## ACK-only and evidence controls

Normal and ACK-only Host rates differed by only 0.033991 FPS. Removing
MediaCodec and Surface therefore did not expose a hidden current transport
bottleneck. Because neither mode reproduced the historical slowdown, this does
**not** prove whether MediaCodec/Surface were required for that historical
state.

Buffering per-record transport/protocol evidence flushes changed normal Host
rate from 56.256470 to 56.290345 FPS (+0.033875 FPS, about 0.060%). It produced
no material cadence change and preserved all correctness counters. Current
evidence flushes are not the responsible 27–30 ms loss.

## Excluded attempts and interpretation

Two setup attempts stopped before Host/pattern launch: the elevated controller
could not communicate with the pre-existing non-elevated ADB server, then a
PowerShell `-d` argument-binding error left `adb logcat` streaming until the new
15-second deadline terminated it. A third real run reached source/Host
56.299418/56.299418 FPS, but a cold decoder-start queue overflow caused a
reconnect and its 500,000-record trace dropped 40,935 tail records. It is retained
privately but excluded from acceptance.

The host ADB server was restarted to recover the elevation-bound ADB hang before
the accepted runs. This is the only observed host-state discontinuity adjacent
to the change from the earlier 26–28 FPS evidence to the current 56 FPS runs.
That correlation is not sufficient to call the ADB server the cause: no degraded
micro-trace exists from before the restart, and the old state cannot be restored
deterministically. The older uninstrumented binary also remained fast after the
restart.

The strongest supported conclusion is therefore:

1. In the current state, Host admission is not blocked by ACK receive, socket
   send, transport locks, keyed-mutex waits or evidence flushes.
2. Normal decode/Surface and ACK-only paths both sustain the source cadence.
3. The previously VERIFIED slowdown is state-dependent, but its exact first
   blocking component remains UNKNOWN.

No production correction is justified from a non-reproducing state. The
smallest future step is a separately authorized degradation-triggered capture:
arm this bounded trace before changing or restarting ADB/receiver state, freeze
it only when a rolling Host/source ratio falls below the existing threshold, and
then compare the exact degraded wait graph with these controls. Only that trace
can justify a narrow correction; no speculative mutex, queue, codec, socket or
ADB workaround should be shipped now.

## Safety and publication

The installed receiver update used ordinary authorized `adb install -r`; it has
only the normal `INTERNET` permission. No root, `su`, remount, SELinux, Fastboot,
flash, partition, AVB, ConfigFS, FunctionFS, NCM, HID, UDC, driver, certificate,
trust-store or Windows boot/security operation occurred. The SweetDisplay driver
was not modified. Raw timing, H.264, APK, process/session and machine evidence
remains ignored/private. No commit or push was performed.
