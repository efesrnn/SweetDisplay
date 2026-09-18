# PHASE 3B — VERIFIED (bounded admission)

Current checkpoint: 2026-09-18. Previous short progressive/capacity/uncapped
results remain VERIFIED and unchanged. The interrupted sustained process actually
completed 1810.0008772 seconds; it was recovered and verified without rerunning.
All 93,069 encoded frames independently decode and match source timestamp/nonce/
counter records. Sustained integrity/resource review passes the latest owner's
bounded-admission criteria, while the stricter all-Host-frame assertion FAILS:
99 explicitly accounted admission drops remain. Clean and forced-Host reconnect
are now independently VERIFIED, with one further accounted transient admission
drop after forced recovery. PHASE 3B is VERIFIED under the latest bounded-admission
acceptance, not lossless-all-Host capacity. Actual 60 unique FPS is not proven.
PHASE 3A is unchanged and VERIFIED; the historical UNKNOWN soak remains UNKNOWN.
No driver deployment, boot/security/certificate/phone/transport operation occurred.

## Recovered sustained run — VERIFIED within bounded-admission scope

Private evidence: `phase3b/sustained-1-uncapped`. Requested1810s, actual
1810.0008772s. Existing Host/pattern exited0 without forced cleanup. No new sustained
or short progressive test was run. Original evidence remains intact; offline
decode and derived review reports were added only to previously absent paths.

| Metric | Result |
|---|---:|
| Source / Host / submitted / encoded / independently decoded | 101,797 / 93,168 / 93,069 / 93,069 / 93,069 |
| Source / Host / submitted / encoded FPS | 56.241409 / 51.474008 / 51.419312 / 51.419312 |
| Producer replacement / Host stale / contention | 69 / 8,509 / 50 |
| Rate-limit / encoder-admission backpressure drops | 0 / 99 |
| Shared queue peak / encoder queue peak / NV12 pool peak | 3 / 1 / 1 |
| Final shared ready / held / encoder pending | 1 / 0 / 0 |
| A / B / C / D / E; invalid | 93,168 / 0 / 0 / 0 / 0; 0 |
| Encoded bytes / actual bitrate | 5,817,406,610 / 25.712282 Mbps |
| Keyframes / parsed IDRs | 776 / 776 |
| Startup latency | 183.730 ms |
| Encode latency mean / p50 / p95 / p99 / max | 8.772909 / 6.7 / 28.2 / 30.8 / 118.3725 ms |
| Conversion completion wall time mean / max | 1.926543 / 133.1967 ms |

Exact accounting:

```text
101797 source = 93168 Host + 69 producer + 8509 stale + 50 contention + 1 ready at cutoff
93168 Host = 93069 submitted + 0 rate-limit + 99 backpressure
93069 submitted = 93069 encoded = 93069 independently decoded
```

The one shared frame at the counter snapshot precedes clean disconnect; it is not
encoder pending output. Held resources0, encoder pending0, clean drain/shutdown.
All accepted inputs have an output; there is no accepted-input output loss.
The 99 rejected admissions are retained as real drops, not upstream losses.

`uncapped-acceptance.json` preserves **FAIL_ALL_UNIQUE_HOST_FRAMES**. The newer
owner instruction permits submission when the bounded pipeline allows it and
requires no unexplained sustained queue/backpressure/latency growth. A separate
hash-bound `sustained-review.json` records **PASS_SUSTAINED_REVIEW** under that
scope. Neither report replaces the other. Recovery's sustained gate accepts this
explicit reviewed scope; binary/queue limits and content/security checks are
unchanged. Do not say all Host frames were encoded or that lossless capacity passed.

### Time distribution and pressure

Buckets use source-relative QPC/PTS and preserve the initial slow interval.

| Seconds | Host count | Encoded count / FPS | Admission drops | Latency mean / p95 / max ms |
|---|---:|---:|---:|---:|
| 0–300 | 8,958 | 8,958 / 29.860 | 0 | 27.270 / 33.131 / 92.640 |
| 300–600 | 16,572 | 16,561 / 55.203 | 11 | 7.069 / 8.315 / 35.337 |
| 600–900 | 16,757 | 16,747 / 55.823 | 10 | 6.824 / 8.175 / 118.373 |
| 900–1200 | 17,033 | 17,008 / 56.693 | 25 | 6.707 / 8.076 / 25.650 |
| 1200–1500 | 16,480 | 16,460 / 54.867 | 20 | 6.708 / 8.080 / 13.909 |
| 1500–1800 | 16,825 | 16,792 / 55.973 | 33 | 6.716 / 8.061 / 68.707 |
| 1800–1810.0008772 | 543 | 543 / 54.295 | 0 | 6.650 / 8.032 / 12.648 |

First near300s upstream telemetry interval contains producer63/stale7722/
contention1, with source16,734 versus Host8,948. These are handoff losses before
encoder admission; no encoder pressure was recorded in the first300s. Later
upstream losses are much smaller but not zero. Exact timing/scheduling cause of
the early slowdown is **UNKNOWN/UNRESOLVED**; conversion and output wall timings
include scheduling/evidence work and cannot isolate ASIC/conversion capacity.

99/93,168 admissions (0.106260%) found no MFT NeedInput token ready. Every such
row has pending1/pool_busy1, below the four-slot limit; maximum consecutive
pressure decisions1. Thus pool exhaustion/full encoder queue is excluded by the
recorded branch conditions. Underlying MFT-event readiness cause is UNKNOWN.
Five-minute pressure counts0/11/10/25/20/33 are not hidden; isolated events do not
form a growing backlog. Queue peak stays1 and later mean latency stays6.7–7.1ms.
No sustained latency accumulation was observed. This does not establish a maximum
encoder throughput or 60FPS; source averaged56.24FPS and the system lost frames.

