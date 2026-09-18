# Display pipeline — implemented handoff and remaining gates

## VERIFIED PHASE 2 path

Actual Windows desktop on SWT0001 / SweetDisplay AMOLED -> IddCx UMDF 2 ->
one GPU CopyResource -> three shared D3D11 textures -> SweetDisplayHost.
The documented IddCx device-interface IOCTL path carries validated metadata and
control only. It never transports full pixel buffers. The earlier CPU-readback
proposal was superseded by this implementation; it is not the current design.

The Host creates same-render-adapter BGRA8 resources with shared NT handles and
keyed mutexes. The driver opens the restricted named resources. Producer work
does not wait for the Host: the bounded queue replaces old ready frames, never a
held frame. The Host fetches the newest ready frame, acquires it, releases it and
acknowledges its exact epoch/ID/slot. File cleanup retires the connection.
See [FRAME_HANDOFF.md](FRAME_HANDOFF.md) for the ABI and measured crash/reconnect
and slow-consumer results. This is one GPU copy, not a zero-copy claim.

An optional diagnostic path samples only two 640-pixel rows to match a nonce and
counter rendered on the actual indirect desktop. It is not full-frame readback,
but synchronous Map can perturb timing and must be measured separately. The
single PHASE 1 BMP stays in private evidence. Earlier PHASE 3A.1 failure controls
saved bounded private crops; successful classified pilots/observation/soak saved
no images. No continuous or normal full-frame CPU readback was added.

## PHASE 3A capacity and soak

60 Hz mode is VERIFIED; sustained 60 FPS handoff is not yet established. Prior
PHASE 2 workload paint cadence was about 39.84/s. A new D3D11 test window renders
distinct counters and moving geometry on the active SWT0001 desktop, using DXGI
latency signaling and optional QPC/high-resolution timer pacing. Measurements
separate source acquisition, metadata reception, mutex acquisition and diagnostic
sample completion. No isolated GPU-copy timestamp is presently available.

The initial 60-second experiments demonstrate workload-sensitive rates above the
old 38.5 FPS result. The 30-minute-10-second run observed 54.325356 FPS, but failed
content acceptance (174 nonce mismatches and one counter regression). It does
not pass the capacity/soak gate. See
[PHASE3_VALIDATION.md](PHASE3_VALIDATION.md) and TEST_LOG for exact attempts.
The installed driver and three-slot architecture remain unchanged. The later
fresh corrected soak passed at 55.662408 Host FPS for 1810.0007551 seconds,
including resource review and fresh-process reconnect. PHASE 3A is VERIFIED;
see [PHASE3A_RESULTS.md](PHASE3A_RESULTS.md). The old soak remains UNKNOWN.

## PHASE 3B planned GPU-native encoding — NOT YET TESTED

Shared D3D11 BGRA8 -> GPU format conversion/scaling as required -> Media
Foundation hardware H.264 MFT with DXGI device manager -> encoded access units.
No full-frame GPU-to-CPU download is permitted in the normal video path.

Enumerate usable MFTs and record friendly name, CLSID, hardware status, adapter,
input/output types, resolution/rate, bitrate, profile, low-latency and GOP/IDR
settings and every rejected configuration. Hardware registration is not proof
of successful activation or actual encoding. Respect asynchronous MFT events.
[MFT enumeration](https://learn.microsoft.com/en-us/windows/win32/api/mfapi/nf-mfapi-mftenumex),
[Microsoft H.264 encoder](https://learn.microsoft.com/en-us/windows/win32/medfound/h-264-video-encoder).

Progressively test 800x360@10, 1280x576@30, 1920x864@30, 2400x1080@30 and
2400x1080@60. Measure real submit/output counts, drop accounting, latency
percentiles, encoded bytes/bitrate, IDR behavior and resources. Bounded input
queues and backpressure must preserve desktop responsiveness. Decode the output
and verify correspondence to Display 3 before declaring success.

## PHASE 3C–3E planned end-to-end path — NOT YET TESTED

The later localhost transport and PC device simulator will carry actual encoded
Display 3 frames over the same logical protocol intended for future displayd.
The simulator must decode/render changing desktop content, recover from restart
and stalled consumers, then return mouse-derived absolute touch via that protocol.
Protocol, simulator and touch implementation are gated on earlier measured results.
Phone-specific decode/display/input behavior remains UNVERIFIED.
