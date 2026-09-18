# PHASE 3 — PC-only validation

PHASE 2 and PHASE 3A are VERIFIED. The fresh corrected 1810.0007551-second soak,
fresh-process reconnect and resource/health review pass; see PHASE3A_RESULTS.md.
The historical failed soak described below remains UNKNOWN. New A–E pilots and
the five-minute observation are also VERIFIED; no historical interval is relabelled.
3B–3E implementation is NOT YET TESTED; 3F is PARTIAL for 3A evidence checks only.
The accepted path stays IddCx -> one GPU copy -> three shared D3D11 textures ->
Host. No phone, boot/security change, real USB gadget or camera work is authorized.
Known-good Host sources/binary and signed driver package were copied and hashed
under ignored docs/evidence/private/phase3/baseline before new changes.

## 3A method and gates

The previous pattern used WM_TIMER(16), GDI redraw and DwmFlush; its observed
paint cadence was about 39.84/s. That is evidence about this workload, not proof
of a 38.5 FPS pipeline ceiling. Use a separate D3D11 flip-model test window
targeted by active SWT0001 identity, at its 2400x1080/60 Hz desktop position.
Each Present renders a new binary counter and moving/color-changing geometry.
Log actual render/Present QPC and counter values; correlate them with the Host's
small diagnostic sample. Never pad the count with repeated images.