### Decode, resources and health

Same AMDh264Encoder hardware-only MFT, same D3D11/DXGI device manager, GPU BGRA
conversion to NV12 default-usage surfaces with CPUAccess0. Low latency enabled,
B0, GOP120, target30Mbps, MFT60 configured, submission rate limiter disabled.
No software fallback/normal-path full-frame CPU readback or duplicate generation.
Independent Microsoft decoder accepts all93,069 AUs, visible2400x1080 within
coded2400x1088 explicit aperture. Every decoded PTS/nonce/counter matches the
accepted source frame and original render ledger. This proves sampled marker
correspondence and metadata/resource invariants, not exhaustive lossy pixel identity.
IDRs every120 encoded frames; media-time intervals2.016001..4.779466s stretch
with actual source cadence.

After120s warm-up, Host private83,525,632..83,808,256 bytes, endpoint delta+106,496;
working set73,048,064..73,674,752, delta+569,344; handles1246..1255, ending1246.
Final5min private/working-set endpoints are equal, handles1246 throughout56
samples. Driver private35,614,720..35,704,832, endpoint delta0; handles934..936,
return934; working set decreases610,304 bytes. Small steps settle to plateaus;
no continuing growth observed, not proof against arbitrary small/future leaks.
Host CPU16.230% of one logical processor, driver2.484%; 330 coarse GPU samples:
VideoCodec0 mean16.955%/peak21%, 3Dmean5.564%/peak10%. Early VideoCodec mean9.836%
versus later17.815–18.782%; no maximum encoder capacity is inferred from counters.

Before/55 periodic/after snapshots: adapter/monitor PnP0/0, same installed packages,
same boot, SecureBootON, HVCION, TESTSIGNINGOFF, CIflags62465. Relevant failure
events0. Helper retained; driver continuity also gates subsequent recovery.
Compressed bitstream/AU evidence totals about11.6GB in ignored private storage;
no raw full-frame video or continuous diagnostic images. Preservation check:
560 earlier3A files,302 earlier3B files,five binaries,driver package/shared ABI
hashes unchanged. Historical old3A soak remains UNKNOWN.

## Clean and forced-Host reconnect — VERIFIED within bounded-admission scope

Distinct private evidence: `sustained-1-recovery-clean`,
`sustained-1-recovery-controller` (failed crash harness),
`sustained-2-recovery-controller`, `sustained-2-recovery-after-crash`.
Derived independent decision: `preflight-sustained-1/recovery-final-review.json`.
No sustained/progressive/clean run was repeated to obtain a more favorable result.

| Session | Seconds | Source / Host / submitted=encoded=decoded | Source / Host / encoded FPS | Producer / stale / pressure |
|---|---:|---:|---:|---:|
| Clean baseline | 20.0231764 | 1120 / 654 / 654 | 55.935 / 32.662 / 32.662 | 1 / 465 / 0 |
| Fresh Host after clean shutdown | 15.0018052 | 849 / 509 / 509 | 56.593 / 33.929 / 33.929 | 1 / 339 / 0 |
| Fresh Host after active-encode forced termination | 20.0020479 | 1104 / 1094 / 1093 | 55.194 / 54.694 / 54.644 | 0 / 10 / 1 |

All three have rate-limit/contention/invalid/D/E0, encoder peak1, final pending/
held0; shared peaks3/3/2. Independent decode/source timestamp/nonce/counter pass
for all654/509/1093 admitted frames. Exact equations:1120=654+1+465;
849=509+1+339;1104=1094+10;1094=1093+1pressure. Clean recovery's upstream slowdown
is explicit; this proves restart functionality, not immediate sustained55/60FPS.

First crash harness stopped on a live stdout file-sharing conflict before its
active-output checkpoint. Own Host cleanup and own pattern exit0 were recorded.
Failure is retained; no successful crash test is claimed for that attempt.
Read-only shared log opening was fixed and independently reproduced/tested with
an open writer. Driver/Host binaries, filesystem permissions and queues unchanged.
Only the missing crash step was retried, referencing already verified clean evidence.

In the second crash test, actual AU evidence grew to3,936,256 bytes and Host
reported processed frames before targeted termination (exit0xFFFFFFFF).
Instantaneous keyed-mutex ownership at the kill was not asserted. With Host absent,
driver remained active and disconnected, producing112 new source frames in
2.0005689s (55.984FPS). Original helper, driver process and boot retained.
New Host uses a fresh three-texture resource generation, first frame ID greater
than the terminated Host's last report, same driver epoch, PTS resets to0, first
submission pending1/pool1. Actual GPU-backed hardware MFT reinitialization and
decode/content proof establish functional recovery without stuck ownership.

One new Host frame at4.9502878s encountered absent MFT input token with pending1/
poolbusy1. Controller's strict zero-pressure assertion stopped with
`Recovery all-Host frame failure`; its FAIL outcome remains unchanged. Existing
encode/content verifier had already passed; no accepted output was lost. Offline
independent review completed the remaining resource-generation/order/health/event
checks, with no more live encoding. Latest owner's no-persistent-backpressure
scope passes separately; strict all-Host assertion still fails. Four5s latency
means7.245/7.239/7.356/7.140ms, no persistent growth. Post-crash p50/p95/p99/max
7.1/8.4/11.3/30.6172ms; actual27.325Mbps,10IDRs. Rate-limit0 and no duplication.

