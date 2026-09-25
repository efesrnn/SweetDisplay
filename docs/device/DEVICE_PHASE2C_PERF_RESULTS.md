# DEVICE PHASE 2C-PERF — sustained cadence root-cause analysis

**Status: VERIFIED.** The degradation is reproducible and is localized to the
boundary between IddCx source availability and Host capture/admission when the
real ADB-forwarded Android target is active. This is a diagnostic result, not a
performance-fix or sustained-60-FPS claim.

## Scope and method

The tested target stayed fixed at 2400x1080, nominal 60 FPS and 30 Mbps H.264.
The existing AMD hardware encoder, Qualcomm hardware AVC decoder, ADB-forwarded
TCP topology, queue capacities, display mode and `mtp,adb` USB composition were
retained. Touch was idle. No queue, codec, bitrate, driver, decoder, protocol or
backpressure policy was changed.

The Windows controller sampled monotonically increasing stage and resource
counters at approximately one-second intervals. Android interval rates use its
own `elapsedRealtime` counter and are aligned only by the observed session-ready
epoch. Windows QPC is never subtracted from an Android clock.

Two detectors are retained:

- within-stage drift: a ten-second rolling rate below 80% of that stage's
  seconds-15..75 median for 15 consecutive samples;
- adjacent-stage divergence: a ten-second rolling downstream/upstream ratio
  below 80% for 15 consecutive samples.

The second detector is necessary here because both real-target runs entered the
slow state before the within-stage reference window. The original derived
timelines and the corrected adjacent-boundary summaries remain private with the
raw run evidence.

## Actual-target baseline

`perf-baseline-1` completed 900.0397067 seconds without operator abort:

- IddCx source: 49,374 frames, 54.857580 FPS.
- Host capture/admission: 25,212 frames, 28.012097 FPS.
- Exact source accounting: 381 producer drops, 23,777 Host stale/queue drops and
  three contention drops; shared queue high-water three.
- GPU conversion / AMD encoder: 25,212 inputs, 25,206 accepted and 25,206
  outputs; six explicit pressure drops; 28.005431 output FPS.
- Conversion average/peak: 3.222518 / 67.948700 ms.
- Encoder latency average/p50/p95/p99/peak: 27.314055 / 28.4 / 34.1 / 35.5 /
  49.6369 ms.
- Transport: 25,206 admitted = wire-complete = ACKed; queue peak one; no
  overflow, expiry, socket error, protocol error or pending frame.
- Host transport enqueue-to-send-start mean/p95/max: 0.109 / 0.066 / 21.200 ms.
  Send-start-to-ACK mean/p50/p95/p99/max: 5.999 / 6.144 / 7.588 / 9.677 /
  43.025 ms.
- Android: 25,206 decoder inputs = 25,206 outputs; 25,046 render callbacks at
  final drain; no CRC, sequence, session, protocol, decoder or queue-overflow
  error. Periodic queue depth was normally zero and at most one near the end;
  session queue peak was four.
- Hardware-only encoding and Qualcomm hardware decoding remained active; there
  was no software fallback. Content classification was A25,212/B0/C0/D0/E0.
- USB was `mtp,adb` before and after; Host, pattern and receiver drain exited
  cleanly.

### First transition

The first seven one-second bins were the only useful pre-transition interval.
The observed transition began at second 7: source remained approximately 56 FPS
while Host admission fell to 41 FPS and then approximately 29–30 FPS. The
ten-second adjacent-stage ratio first fell below 80% at the window ending second
11 and satisfied the required 15 consecutive windows at second 25.

| Window | IddCx source | Host / conversion / encode / wire | Android receive / decode / render | Encoder latency |
|---|---:|---:|---:|---:|
| seconds 0–6 | 53.81 FPS | 52.57–52.86 FPS | startup-aligned; not a steady comparison | 7.08 ms |
| seconds 7–14 | 52.70 FPS | 30.25–30.50 FPS | 31.26–32.82 FPS while alignment catches up | 25.89 ms |
| seconds 15–75 | 54.93 FPS | 28.90–28.92 FPS | 28.75–28.92 FPS | 27.47 ms |
| seconds 300–359 | 54.21 FPS | 28.78–28.80 FPS | 28.80 FPS | 27.88 ms |
| seconds 840–899 | 54.55 FPS | 24.83 FPS | 24.89–24.91 FPS | 28.00 ms |

No later stage develops a separate material divergence: conversion, encoder
submission/output, sender admission, wire completion, ACK, Android receive,
MediaCodec input/output and render all follow the already-reduced Host rate.

## Reproduction and controls

`perf-android-repeat-1` repeated the real target for 120.0199471 seconds under
the explicit short-control mode. It reproduced the result:

- source 53.924370 FPS versus Host/encoder 26.228973 FPS;
- the observed transition began at second 3;
- the ten-second ratio window ending second 9 was below 80% and the 15-window
  criterion was confirmed at second 23;
- encoder latency average/p50/p95 was 27.154860 / 28.4 / 34.8 ms;
- 3,148 encoded = admitted = wire-complete = ACKed, transport queue peak two,
  zero transport/protocol/decoder errors;
- send-start-to-ACK mean/p95 was 6.080 / 7.765 ms;
- Android decoder input/output was 3,148/3,148 with 3,111 final render callbacks;
  thermal status remained zero and USB remained `mtp,adb`.

The one-variable controls used the same source geometry and hardware encoder:

