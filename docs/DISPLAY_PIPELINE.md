# Display pipeline decision — proposed, NOT YET TESTED

## First driver-to-host interface

Choose a private WDF device interface with overlapped buffered IOCTL requests for the first low-rate CPU frame proof. UMDF supports application device-interface I/O and framework-managed request buffers. This is a documented mechanism; compatibility with our actual IddCx derivative remains to be demonstrated. [Device interfaces](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/using-device-interfaces), [UMDF buffers](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/accessing-data-buffers-in-wdf-drivers).

Planned operations: GET_CAPS, GET_STATS, WAIT_FRAME(last seen generation/frame ID). Define a separate versioned local ABI with fixed-width fields, no pointers. Host opens the registered interface using SetupAPI/Configuration Manager and CreateFile, with restrictive access policy allowing the intended host principal. Driver validates every size/version, zeroes output padding and completes/cancels each request exactly once. Use a cancel-safe manual WDF queue, bounded outstanding requests, and remove/power-down cleanup. Resolve actual device ACL/host identity before deployment; do not expose desktop pixels to every local user.

Copy acquired D3D texture into driver-owned resources before source reuse. For initial proof, downscale to 800x360 and read back BGRA8 at up to 10 fps into a two-slot CPU queue. Honor D3D row pitch, GPU completion and format; never pass a COM surface pointer across processes. Do not block the IddCx acquisition loop on encoding, USB, host I/O or disk. Device loss/swapchain replacement retires resources and increments generation; host disconnect cannot hold the desktop pipeline hostage.

The pinned sample's `SwapChainProcessor::RunCore` uses the selected render adapter, ReleaseAndAcquireBuffer, buffer-available/termination events and FinishedProcessingFrame. Its TODO site is the future extraction point. Preserve those lifetime rules; do not assume FinishedProcessingFrame alone makes an asynchronous readback complete. [Pinned sample](https://github.com/microsoft/Windows-driver-samples/blob/67d81f217bc01edf7a4320e4911c11065635acfa/video/IndirectDisplay/IddSampleDriver/Driver.cpp).

## Alternatives

| Mechanism | Decision |
|---|---|
| Buffered WDF IOCTL | First proof: simple explicit ownership/cancellation, with CPU-copy cost |
| Shared CPU ring | Revisit if profiling warrants; requires ACL, cross-session handle transfer and synchronization design |
| Shared D3D11 textures | Later optimization; needs same-adapter/format support, NT handle transfer, ACLs, synchronization and reset handling |

No zero-copy or shared-GPU-texture support is claimed. At 2400x1080 BGRA8/60 the frame bytes alone are 622,080,000 bytes/s; the initial IOCTL/readback approach is not the final performance promise.

## Proof criteria

Move a distinguishable window on the actual indirect monitor. Host logs frame ID, generation, monotonic QPC timestamp/frequency, source and output dimensions, format, stride, queue depth and drops. Optionally dump one requested frame, not every frame. A synthetic probe is never counted as desktop capture. Measure interarrival median/p95/p99 and CPU/GPU utilization, recording GPU, mode and workload. A static desktop need not produce frames at refresh cadence.

## Encoder experiments

Enumerate H.264 hardware encoder MFTs; record friendly names/CLSID and HRESULTs before selecting one. Hardware registration is not proof of activation or throughput. Probe actual NV12/D3D11 acceptance, accepted type ranges, D3D-aware attributes and asynchronous MFT event handling. Use hardware MFT documentation rather than assuming Microsoft's software H.264 encoder properties apply identically to every vendor. [MFTEnumEx](https://learn.microsoft.com/en-us/windows/win32/api/mfapi/nf-mfapi-mftenumex), [H.264 encoder](https://learn.microsoft.com/en-us/windows/win32/medfound/h-264-video-encoder).

Request low latency through supported CODECAPI/MF properties, check each result, minimize B-frames where supported, and record output SPS/PPS, keyframe cadence, bitrate and actual timestamp behavior. Progress: synthetic 800x360@10 → real frames → 1280x576@30 → 2400x1080@30 → 60. Measure submit-to-output latency and queue depth before optimization. No real encode experiment or end-to-end latency result exists yet.

Discovery result 2026-09-13: hardware H.264 MFT registrations were returned successfully. Registration discovery does not prove activation or real encoding; raw hardware details remain in private evidence.