All normal Host/pattern sessions exit0, encoder drains and shuts down S_OK;
only the explicitly targeted crash Host is killed. Driver post-cleanup private
35,643,392B and handles931 match after sustained, clean and forced recovery.
No leftover Host/pattern; original helper kept running. Nine recovery health
snapshots plus child post-test health: PnP0/0, same boot/driver/helper, SecureBootON,
HVCION, TESTSIGNINGOFF. Event query covering both controllers and cleanup finds
zero relevant driver/system failures. Evidence demonstrates bounded recovery;
future rare leaks/scheduling causes are not ruled out.

**Final scope:** sustained GPU-native hardware encode, exact bounded drop accounting,
all admitted output independently decoded/content matched, stable observed resources
and latency, clean and forced-Host reconnect VERIFIED. Early slowdown and underlying
transient input-token timing remain UNKNOWN/UNRESOLVED. No lossless-all-Host or
60FPS claim. All earlier successful/failed/UNKNOWN evidence is preserved. Stop here;
PHASE 3C, driver/deployment/security changes and phone work are not started.

## First live stage — VERIFIED

Evidence: ignored `phase3b/encode-800-2`. Actual Display 3 / SWT0001 frames travel
through the unchanged IddCx driver and three shared BGRA8 D3D11 textures, then
Host's same-device GPU video processor and four bounded NV12 GPU surfaces into
an adapter-filtered Media Foundation hardware H.264 MFT. No normal full-frame
CPU readback or software encoder fallback is present.

| Metric | Measured result |
|---|---:|
| Main duration | 20.0017116 s |
| Input geometry / encoded visible geometry | 2400x1080 BGRA8 / 800x360 H.264 |
| Source acquisitions / Host received | 1,105 / 1,104 |
| Source / Host input FPS | 55.245272 / 55.195276 |
| Encoder accepted / output access units | 200 / 200 |
| Observed output FPS | 9.999144 |
| Intentional rate drops / encoder-backpressure drops | 904 / 0 |
| Upstream Host-stale / producer / contention drops | 1 / 0 / 0 |
| Shared queue peak / encoder queue peak / NV12 pool peak | 2 / 1 / 1 |
| Invalid / pending / held at completion | 0 / 0 / 0 |
| A / B / C / D / E | 1104 / 0 / 0 / 0 / 0 |
| Encoded bytes | 5,001,650 |
| Observed bitrate / CBR target | 2,000,488.798 bit/s / 2,000,000 bit/s |
| Keyframes / parsed IDRs | 10 / 10 |
| Startup to first output | 184.4544 ms |
| Accepted-input to observed-output latency mean / peak | 4.475331 / 19.0161 ms |
| Latency histogram p50 / p95 / p99 | 4.1 / 6.1 / 12.2 ms |
| GPU conversion completion wall time mean / peak | 3.771711 / 16.1131 ms |

Exact accounting: **1105 = 1104 received + 1 upstream stale**;
**1104 = 200 accepted + 904 deliberate rate drops + 0 backpressure**;
**200 accepted = 200 output = 200 independently decoded**. Nothing was duplicated
to reach the target rate. This test establishes actual 10 FPS output, not maximum
encoder throughput or 60 FPS capacity. Latency includes Host scheduling/output
drain and evidence I/O; conversion wall time is not isolated GPU execution time.
Histogram percentiles have 0.1 ms bins; exact per-frame latency remains in CSV.

## Hardware path evidence

Selected **AMDh264Encoder**, CLSID `{ADC9BC80-0F41-46C6-AB75-D693D793597D}`,
on the driver-selected Radeon 610M adapter. The combined evidence is:

- MFTEnum2 uses HARDWARE-only category and the actual render adapter LUID.
- Hardware URL exists; activated MFT reports async=1 and D3D11-aware=1.
- Same-device MF DXGI manager setup and NV12/H.264 negotiation return S_OK.
- Actual input IMFDXGIBuffer returns the expected NV12 GPU texture identity,
  800x360, default usage, CPUAccessFlags=0, current length432000.
- ProcessInput/ProcessOutput produce actual valid H.264, independently decoded.
- No software encoder is selected or enumerated as a fallback.

Low-latency=true, B pictures=0, GOP=20, CBR configured and read back through
ICodecAPI. H.264 Main profile requested; level is automatic and not separately
parsed/reported yet. Runtime recorded no MF error; drain, END_STREAMING and
IMFShutdown returned S_OK. GPU sample reference-release callbacks returned all
pool slots before shutdown acceptance.

Integer per-process GPU counters show 3D mean1.25%, peak2%; Video Codec0 samples
are0%. This low-load sampling does not independently quantify ASIC encode-engine
utilization. Hardware-path verification rests on the hardware-only MFT contract,
actual DXGI GPU inputs and valid outputs, not the vendor name alone.

The NVIDIA H.264 candidate `{60F44560-5A20-4857-BFEF-D29773CB8040}` failed activation
with **0x8000FFFF** during discovery. Cause UNKNOWN; record retained. No cross-GPU
fallback or architecture redesign was attempted.

## Independent bitstream/content validation

`DecodeH264Evidence.exe` uses separate Microsoft CLSID_CMSH264DecoderMFT resources
with no encoder device manager. CPU decoding here is an offline verifier, not a
normal encode-path readback or a simulator integration. The actual 200 captured
access units all decode; timestamps progress and each decoded nonce/counter
matches its accepted input and original Display 3 frame record. The independent
PowerShell verifier also checks bytes, IDs, all drops and resource/queue bounds.