| Run | Path | Source | Host | Encoder output | Encoder latency | Transport output |
|---|---|---:|---:|---:|---:|---:|
| `perf-source-control-1` | source/capture only | 54.281420 | 54.123116 | n/a | n/a | n/a |
| `perf-encode-control-1` | source + GPU conversion + AMD encode | 54.050957 | 50.611108 | 50.381118 | 7.236316 ms | n/a |
| `perf-local-transport-1` | same Host + local protocol receiver | 54.923986 | 51.807377 | 51.515716 | 7.392955 ms | 51.449944 FPS |
| `perf-baseline-1` | same Host + ADB/Android target | 54.857580 | 28.012097 | 28.005431 | 27.314055 ms | 27.999339 FPS |
| `perf-android-repeat-1` | repeated ADB/Android target | 53.924370 | 26.228973 | 26.228973 | 27.154860 ms | 26.183632 FPS |

The local receiver completed 6,182 admitted = sent = ACKed frames with queue
peak one and send-start-to-ACK mean/p95 0.521/0.776 ms. The encoder-only and
local-transport controls therefore establish that source capture, GPU conversion,
AMD hardware encoding and the local SweetDisplay transport implementation can
sustain approximately 50–54 FPS in the observed system state. Replacing the
local receiver with the real ADB-forwarded Android target reproducibly changes
the Host service cadence to approximately 26–28 FPS.

## Resource and thermal observations

In the 15-minute run, Host private bytes changed from 38,883,328 at the first
sample to 83,480,576 at the last, peaking at 86,306,816. Handles changed from
1,023 to 1,269, peaking at 1,279. Threads changed from 73 to 39. Most private-byte
and handle growth occurred during initialization and had already plateaued before
the sustained slow interval; the final five-minute and final-minute values were
flat. The 120-second reproduction showed the same startup shape. This is bounded
startup allocation evidence, not proof of a leak, and it does not correlate with
the later fall from approximately 29 to 25 FPS.

During seconds 15–75 of the long run, Host CPU averaged about 15.15% of one core,
the summed sampled Host GPU-engine counters about 12.92%, and Android process CPU
about 29.12% of one core. During seconds 840–899 they were about 15.47%, 10.62%
and 25.31%, respectively. No CPU or sampled GPU saturation was observed.

Android PSS changed from 57,921 KiB at the first sample to 68,454 KiB at the last,
with a 98,662 KiB startup peak. Public Android thermal status remained zero for
the whole measured interval; battery temperature ranged from 33.3 to 34.2 C and
refresh rate remained approximately 120 Hz. These facts do not support thermal
throttling as the cause.

## Localization, inference and remaining unknowns

Established facts:

- The first measured cadence divergence is **IddCx source availability -> Host
  capture/admission**. It occurs before conversion, encoder output, transport ACK
  or Android decode/render counters.
- The divergence is reproduced in two real-target runs and is absent in the
  source-only, encode-only and local-receiver controls.
- Once a frame is admitted by Host, every downstream stage preserves essentially
  the same reduced cadence with bounded queues and exact accounting.
- Android receive, MediaCodec input/output and render do not introduce an
  additional sustained loss. Public thermal evidence remains normal.

Supported inference:

- Engagement with the real ADB/Android target changes the rate at which the
  single Host service loop returns to source fetch/admission. The narrowest proven
  causal boundary is therefore the real-target interaction across the Host
  transport endpoint, not the IddCx producer, standalone AMD encoder, local
  transport receiver or post-receive Android decode/render stages.

Still unknown:

- Whether the immediate mechanism is ADB-forwarded socket activity on Windows,
  receiver/ACK scheduling on Android, mutex/evidence work in the Host transport
  sink, or encoder-event polling delay caused by that interaction.
- The encoder latency counter includes the time until the Host pumps and records
  output; its rise from about 7.3 to 27.2 ms does not alone prove that the AMD
  hardware encoder itself slowed.
- The operator did not provide a separate visual-stutter timestamp for the long
  run; cadence classification therefore rests only on counters.
- No leak-free claim, end-to-end photon latency claim or sustained-60-FPS claim
  is made.

## Proposed next phase — not implemented

A separately authorized correction phase should first add bounded micro-timing
around `Encoder::Pump`, `Encoder::Output`, `EncodedSink::Consume`, transport
mutex acquisition, evidence flush, socket send and ACK receive. A one-variable
Android ACK-only control using the same ADB forward should then split ADB/Host
interaction from MediaCodec/render activity. If sink-side work is confirmed on
the capture/encoder service thread, the low-risk correction candidate is to move
CRC/copy/evidence work behind the existing bounded transport worker ownership,
without increasing queue capacity, removing backpressure or changing protocol
semantics. No part of that correction was implemented here.

## Safety and closure

The Android change added public, read-only process/thermal/battery/display metrics
and one-second periodic reporting. The Windows changes only extended bounded
diagnostic duration/evidence capacity and added collection/analysis scripts.
Touch lifecycle code was not modified, touch stayed idle, and no touch smoke was
required; DEVICE PHASE 2C-T remains historically PARTIAL and DEVICE PHASE 2C-T1
remains VERIFIED.

No Fastboot, flash, erase, format, root, `su`, `adb root`, remount, Magisk,
recovery, AVB, partition, SELinux, ConfigFS, FunctionFS, NCM, HID, UDC, driver,
certificate or Windows-security operation occurred. No fix, DEVICE PHASE 2C-1
or Windows PHASE 3E was started. Nothing was committed or pushed.

Raw telemetry, compressed streams, run logs, runtime topology values and build
outputs remain under ignored/private paths. This document contains no Android
serial, IMEI, MAC address, session identifier, topology token or other unique
device identifier.
