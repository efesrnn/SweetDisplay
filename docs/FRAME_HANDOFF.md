# PHASE 2 — frame handoff

Architecture decision, written before implementation. Full PHASE 2 acceptance: VERIFIED.
Scope: one local administrator Host, one SweetDisplay monitor, three GPU slots.
No encoder, network, phone, or new full-frame dump is part of this phase.

## Supported boundary and alternatives

The DLL runs inside the UMDF WUDFHost process, normally under Local Service.
The swapchain thread owns a D3D11 device on IddCx's render-adapter LUID.
It cannot pass a process-local pointer or HANDLE value to another process.
[Microsoft UMDF host](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/umdf-driver-host-process).

| Approach | Decision |
|---|---|
| Shared D3D11 textures | Selected: one GPU copy from the acquired surface into a bounded pool; Host opens/owns the same GPU allocations |
| CPU shared-memory ring | Supported general IPC, but full-frame readback adds bandwidth, synchronization and future upload cost; deferred |
| Device interface and IOCTL | Selected for bounded metadata/control only; not bulk pixels |
| ETW diagnostic stream | Keep PHASE 1 evidence support; not a production frame channel |
| Process injection, private UMDF handles, custom kernel bridge | Unnecessary and excluded |

Use WdfDeviceCreateDeviceInterface and the actual IddCx callback
EvtIddCxDeviceIoControl, NOT an independently created default WDF I/O queue.
The Microsoft sample explicitly documents IddCx queue redirection; EWDK 26100
iddcx/1.6/iddcx.h declares the callback with Device, Request, output/input lengths
and control code. Buffered IOCTLs exchange fixed-width, versioned structures.
[Sample](https://github.com/microsoft/Windows-driver-samples/blob/main/video/IndirectDisplay/IddSampleDriver/Driver.cpp),
[UMDF device interface](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfdevice/nf-wdfdevice-wdfdevicecreatedeviceinterface).

## Selected GPU and security model

Host creates three D3D11 DEFAULT textures on the driver's reported render LUID,
with SHARED_NTHANDLE and SHARED_KEYEDMUTEX. Random Global resource names cross
the interactive-user / UMDF session boundary. Host passes names through the
device interface; driver uses ID3D11Device1::OpenSharedResourceByName.
No OpenProcess(WUDFHost), raw-pointer sharing, or guessed handle duplication.
The actual texture description is checked before copy.
[CreateSharedHandle](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgiresource1-createsharedhandle),
[OpenSharedResourceByName](https://learn.microsoft.com/en-us/windows/win32/api/d3d11_1/nf-d3d11_1-id3d11device1-opensharedresourcebyname).

The development device ACL permits Administrators, SYSTEM and UMDF drivers.
Host requires elevation for desktop-frame access. GPU object ACLs grant the
same principals plus Local Service as required by the UMDF token. No Everyone
or Users access, inheritable handles, persistent IPC objects or new certificates.
This trusts other administrators and UMDF components on this single machine;
per-service production isolation is future work. INF security is the documented
UMDF path; WdfDeviceInitAssignSDDLString is KMDF-only.
[Access policy](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/controlling-device-access).

## Lifetime, synchronization and bounded policy

One connection belongs to one WDF file object. Closing the last handle (including
process termination) disconnects it through EvtFileCleanup. New connections use
fresh names/resources; GPU references survive only while an in-flight copy needs
them. No pending IOCTL waits on Host consumption.
[File cleanup](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfdevice/nc-wdfdevice-evt_wdf_file_cleanup).

Slot states are free, ready and held by Host. Producer prefers a free slot,
otherwise replaces the oldest ready slot; it never overwrites a held slot.
Host takes the newest ready frame and discards older queued metadata. Every
GPU access uses IDXGIKeyedMutex. Producer AcquireSync has timeout zero; a busy
pool drops input instead of waiting. Key 0 belongs to producer, key 1 to reader;
discarded ready allocations retain key 1 until producer reclaims them.
Only S_OK means acquired: WAIT_TIMEOUT and WAIT_ABANDONED must not be treated
as success via SUCCEEDED. Abandonment invalidates the connection.
[Keyed mutex](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgikeyedmutex-acquiresync).

Control-state critical sections are bounded; GPU copies are submitted on the
swapchain thread and flushed before handing off key 1. No CPU wait for Host,
encoding or disk I/O. IddCx source reference is released and FinishedProcessingFrame
called in the existing loop. Acquired surfaces never escape into Host.
On mode/render-device change, invalidate the connection and renegotiate. Host
retries discovery/connect with bounded backoff. Driver frame IDs continue across
Host sessions; a source epoch distinguishes driver/swapchain restarts.

## Copy and measurement model

IddCx GPU surface -> one GPU CopyResource -> shared GPU slot -> Host GPU resource.
This is minimal-copy, not zero-copy. The three 2400x1080 BGRA allocations hold
31,104,000 logical pixel bytes, plus GPU allocation overhead. Metadata is tiny.
Host may copy a small pattern sample region to staging memory to validate content;
that optional diagnostic readback is not a full-frame transfer or future encoder path.
Polling and GPU availability add latency; measure QPC source-to-Host age rather
than claiming a guaranteed bound. Slow-Host drops preserve bounded storage.

Metadata includes version, source epoch, frame ID, source QPC/frequency,
dimensions, format and flags. Host records receive QPC separately. Row pitch is
not a GPU texture property; only the staging Map result supplies a CPU row pitch.
The local v1 ABI is little-endian with explicit 8-byte packing and compile-time
size checks: State 208 bytes, Connect 624 bytes, Ack 40 bytes. This is not the
future USB protocol. GET_STATE/FETCH return metadata, CONNECT passes three names,
ACK releases one exact ID/slot, and DISCONNECT/file cleanup retire the connection.
Telemetry separates acquired SourceFrames, successfully received frames,
producer drops, stale Host-queue drops, busy/invalid frames, outstanding slots,
high-water, ID gaps, intervals and content changes. Frame ID gaps caused by the
declared drop policy are not serialization errors.
Producer try_lock failures are reported as contention, separately from GPU busy
and stale-queue drops. SourceFrames includes those acquisitions. Accounting:
source = acknowledged + producer replacements + Host stale drops + GPU busy +
invalid + contention + ready + held. One Host lease can be outstanding in v1.
Frames ready at measurement end are reported as outstanding, not received/lost.

Receive age means acquisition QPC to metadata receipt, before Host's keyed-mutex
wait and optional sample Map. It is not encoder latency or total GPU-consumption
latency. Both processes use the system QPC frequency. FPS uses elapsed QPC;
intervals use differences between actually delivered source timestamps.

## Build, deployment and rollback record

VERIFIED: EWDK 26100 Debug x64 driver and Release x64 Host build; production
validation/queue checks pass. InfVerif /u, Inf2Cat and Universal API validator pass.
PHASE 1 DLL/CAT/INF signatures and active problem-0 baseline were recorded before
deployment. The existing certificate signed only the changed driver DLL/CAT.
Monitor package and trust stores were retained. SignTool /pa verifies DLL/CAT
signatures and INF/DLL catalog membership.

Rollback remains ignored at out/dev-signed/phase1-driver/; hashes, old package
names and security snapshot are in the private phase2 baseline directory. The
old package remains in Driver Store. No rollback was executed. If needed, an
explicitly reviewed rollback would remove only the new phase2 package with
pnputil /delete-driver ... /uninstall, re-add the preserved phase1 INF with
/add-driver ... /install and verify active INF, PnP and topology. Do not assume
/add-driver forces an older package rank. Retain monitor, trust and boot settings.

VERIFIED: unelevated frame-interface open denied with Win32 error 5. The initial
8.001657-second smoke test delivered 314 frames (39.241872 FPS), all nonce-matched,
zero drops, high-water 1. Smoke alone does not satisfy full PHASE 2 acceptance.

## Verified integration results — 2026-09-15

The completed acceptance-4 harness and independent evidence verifier both pass.
Every received frame was 2400x1080, DXGI format 87 (BGRA8), with valid flags,
ordered frame IDs/source QPC/receive QPC and a matching pattern nonce. Sampled
paint counters were also correlated independently against the actual SWT0001
pattern CSV; metadata alone was not treated as proof of real pixel transfer.

| Run | Seconds | Source | Received | Host FPS | Producer replacement | Host stale drop | Contention drop | Outstanding at end | High-water |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Normal | 35.000873 | 1359 | 1357 | 38.770462 | 0 | 0 | 2 | 0 | 2 |
| Clean reconnect | 35.002286 | 1352 | 1351 | 38.597480 | 0 | 0 | 1 | 0 | 2 |
| Slow Host, 100 ms delay | 12.094161 | 469 | 115 | 9.508721 | 123 | 228 | 0 | 3 ready | 3 |
| Reconnect after Host crash | 12.003027 | 461 | 461 | 38.406978 | 0 | 0 | 0 | 0 | 2 |

Invalid and GPU-busy counts were zero in all completed runs. Slow-run accounting
is 469 = 115 received + 123 replaced + 228 stale-dropped + 3 still ready. The
three outstanding frames are retired at disconnect. They are not counted as
received, and a bounded end-of-run snapshot is not a queue leak.

| Measurement | Normal | Clean reconnect |
|---|---:|---:|
| Delivered source interval average | 25.786398 ms | 25.903817 ms |
| Delivered source interval min / max | 8.467900 / 62.117700 ms | 7.800500 / 62.347500 ms |
| Metadata receive age average / max | 1.509300 / 5.191900 ms | 1.495693 / 6.899200 ms |
| Sample content changes | 1335 | 1316 |
| Received ID gaps, explained by contention | 2 | 1 |

VERIFIED: with no streaming Host, source advanced 155 frames in four seconds.
The crash test started a separate Host, observed its GPU-lease-held marker and
terminated only that process (expected termination exit 0xFFFFFFFF). WDF cleanup
removed the connection: the following four-second absent-Host check saw 158
new source frames and connected=0. A fresh Host then received 461 valid frames.
The driver source epoch stayed unchanged and IDs continued increasing across
all sessions. No reboot or package reload was needed.

VERIFIED: both device classes remained OK / PnP problem 0; SWT0001 stayed active
at 2400x1080@60. Secure Boot and HVCI remained ON; TESTSIGNING remained OFF and
normal code-integrity enforcement ON. Boot time was unchanged. The acceptance
interval contained zero matching WUDFHost/SweetDisplay crash or selected system
driver-failure/bugcheck events. No new certificate/store/security change occurred.

## Rate interpretation and limits

VERIFIED: the pattern generated approximately 39.83 / 39.84 paint events per
second within the two measured source windows. Acquisition averaged 38.83 /
38.63 FPS; Host delivery was 38.77 / 38.60 FPS, retaining 99.85% / 99.93% of
acquired frames. Sample changes were approximately 38.18 / 37.63 per second.
The 60 Hz display mode is distinct from each of those measured rates.

SUSPECTED: this timer/DwmFlush test pattern and desktop scheduling limit the
changed-content rate in these runs. The handoff is not a demonstrated major
bottleneck; the observed added loss is the explicitly counted metadata-lock
contention. NOT YET TESTED: sustained 60 FPS source/Host capacity, alternate
GPUs, long soak, GPU reset and production least-privilege multi-user isolation.
The optional sample readback also contributes overhead; its standalone cost
has not been isolated. No encoder/USB/simulator/phone capability is inferred.

## Reproduction and evidence

PHASE 2 implementation inventory:

- windows/driver/SweetDisplayDriver/FrameHandoff.h: GPU pool, device/file ownership,
  IddCx control operations and nonblocking producer policy.
- windows/driver/SweetDisplayDriver/Driver.cpp and Driver.h: swapchain hook,
  per-device handoff lifetime, device interface and file cleanup callback.
- SweetDisplayDriver.inf and SweetDisplayDriver.vcxproj in that directory:
  administrator/UMDF access policy and header project entries.
- windows/shared/FrameHandoffProtocol.h: versioned local metadata ABI, validation,
  timestamp conversion and the shared bounded queue policy.
- windows/host/SweetDisplayHost.cpp, SweetDisplayHost.vcxproj and README.md:
  discovery, shared textures, consumption, sample validation and telemetry.
- windows/host/tests/FrameHandoffTests.cpp and FrameHandoffTests.vcxproj:
  production metadata/queue checks.
- scripts/windows/Build-FrameHandoff.cmd, Test-FrameHandoff.ps1 and
  Verify-FrameHandoffEvidence.ps1: build and separate real-device/evidence tests.
- docs/FRAME_HANDOFF.md, docs/TEST_LOG.md and STATUS.md: decisions and results;
  LICENSE.md/NOTICE.md: attribution for sample-derived project scaffolding.

PHASE 1 diagnostic sources/evidence are retained. Their existing uncommitted
changes are not new PHASE 2 work.

Build-FrameHandoff.cmd builds Host, 2,033 shared production metadata/queue checks
and the driver; its host-only option builds just Host without touching the DLL
or package. Test-FrameHandoff.ps1 runs the actual source integration scenarios;
Verify-FrameHandoffEvidence.ps1 independently checks metadata, QPC, pattern
correlation, drops, capacity and reconnect. See windows/host/README.md.

The final Host-only rebuild added contention and total-drop fields to the
one-second console summary; those counters were already measured in the JSON
reports. Frame-transfer behavior was unchanged. Earlier test-runner failures
are retained in TEST_LOG.md; they are not used as completed acceptance runs.

Raw evidence is ignored under docs/evidence/private/phase2/acceptance-4/: execution,
verification, frame CSVs, pattern log, before/after health/topology, failure-event
query and rate-observation files. Build/signature logs and rollback hashes are
in the parent private directory. PHASE 1's single BMP remains the only BMP.
Public file/index/history hygiene passes. PHASE 2 is complete; later phases
require a separate instruction.
# PHASE 3A follow-up (2026-09-16)

The PHASE 2 driver/ABI/three-texture architecture below remains unchanged.
Current staged DLL/INF/CAT and shared ABI hashes match the saved PHASE 2 baseline.
Host-only instrumentation adds presentation ID, resource-acquisition/sample-complete
QPC, mutex wait and one-second queue/source/drop telemetry. The test duration limit
is now 86,400 seconds; no normal full-frame CPU readback was added.

PHASE 3A is now VERIFIED by the fresh corrected soak; see PHASE3A_RESULTS.md.
Short sampled GPU-pattern runs reached 55.604194 FPS, exceeding
the earlier workload's ~38.5 FPS. No-Host/metadata controls locate an upstream
target-presentation cadence shortfall; this is not proof of the GPU handoff's
maximum capacity. The historical 1,810-second run received 98,329 frames (54.325356 FPS), but
fails content acceptance with 174 nonce mismatches and one counter regression.
It is not a VERIFIED soak and is not evidence for sustained 60 FPS. The runner
stopped before its reconnect phase. See PHASE3_VALIDATION.md and TEST_LOG.md for
historical metrics and diagnosis. The new 1810.0007551-second soak received
100,749 frames (55.662408 FPS), A only, with exact drops, bounded resources,
clean shutdown and fresh-process reconnect. The old cause remains UNKNOWN;
PHASE 3B has not started.