First decode attempt rejected a dimension mismatch; a second diagnostic attempt
showed **800x368 coded allocation** with explicit **800x360 minimum/geometric
visible aperture**, offset0. The decoder was corrected to validate Microsoft's
visible-aperture contract and rectangle bounds. It never substitutes expected
size for missing stream metadata. Successful decode lives separately in
`host/decode-3`; both failed attempts remain intact. An intentionally wrong
800x400 expected visible size still fails. This is a verifier implementation
correction, not permission to accept an incorrect visible resolution.

Content correspondence covers the controlled sampled nonce/counter regions on
all200 decoded frames; it is not an exhaustive comparison of every lossy pixel.
No unexpected content or unclassified event was omitted.

## Resources, shutdown and health

Four process samples span about16.1s and include startup. Host CPU averages7.77%
of one logical processor; driver2.14%, pattern3.40%. Host private bytes decrease
from54,026,240 to50,384,896; handles1531 to1257. Driver private range35,233,792–
35,291,136, handles931–934. These short measurements are not sustained leak proof.

Host and producer exit0; the exact producer-window close succeeds without forced
termination. Encoder drain/shutdown and shared-texture disconnect are clean.
A fresh-process encoding reconnect has NOT YET TESTED status.
Recorded PnP codes0/0, Secure BootON, HVCION, TESTSIGNINGOFF, CIflags62465, unchanged
boot and selected failure-events0. Driver DLL/INF/CAT, shared ABI and known-good
PHASE3A Host binary remain unchanged. All560 prior evidence hashes match baseline.

## Remaining progressive gates

### Uncapped-source submission control — VERIFIED for all unique Host frames

Evidence: `phase3b/uncapped-1-2400`; previous capped run unchanged. The only
behavioral encoder change is opt-in `--encode-uncapped`, which bypasses the
submission time-bin selector. Default remains capped. MFT rate60,bitrate30Mbps,
lowlatencyON,B0,GOP120, sample timestamps/durations, GPU conversion and source
pattern/IddCx cadence configuration are unchanged. Four NV12 surfaces, three
shared slots, token availability and original backpressure/drop policy remain.

`Build-UncappedEncodeHost.cmd` builds the same Host source into isolated
`out/encode-host-control`; `Test-HardwareEncoding.ps1 -UncappedSubmission` selects
that binary and adds the option, logging it in execution.json and encoder calls.
Previous encode Host, known-goodHost, decoder and pattern binaries are unchanged.
First build attempt failed because EWDK was no longer mounted after reboot;
failure preserved. Existing ISO was mounted read-only atD:, then EWDK26100.6584
build succeeded without compiler warnings. No Windows/driver installation.
CLI rejects uncapped without encode before device access (87/0x57); option
guard check passed. Live preflight/verification acceptance was not weakened.

| Metric | Measured result |
|---|---:|
| Duration | 30.0096792s |
| Source count / FPS | 1654 / 55.115551 |
| Host count / FPS | 1637 / 54.549067 |
| Submitted count / FPS | 1637 / 54.549067 |
| Encoded count / FPS | 1637 / 54.549067 |
| Independent decoded count | 1637 |
| Producer / stale / contention | 0 / 17 / 0 |
| Rate-limit / backpressure | 0 / 0 |
| Shared / encoder queue / NV12 pool peaks | 3 / 1 / 1 |
| Encode latency p50 / p95 / p99 / max | 6.6 / 8.2 / 10.4 / 13.5382ms |
| Mean encode latency / startup | 6.829216 / 170.5762ms |
| Conversion completion wall time mean / max | 2.666908 / 23.2895ms |
| Source-to-submit latency mean / max | 9.867293 / 28.6689ms |
| Encoded bytes / actual bitrate | 102,322,966 / 27,277,323.511bit/s |
| IDRs / keyframes | 14 / 14 |
| A / B / C / D / E | 1637 / 0 / 0 / 0 / 0 |
| Invalid / pending / held at completion | 0 / 0 / 0 |

Exact accounting:1654source=1637Host+17upstream-stale;1637Host=1637submitted+
0rate+0backpressure;1637submitted=1637encoded=1637decoded. Every Host frame ID is
unique and every input decision is accepted. Independent decoding of the actual
bitstream and all decoded sourcePTS/nonce/counter associations pass; no duplicate
or interpolated frames. Visible2400x1080 aperture inside coded2400x1088 validated.
The unchanged general verifier passed, followed by explicit uncapped acceptance
checking disabled-limiter evidence, all-Host equality and zero rate/backpressure.

**Interpretation: encoder kept up with every unique frame received by Host in
this30s run.** All1637 frames finished, encoder queue/pool peak1 and0backpressure.
Four7.5s mean encode latencies6.803/6.833/6.895/6.786ms do not accumulate. This
supports a source/Host-delivery-limited result at54.549FPS rather than an observed
MFT throughput ceiling.17 stale drops occurred before submission in the retained
Host queue policy; they are not hidden or attributed to encoder rate/backpressure.
Their exact scheduling cause is not isolated by this short observation. Conversion
completion wall time includes scheduling; its23.2895ms maximum does not by itself
prove a conversion throughput bottleneck. No claim that all1654 IddCx frames were
encoded, that maximum encoder capacity was found, or that60FPS was achieved.