Pace rendering with a DXGI frame-latency waitable swap chain, maximum latency 1,
plus an optional QPC deadline and high-resolution waitable timer. CLI experiments
select render limit (0 = only DXGI pacing) and Present sync interval 0 or 1.
This changes the workload only. Microsoft documents
waiting before rendering, including the first frame; it avoids relying on the
old window timer as a 60 Hz clock.
[Microsoft DXGI pacing](https://learn.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains).
[High-resolution timer](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createwaitabletimerexw).

In this machine's multi-monitor setup, Present(1,0) plus the latency event alone
produced about 133 renders/s, not 60. The explicit QPC-limited experiment produced
60.000 renders/s. Every render updates the counter; repeated desktop content is
counted separately from distinct received pattern counters. Source production
above 60 renders/s is a workload experiment, never artificial duplication at Host.

Keep the installed driver and local ABI unchanged initially. Its source counter
counts successful swapchain acquisitions reaching the handoff hook. This is not
an independent measurement of compositor arrival before acquisition. Log existing
IddCx presentation numbers, source IDs/QPC, Host metadata-receive QPC and successful
keyed-mutex acquisition QPC separately. Resource acquisition proves consumer access;
the elapsed wait is not isolated GPU-copy execution time. Do not label it as such.
[Keyed-mutex contract](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgikeyedmutex-acquiresync).

Capacity report: unique source/received/pattern frames, rates, all drop categories,
queue peak, interval p50/p95/p99/max and acquisition wait. Record process CPU,
private/working-set bytes and handles; GPU engine counters where available.
Once cadence is understood, run >=1,800 seconds at the verified achievable rate,
with periodic health/queue/resource sampling and clean shutdown/reconnect checks.
Raw evidence remains private. Report warm-up separately from any continuing growth.

If sustained 60 FPS fails, explain the measured limiting stage before changing
architecture; stop rather than guessing at a redesign. Do not pass 3A while rate
behavior remains unexplained. No deliberate GPU reset or driver redeployment is
part of the initial measurement. The owner's subsequent corrected acceptance
allows the measured achievable rate without exactly 60 unique FPS. The limiting
stage is localized upstream; the precise scheduling cause remains SUSPECTED.

## Initial capacity experiments — VERIFIED measurements, PARTIAL gate

All successful rows below passed independent CSV/JSON verification: actual
2400x1080 BGRA8, ordered IDs/QPC, exact drop/outstanding accounting, bounded queue,
clean reconnect, PnP problem 0, unchanged security and no selected failure events.
Sampled rows also match every received nonce/counter to the real SWT0001 pattern.
An unsampled row is a timing control, not standalone pixel-content proof.

| Workload | Main duration | Source FPS | Host FPS | Distinct content FPS | Stale drops | Peak |
|---|---:|---:|---:|---:|---:|---:|
| DXGI-paced, sync 1, sampled | 60.013459 s | 56.553981 | 55.604194 | 55.587531 | 57 | 3 |
| QPC 60, sync 1, sampled | 60.004936 s | 48.246031 | 46.796150 | 46.796150 | 87 | 3 |
| QPC 60, sync 0, sampled | 30.007615 s | 53.019875 | 50.287236 | 50.220586 | 81 | 2 |
| QPC 120, sync 0, sampled | 30.005215 s | 56.423525 | 53.957287 | 53.957287 | 73 | 3 |
| QPC 120, sync 0, no sampling | 30.003613 s | 55.459988 | 53.393570 | NOT YET TESTED | 61 | 2 |

Use the final TEST_LOG table for exact values; do not infer a throughput ceiling
from short-run variation. The first, incomplete tool attempt failed before any
measurement at GetBuffer(1), DXGI_ERROR_INVALID_CALL (0x887A0001). D3D11's buffer
zero view is now used. No driver change was needed.

No-Host controls (no shared textures/copy/consumer) measured 54.599296, 56.530831
and 55.397324 acquisitions/s over 15 seconds. Removing the diagnostic sample did
not make this workload reach 60 FPS. Queue loss is a separate smaller reduction.

Before soak, metadata-only ETW measured 844 acquisitions, 56.202034 FPS across
14.999457 seconds, zero lost ETW events/buffers and zero presentation-number gaps.
The target presentation-time interval averaged 17.793594 ms, minimum 16.6666,
maximum 50.0; acquisitions averaged 6.747976 ms before the target time (worst
late acquisition 0.3565 ms). The supplied target times themselves skip refresh
intervals. This locates the major shortfall upstream of shared-texture handoff;
the precise desktop scheduling cause remains SUSPECTED, not proven.
PresentDisplayQPCTime is a target display time, not an arrival timestamp:
[Microsoft IDDCX_METADATA](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/iddcx/ns-iddcx-iddcx_metadata).

## Historical soak attempt — failed original oracle, cause UNKNOWN

The DXGI-paced workload ran for 1,810.0019503 seconds (received first-to-last frame
span 1,809.9833452 seconds). Host received 98,329 frames from 100,399 source
acquisitions: raw rates 54.325356 and 55.469001 FPS respectively. Host cleanly
disconnected, then its content acceptance check failed with exit 1:
`integration validation failed; inspect report; code=13 (0x0000000D)`.
The runner stopped before the planned fresh-process reconnect and final standard
health stage. Separate read-only post-failure health/event queries were saved.

Exactly 174 received frames decode nonce=0/counter=0 instead of the expected
pattern nonce, between 90.7196575 and 94.3163347 seconds. One additional frame at
90.6019812 seconds retains the nonce but its counter regresses from 16,148 to
16,122. This is a content-oracle failure, not a proven GPU corruption diagnosis.
A transient desktop overlay/dimming or snapshot transition is SUSPECTED; only
hashes/counters were retained, so the cause cannot be established from this run.
Do not silently remove this interval, relax the verifier or count it as a pass.

| Observed metric (not an acceptance pass) | Result |
|---|---:|
| Received / source frames | 98,329 / 100,399 |
| Producer replacements / Host stale / contention | 44 / 1,996 / 30 |
| Busy / invalid metadata / pending at end | 0 / 0 / 0 |
| Queue high-water | 3 |
| Source interval p50 / p95 / p99 / max | 16.7310 / 33.3764 / 33.9865 / 162.0360 ms |
| Host receive interval p50 / p95 / p99 / max | 15.9342 / 31.4779 / 39.2472 / 163.1443 ms |
| Sample-completion interval p50 / p95 / p99 / max | 16.3180 / 34.4687 / 45.6943 / 163.2996 ms |
| Mutex wait p50 / p95 / p99 / max | 0.0478 / 0.0762 / 0.1010 / 0.7612 ms |
| Source-to-sample-complete p50 / p95 / p99 | 8.9666 / 21.6552 / 27.0923 ms |

The sample completion includes the two-row GPU copy/Map and synchronization;
none of these values is an isolated full-frame GPU-copy execution time.
Source/drop accounting is exact: 100,399 = 98,329 + 44 + 1,996 + 30. Content
failures are separate from those drops. 98,111 distinct counters had matching
nonce, an observed 54.204914/s; this does not override the counter regression.

After the first 120 seconds, driver/Host/pattern CPU usage averaged respectively
2.84% / 1.97% / 3.64% of one logical processor (about 0.09% / 0.06% / 0.11% of
this machine's total logical CPU capacity). Sampled 3D engine mean/peak utilization:
driver 3.96%/5%, Host 0.33%/2%, pattern 9.50%/14%; these are per-process engine
counters, not aggregate GPU utilization or GPU execution timers.

Warm-up-excluded private-memory end deltas: driver -28,672 B, Host -57,344 B,
pattern 0 B. Driver handles stay in 933–935, ending at 933 in the sampled stream;
Host 763–767, ending 765; pattern 769–771, ending 769. After disconnect, driver
handles fall to 930, then 928 after settling; private bytes also fall. No continuing
growth is observed in this interval, but this is not an unlimited-duration leak proof.
55 elevated before/periodic health snapshots preserve security and PnP health.
Post-failure read-only checks: both PnP problems 0, same boot, Secure Boot/HVCI
registry state ON, selected driver/system failure events 0. No driver reinstall,
resigning, boot/security/trust change or phone operation occurred.

The owner subsequently authorized a bounded first-failure diagnostic crop and
controlled benign transitions. That investigation is PARTIAL; see
[PHASE3_FIRST_FAIL.md](PHASE3_FIRST_FAIL.md). A controlled grey overlay reproduced
zero nonce/counter with matching GPU crop evidence, but historical cause remains
UNKNOWN. That result alone did not authorize a repeat soak or PHASE 3B–3E work.
The owner later explicitly authorized corrected A–E pilots, observation and a
conditional fresh soak, with a mandatory stop before PHASE 3B.
The driver/three-texture architecture and strict capacity verifier remain unchanged.

## Later gates

- 3B: only after 3A is understood/passes, real Media Foundation hardware H.264,
  DXGI manager and GPU BGRA-to-NV12 conversion as required; progressive real
  encode/decode checks, bounded input and measured latency. No normal full-frame
  CPU readback. Registration is not an encoding result.
- 3C: actual versioned bounded binary framing over localhost, transport abstraction,
  partial read/reconnect/malformed length handling and protocol documentation.
- 3D: actual source -> hardware encode -> protocol -> simulator decode/render,
  including stall/disconnect/restart tests. Synthetic video is not final acceptance.
- 3E: after working video, same-protocol single-contact touch simulation with
  coordinate and disconnect-cancel tests, using documented Windows input APIs.
- 3F: extend tests without weakening existing checks. Prepare logical Linux daemon
  contracts; all Redmi-specific hardware behavior stays UNVERIFIED.

Subphase reporting follows the owner's VERIFIED / PARTIAL / BLOCKED categories;
individual technical claims retain VERIFIED / ASSUMED / SUSPECTED / BLOCKED /
NOT YET TESTED. No push or publication is authorized.