Actual AMDh264Encoder CLSID{ADC9BC80-0F41-46C6-AB75-D693D793597D}, hardware-only
async/D3D11-aware MFT with same-device DXGI manager and actualNV12GPU CPUAccess0
continues. No software fallback/full-frame CPU readback in normal encoding.
IDRs remain exactly120 encoded frames apart, media gaps2.101..2.292s. MF
drain/end/Shutdown succeed; independent decoder is offline validation only.

Six resource samples span28.28s including startup: Host34,418,688bytes/964handles
initially, then86,568,960bytes/1254handles at all five samples from6.964s onward.
Driver private35,659,776bytes throughout,handles928..931. CPU one-core averages
Host22.543%,driver2.487%,pattern3.869%. GPU integer counters:3Dmean5.5%/peak6%,
VideoCodec0mean18.667%/peak21%. Short resource plateau only; no sustained proof.
Host/pattern exit0,no forced stop,cleandrain/disconnect,failureevents0. Before/
periodic/after PnP0/0,sameoem83/oem81,sameboot,SecureBootON,HVCION,TESTSIGNINGOFF,
CIflags62465. Existing helper remains running.250 previous3B files,560 previous3A
files, stageddriver/sharedABI and old binaries SHA256-identical. HistoricalUNKNOWN
soak preserved. PHASE3B PARTIAL; stopped before sustained/reconnect/PHASE3C.

### Latest60FPS target capacity observation — VERIFIED at measured rate

Fresh evidence: `phase3b/capacity-1-2400-60`. Target2400x1080,60FPS,30Mbit/s,
30seconds; passed the unchanged elevated preflight before any source/Host frames.
This is a short observation and integrity result, not a claim of60 encodedFPS.

| Metric | Measured result |
|---|---:|
| Duration | 30.008907s |
| Source / Host counts | 1682 / 1682 |
| Submitted / encoded / independently decoded counts | 1572 / 1572 / 1572 |
| Source / Host FPS | 56.050025 / 56.050025 |
| Submitted / encoded / decoded media-equivalent FPS | 52.384447 / 52.384447 / 52.384447 |
| Producer / stale / contention / busy drops | 0 / 0 / 0 / 0 |
| Rate-limit / backpressure drops | 110 / 0 |
| Shared / encoder queue / NV12 pool peaks | 2 / 1 / 1 |
| A / B / C / D / E | 1682 / 0 / 0 / 0 / 0 |
| Invalid / final pending / held | 0 / 0 / 0 |
| Encode latency p50 / p95 / p99 / max | 6.6 / 8.2 / 10.0 / 17.9433ms |
| Mean encode latency / startup | 6.854583 / 178.0558ms |
| Conversion completion wall time mean / max | 1.971667 / 17.885ms |
| Encoded bytes / actual bitrate | 98,260,076 / 26,194,909.665bit/s |
| IDR / keyframe count | 14 / 14 |

Exact accounting:1682source=1682Host;1682Host=1572submitted+110rate-limit+
0backpressure;1572submitted=1572encoded=1572decoded. Decoder FPS above divides
offline decoded count by live-test duration; decoder wall-clock capacity was not
measured. No duplicated frames or fabricated60FPS count. Output/source IDs and
timestamps, all decoded nonce/counter sampled regions, bytes and queue bounds
pass independent verification. Visible decoded2400x1080 is explicitly described
by the bitstream aperture inside coded2400x1088; bounds validated unchanged.

**Bottleneck finding: source cadence below60 plus Host rate-selector loss;
ENCODER-LIMITED behavior was not observed.** The current selector admits only the
first source frame in each fixed1/60-second bin relative to first-sourceQPC.
Independent replay of all1682 QPC records reproduces all110 rate drops exactly,
zero decision mismatches.110 bins contain two distinct source frames; arrival
jitter/bursting crosses the selector boundaries even though mean sourceFPS<60.
Dropped arrivals precede next-bin boundary by0.0043..4.3044ms (mean1.9253ms).
They never reached the MFT. Thus the52.384FPS result is not an encoder ceiling.

There is no encoder backpressure, queue/pool peak1, and all1572 submitted frames
complete. Four7.5s latency means6.850/6.881/6.791/6.897ms show no accumulating
latency. In the owner's strict SOURCE-LIMITED definition, processing *every*
available unique source frame is still NOT YET TESTED:110 were excluded upstream
of encoder submission. Observations support source/Host-policy limitation with
no observed encoder saturation, not a proof of spare capacity for all1682 or60FPS.
No limiter change, additional test or architecture change was made this turn.

Actual AMDh264Encoder CLSID{ADC9BC80-0F41-46C6-AB75-D693D793597D}, hardware-only
async/D3D11-aware enum and same-adapter DXGI manager remain active. GPU-native
BGRA->NV12, actual2400x1080 defaultGPU surfaceCPUAccess0,length3888000; software
fallback=false. LowlatencyON,B0,GOP120,CBR30Mbps configured/read back. IDRs at
output indices0,120,...1560: exactly120 coded frames apart, media-time gaps
2.200..2.392s reflecting actual cadence. Recorded MF drain/Shutdown S_OK.

Six resource samples over28.26s including startup: Host33,447,936bytes/959handles
initially, then85,536,768..85,544,960bytes/1254handles from6.965s onward. Driver
private35,561,472..35,622,912bytes,handles926..931. One-core CPU Host18.520%,
driver2.710%,pattern4.093%. Integer GPU samples:3Dmean5%/peak6%,VideoCodec0
mean17.833%/peak21%. Sampling is coarse; no maximum-capacity claim follows from
GPU utilization alone, and this is not sustained memory stability proof.

Host/pattern exit0, no forced stop, clean encoder/shared disconnect, failureevents0.
Before/periodic/after adapter+monitor OK/problem0/0,oem83/oem81,SecureBootON,HVCION,
TESTSIGNINGOFF,CIflags62465,sameboot. Source is the active SWT0001 indirect desktop.
All209 prior3B evidence files and560 prior3A evidence files remain SHA256-identical;
staged driver/ABI and known-goodHost unchanged. HistoricalUNKNOWN soak unchanged.
PHASE3B remains PARTIAL; sustained/reconnect/PHASE3C not started.

### Latest2400x1080@30 target stage — VERIFIED short stage

Fresh evidence: `phase3b/progressive-1-2400-30`. The unchanged runner passed
adapter/monitor OK/problem0 and security checks before starting the pattern/Host.
Only this30-second stage ran. Actual source remained the identified SWT0001
2400x1080@60 indirect desktop. No reinstall/redeploy, software fallback or normal
full-frame CPU readback; existing helper stayed under owner control.

| Metric | Measured result |
|---|---:|
| Duration | 30.0116845s |
| Source / Host frames | 1686 / 1684 |
| Source / Host / encoded FPS | 56.178120 / 56.111479 / 29.655116 |
| Submitted and accepted / encoded / independently decoded | 890 / 890 / 890 |
| Producer / stale / contention drops | 0 / 1 / 1 |
| Rate-limit / backpressure drops | 794 / 0 |
| Shared / encoder queue / NV12 pool peaks | 2 / 1 / 1 |
| A / B / C / D / E | 1684 / 0 / 0 / 0 / 0 |
| Invalid / pending / held at completion | 0 / 0 / 0 |
| Encoded bytes / measured bitrate | 55,631,030 / 14,829,165.620bit/s |
| Keyframes / IDRs | 15 / 15 |
| Startup latency | 186.6423ms |
| Encode latency mean / p50 / p95 / p99 / max | 6.943102 / 6.7 / 8.2 / 11.8 / 23.7973ms |
| Conversion completion wall time mean / max | 2.363925 / 21.6283ms |

Exact accounting:1686=1684+0producer+1stale+1contention;
1684=890submitted+794rate-limit+0backpressure;890submitted=890encoded=890decoded.
Maximum encoder capacity was not isolated. Observed output is29.655116FPS at
target30, not a claim of exactly30FPS sustained or60FPS capability. Nothing was
duplicated. Latency histogram uses0.1ms bins; completion wall time includes CPU
scheduling and is not isolated GPU execution time.

Actual encoder remains AMDh264Encoder, CLSID
{ADC9BC80-0F41-46C6-AB75-D693D793597D}, hardware-only enum, async/D3D11-aware,
same-adapter DXGI manager. ActualNV12 input2400x1080, default usage,CPUAccess0,
currentlength3888000. Low latencyON,Bframes0,GOP60,CBR15Mbps readbacks succeed.
MF drain/end/shutdown S_OK. Independent Microsoft H.264 decoder accepted all890
actual access units: coded2400x1088, explicit visible aperture2400x1080 at0,0.
Unchanged verifier validates aperture bounds; each decodedPTS/nonce/counter
matches its source/input record. This is sampled-marker content correspondence,
not exhaustive lossy-pixel equivalence. PASS_ENCODE_DECODE and exact bytes/IDs/
timestamps/order/drop bounds all pass.

Six process samples over28.26s include startup. Host initially33,345,536bytes/
947handles; from6.997s onward private83,845,120..83,849,216bytes,handles1254.
Driver private35,606,528..35,647,488bytes,handles930..933. CPU one-core averages
Host11.999%,driver2.822%,pattern4.260%. Integer GPU counters:3Dmean/peak3%/3%,
VideoCodec0mean9.833%/peak10%. These short observations do not establish sustained
resource stability or maximum engine capacity.

Host/pattern exit0, no forced stop, clean encoder shutdown/shared disconnect;
failureevents0. Before/periodic/after PnP0/0, sameoem83/oem81 packages, sameboot,
SecureBootON,HVCION,TESTSIGNINGOFF,CIflags62465. SHA256 confirms all118 files in
previous successful800/1280/1920 and failed1920 runs unchanged, all560 prior3A
evidence unchanged, staged driver/ABI and known-goodHost unchanged. Old soak
UNKNOWN remains historicalUNKNOWN. 2400x1080@60 NOT STARTED; PHASE3B PARTIAL.

### Manually launched1280x576 stage — VERIFIED

Evidence: `phase3b/progressive-1-1280-30`. Owner ran the single live runner from
Administrator PowerShell. No automatic elevation or full progressive controller
was started. The30.0008517s run passed; independent actual-bitstream decode and
source correspondence checks subsequently returned PASS_ENCODE_DECODE.

| Metric | Measured result |
|---|---:|
| Source / Host frames | 1601 / 1540 |
| Source / Host input / encoded output FPS | 53.365152 / 51.331876 / 29.299168 |
| Accepted / output / independently decoded | 879 / 879 / 879 |
| Deliberate rate drops / encoder backpressure drops | 661 / 0 |
| Producer / Host stale / contention drops | 4 / 57 / 0 |
| Shared queue / encoder pending queue / NV12 pool peaks | 3 / 1 / 1 |
| A / B / C / D / E | 1540 / 0 / 0 / 0 / 0 |
| Invalid / final pending / final held | 0 / 0 / 0 |
| Bytes / measured bitrate | 18,318,433 / 4,884,776.788bit/s |
| Keyframes / IDRs | 15 / 15 |
| Startup latency | 189.7684ms |
| Encode latency mean / p50 / p95 / p99 / peak | 4.649266 / 4.1 / 7.2 / 15.1 / 27.5568ms |
| Conversion completion wall time mean / peak | 4.074951 / 63.6476ms |

Exact totals:1601=1540+4+57;1540=879+661+0;879accepted=879output=879decoded.
All decoded PTS/nonce/counter match source records; visible and coded resolution
both1280x576. AMDhardware-only MFT, same DXGI GPU path, inputNV12 CPUAccess0,
no software fallback. All recorded MF setup/drain/shutdown calls S_OK.
This establishes29.299168FPS actual output at target30, not an independently
measured maximum encoder capacity or a guarantee of sustained30/60FPS.

Six resource samples cover28.45s including startup: Host private53,727,232 to
57,348,096bytes, handles1536 to1262; driver private35,258,368 to35,311,616bytes,
handles931 to934. CPU averages16.147%Host,2.363%driver,2.253%pattern of one logical
processor. These short observations do not establish long-term memory stability.
Host/producer exit0, no forced termination, clean shared disconnect and encoder
shutdown. Before/periodic/after PnP0/0, SecureBootON,HVCION,TESTSIGNINGOFF,
CIflags62465, unchanged boot, selected failure-events0. No fresh-process reconnect
was requested for this short stage. Next manual stage1920x864@30,10Mbit/s,30s;
its separate evidence directory is absent. Do not launch the all-stage controller
or any soak under the current manual single-stage workflow.

| Mode | Bitrate target | Status |
|---|---:|---|
| 800x360@10 | 2 Mbit/s | VERIFIED short real encode/decode/content stage |
| 1280x576@30 target | 5 Mbit/s | VERIFIED short encode/decode/content stage; actual29.299168FPS |
| 1920x864@30 target | 10 Mbit/s | VERIFIED short encode/decode/content stage; actual29.796274FPS |
| 2400x1080@30 target | 15 Mbit/s | VERIFIED short encode/decode/content stage; actual29.655116FPS |
| 2400x1080@60 target | 30 Mbit/s | VERIFIED short observation at52.384447FPS; source+Host-selector limitation; actual60FPS/all-source capacity unproven |
| 2400x1080 uncapped submission, MFT60 target | 30 Mbit/s | VERIFIED all unique Host frames at54.549067FPS;17 upstream stale; no actual60FPS claim |
| Sustained highest passing mode,600s | 25 Mbit/s planned | NOT YET TESTED |
| Fresh-process encoder/Host reconnect,15s | same mode | NOT YET TESTED |

`Test-HardwareEncodingProgressive.ps1 -RunPrefix progressive-1` is a fixed,
bounded sequence: each30s mode must pass actual independent decode/content checks
before advancing; then600s sustained plus15s reconnect. Stop on first failure.
Final memory/handle review is mandatory and distinct from automatic test passes.

Both progressive UAC attempts were cancelled by Windows with
`İşlem kullanıcı tarafından iptal edildi` before controller launch. The second
attempt followed the owner's explicit request to reopen the prompt; the Windows
consent process was observed, but Start-Process returned exit 1 without an elevated
controller PID. The corrected launch wrapper stops on this error instead of
reporting an empty PID as success. The native numeric cancellation code was not
exposed in the command output and is not inferred here.

At that failed-launch checkpoint no progressive run directory existed and no
higher-resolution test had run. Separate
`preflight-1/uac-progressive-2-failure.json` preserves the second attempt; the first
800x360 success was not rerun or overwritten. Explicit PHASE3B chat authorization
remains valid; successful Windows consent is still required. Read-only preservation
checks again matched all560 prior evidence files, the known-good Host, driver
package and shared ABI. No test Host, pattern or consent process remained afterward.

## Preserved earlier failures and implementation checks

### Fresh1920x864 retry — VERIFIED short stage

Evidence: `phase3b/progressive-2-1920-30`; original failed1920 run remains intact.
Owner restarted existing helper and explicitly reauthorized UAC. Elevated runner
verified adapter/monitor OK/problem0 on oem83.inf/oem81.inf before creating the
pattern or Host. UAC launch succeeded this time. No package/certificate/security
change. Runtime source was the active SWT0001 indirect2400x1080@60 target;
the GDI name after reboot was DISPLAY10, not a reliance on an old display number.
Windows Settings numbering was not independently reread. Source identity and
pattern/source/decoded correspondence establish the intended SweetDisplay content.

| Metric | Measured result |
|---|---:|
| Duration | 30.0037516s |
| Source / Host received | 1695 / 1686 |
| Source / Host input / encoded output FPS | 56.492935 / 56.192973 / 29.796274 |
| Accepted / outputs / independent decoded | 894 / 894 / 894 |
| Upstream producer / Host stale / contention | 0 / 4 / 5 |
| Deliberate rate drops / encoder backpressure | 790 / 2 |
| Shared queue / encoder queue / NV12 pool peaks | 2 / 1 / 1 |
| A / B / C / D / E | 1686 / 0 / 0 / 0 / 0 |
| Invalid / final pending / held | 0 / 0 / 0 |
| Encoded bytes / bitrate | 37,256,023 / 9,933,697.225bit/s |
| Keyframes / IDRs | 15 / 15 |
| Startup latency | 203.1429ms |
| Encode latency mean / p50 / p95 / p99 / peak | 6.374272 / 5.9 / 10.7 / 15.6 / 20.5087ms |
| Conversion completion wall time mean / peak | 3.624872 / 16.4157ms |

Accounting:1695=1686+0+4+5;1686=894+790+2;894accepted=894output=894decoded.
The two backpressure decisions are retained, not excluded. No duplicate frames
were created. Encoder throughput observed29.796274FPS; maximum capacity was not
isolated and sustained30/60FPS is not claimed. All894 decoded1920x864 frames pass
PTS/frame/source nonce/counter association. Sampled content regions, not exhaustive
lossy-pixel equivalence, are checked. Actual AMD hardware-only async/D3D11 MFT,
same-device DXGI manager, NV12 GPU surfaces CPUAccess0; no software fallback or
normal full-frame CPU readback. MF drain/shutdown S_OK; no recorded runtime error.

Five resource samples include startup. At11.97/17.31/22.67/28.05s Host private
bytes stay70,934,528 and handles1254; initial sample was34,172,928/964. Driver
private35,487,744..35,639,296 and handles928..933. CPU one-core Host16.729%,
driver3.079%,pattern4.422%. Integer GPU counters:3Dmean6.4%/peak7%,VideoCodec0
mean6.8%/peak7% (five samples, coarse sampling, not maximum engine capacity).
Short-run plateau is observed; sustained resource acceptance remains pending.

Host/pattern exit0, no forced kill, clean encoder drain and shared disconnect.
Before/periodic/after PnP0/0,SecureBootON,HVCION,TESTSIGNINGOFF,CIflags62465,
same boot within run, failure-events0. Fresh-process encoding reconnect was not
part of this short test. SHA256 preservation:81 prior3B files (including failed1920),
560 prior3A evidence files, known-goodHost, staged driver package and shared ABI
all unchanged. Historical soak UNKNOWN stays unchanged. Next planned stage is
2400x1080@30 target,15Mbit/s,30seconds; NOT STARTED per explicit stop instruction.

After the owner rebooted, manual `progressive-1-1920-30` stopped in preflight
with `SweetDisplay PnP health failure`. Both health snapshots contain an empty
Devices array, SecureBoot=true,HVCI=1,CIstatus0,CIflags62465. Host directory is
empty and Commands is empty: no frames or encode work began. No hardware encoder
failure is established. SweetDisplayDevice helper was not running. Its source
uses default SwDeviceLifetimeHandle, with no persistent lifetime or startup
service; disappearance when the helper ends is consistent with the documented
software-device contract. Restart the existing helper manually and verify both
PnP nodes before another test. No driver package install or policy change is
needed as an initial recovery step. Recovery was pending then and is now VERIFIED
by the fresh retry above. Preserve this
failed run; use a fresh name such as progressive-2-1920-30 for any later retry.
An additional unelevated Get-PnpDevice query returned Access denied; diagnosis
uses the owner's elevated test snapshots, not that failed inventory query.

Owner clarification: no UAC window appeared and the owner did not select No.
The recorded cancellation text is a launch error, not evidence of a user refusal;
why consent was not visible remains UNKNOWN. Do not retry RunAs or change policy.
The next launch is manual from an owner-opened Administrator PowerShell, using
Test-HardwareEncoding.ps1 for progressive-1-1280-30 only: 30 seconds, observe,
classified, 1280x576, target30FPS, bitrate5000000. The evidence directory was
confirmed absent; the existing script refuses an existing run directory before
writing. No reconnect, further mode or soak is included in this command.
Independent decoding/content verification follows from the resulting evidence.

`encode-800-1` failed before frames because the new executable was built one
folder too deep for the existing private-evidence guard. Its error and clean
producer cleanup remain recorded. The guard was not changed; new builds use
`out/encode-host`, preserving `out/host` and PHASE3A evidence. Earlier compiler
include/MSBuild environment failures and the exploratory probe's redundant
ShutdownObject error0x80004005 after IMFShutdown are retained; none count as a
successful live gate. Earlier automatic approval rejections were resolved by
explicit in-chat PHASE3B/UAC authorization.

Final Host and decoder builds pass without warnings. Four malformed-AU cases,
one wrong-visible-size real-bitstream case and eight encoding-evidence fixtures
pass (including wrong content/PTS, queue overflow, hidden drops, software fallback
and byte-total rejection). Existing shared-texture/A–E verification remains active.

At that historical checkpoint PHASE3B was PARTIAL pending progressive,
sustained/resource and reconnect gates. The current bounded-admission decision
above supersedes that pending status. No PHASE3C/transport/USB/simulator/phone work.

## Microsoft contracts

- [Adapter-filtered hardware MFT enumeration](https://learn.microsoft.com/en-us/windows/win32/api/mfapi/nf-mfapi-mftenum2).
- [Async MFT events, drain and shutdown](https://learn.microsoft.com/en-us/windows/win32/medfound/asynchronous-mfts).
- [GPU-backed MF buffer](https://learn.microsoft.com/en-us/windows/win32/api/mfapi/nf-mfapi-mfcreatedxgisurfacebuffer).
- [Tracked sample release](https://learn.microsoft.com/en-us/windows/win32/api/mfidl/nf-mfidl-imftrackedsample-setallocator).
- [GPU video processor](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11videocontext-videoprocessorblt).
- [H.264 decoder](https://learn.microsoft.com/en-us/windows/win32/medfound/h-264-video-decoder).
- [Display aperture and coded frame size](https://learn.microsoft.com/en-us/windows/win32/medfound/picture-aspect-ratio).
