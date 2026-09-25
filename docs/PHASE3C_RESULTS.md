# PHASE 3C results — 2026-09-19

**VERIFIED — bounded localhost transport scope.** The owner ran `flow-content-3`;
both normal and real receiver reconnect stages pass, including independent
hardware H.264 decode/content/byte integrity, exact accounting and clean process
shutdown. Independent closed-artifact rechecks are in `phase3c/final-review-1`.
No new live test was started during final review. PHASE 3D has not started.

## Final acceptance: flow-content-3

Existing 58,155 protocol/parser/queue/resync checks and normal/slow synthetic
fixtures pass; nine shutdown cases pass in both PowerShell environments. Fresh
real Display 3 runs below complete the remaining live acceptance gates.

| Measurement | Normal | Receiver disconnect/reconnect |
|---|---:|---:|
| Host observation duration | 35.0015254 s | 45.0023031 s |
| Source count / FPS | 1,977 / 56.483253 | 2,509 / 55.752702 |
| Host count / FPS | 1,786 / 51.026348 | 2,115 / 46.997595 |
| Submitted = encoded = independently Host-decoded | 1,782 | 2,106 |
| Encoded FPS using Host duration | 50.912067 | 46.797605 |
| Receiver validated / ACKed | 1,782 / 1,782 | 1,907 / 1,907 |
| Receiver capture byte-identical and independently decoded | 1,782 | 1,506 after reconnect |
| Encoder pressure / rate drops | 4 / 0 | 9 / 0 |
| Producer / stale / contention drops | 0 / 190 / 1 | 12 / 380 / 2 |
| Transport disconnected / resync / unconfirmed | 0 / 0 / 0 | 160 / 38 / 1 |
| Transport queue overflow / aborted / expired / final pending | 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 |
| Shared / encoder / transport queue peaks | 3 / 1 / 1 | 3 / 1 / 1 |
| Transport queue byte peak | 62,616 | 62,616 |
| A / B / C / D / E | 1786 / 0 / 0 / 0 / 0 | 2115 / 0 / 0 / 0 / 0 |
| Actual encoded bitrate | 25.458635 Mbps | 23.401196 Mbps |
| Encoder latency p50/p95/p99/max | 6.8 / 8.2 / 12.6 / 24.6932 ms | 7.0 / 27.8 / 31.8 / 35.4483 ms |
| Transport latency p50/p95/p99/max | 0.3746 / 0.6101 / 0.6828 / 2.0097 ms | 0.3685 / 0.6118 / 0.6801 / 2.6576 ms |
| IDRs | 15 | 18 |
| Protocol errors / invalid samples | 0 / 0 | 0 / 0 |
| Final Host / receiver / pattern exit | 0 / 0 / 0 | 0 / 0 / 0 |

Exact accounting:

```
Normal source:   1977 = 1786 Host + 190 stale + 1 contention
Normal encoder:  1786 = 1782 accepted + 4 pressure
Normal transport:1782 = 1782 admitted = sent = received = ACKed

Reconnect source:    2509 = 2115 Host + 12 producer + 380 stale + 2 contention
Reconnect encoder:   2115 = 2106 accepted + 9 pressure
Reconnect transport: 2106 = 1908 admitted + 160 disconnected + 38 resync
Admitted:            1908 = 1907 ACKed + 1 unconfirmed
Received:            1907 = 401 before termination + 1506 after reconnect
```

All 2,106 Host-encoded outputs independently decode and match their source
nonce/counter/PTS, including outputs intentionally not transmitted. The first
receiver's 401 frames retain length/CRC/header/ACK records; its optional AU capture
was disabled for the planned forced termination. They are not claimed as a
separate independently decoded receiver capture. All 1,506 captured post-reconnect
AUs are byte-identical to the corresponding Host AUs and independently decode
with matching source markers/timestamps. Normal receiver captures prove the same
for all 1,782 frames. Content proof covers sampled marker regions, not exhaustive
whole-frame pixel equality.

The receiver was forcibly terminated while the same Host/source remained active.
One attempted send became unconfirmed (frame149500, absent from received ledgers);
four expected socket errors record reset10054 and three connection timeouts10060.
No protocol errors occurred. After receiver restart, a different random session
performed HELLO/CAPABILITIES with sequences1/2 before FRAME3. The new stream starts
at frame149735 with IDR+SPS+PPS; 38 dependent AUs were explicitly suppressed after
READY. First validated recovery FRAME arrived 805.5547ms after new READY. No old
session FRAME or partial bytes were replayed. Driver/helper and Host process were
retained across receiver recovery; the source epoch stayed unchanged.

Both final DRAIN/DRAIN_ACK transcripts and natural process exit codes pass. The
fixed receiver-first shutdown branch was exercised in the real reconnect run,
waited within its bounded grace, and then recorded Host exit0. The first receiver's
planned forced termination is distinct from final clean shutdown.

Hardware remains AMDh264Encoder on Radeon610M, hardware-only asynchronous
D3D11-aware MFT, DXGI device manager and actual NV12 GPU surfaces. Software
fallback and normal-path full-frame CPU readback remain absent. Target config is
2400x1080/nominal60/30Mbps with uncapped unique-source submission; there is no
duplicate/interpolated-frame mechanism. **60 unique FPS is not proven.**

No growing transport backlog or latency accumulation is observed. Normal encode
10-second-bin p95 stays about8.1–8.3ms. Reconnect's first bin p95 is33.1ms, then
later bins settle to8.3–8.6ms; this is not progressive accumulation. The cause of
that initial scheduling interval is not established. After startup, normal Host
private bytes stay87,240,704; reconnect ends85,909,504, flat over its final four
samples. Post-reconnect receiver stays884,736 bytes/83handles. Driver returns to
35,598,336 bytes/931handles after both runs; same process throughout. Observed
Host handles settle around1283–1286. These short tests do not establish a long
transport soak or exhaustive leak freedom. Video Codec0 utilization samples:
normal mean16.43%/max19%, reconnect mean16.44%/max21% (7/9 samples, respectively).

All seven before/during/after health snapshots show PnP0/0, Secure Boot ON,
HVCI ON and TESTSIGNING OFF on the same boot. Relevant crash/bugcheck events0.
Old failure/UNKNOWN evidence and known-good binaries are preserved. PHASE3B
remains VERIFIED under its bounded-admission scope. Hard stop before3D: no
visible rendering, USB/WinUSB, phone work, deployment/security changes or publish.

## Historical checkpoint before final acceptance — PARTIAL (preserved)

The owner manually ran `flow-content-2`. Its 35.0267594-second normal
stream produced 1,792 real encoded/received/ACKed AUs; supplemental independent
decode/source/byte/sequence review passes for all 1,792. Controller incorrectly
treated normal receiver-first exit after DRAIN_ACK as failure and recorded a
forced Host stop. That execution remains ERROR, with clean process exit unverified.
The controller race is fixed and nine process cases pass in PowerShell 7 and
Windows PowerShell. Fresh normal/reconnect acceptance is pending under new names
`flow-content-3-*`. No live rerun, driver/security change or PHASE 3D work was
performed during this diagnosis. Earlier UAC cancellation and failures are preserved.

Protocol and bounded localhost endpoint are implemented. Earlier deterministic
tests and synthetic transport controls pass. Real hardware-encoded Display 3
content traversed the protocol and independently decoded for a completed prefix.
The requested 30-second live run stopped on E/UNCLASSIFIED source content, so
complete-run acceptance, clean end-to-end shutdown and real receiver reconnect
were NOT YET TESTED successfully at that checkpoint. PHASE 3D has not started.

PHASE 3B remains VERIFIED under its documented bounded-admission scope. No old
run is relabelled; the original PHASE 3A historical soak remains UNKNOWN.

## Implementation and deterministic gates

Protocol 1.0 has a fixed 48-byte header, explicit little-endian fields, bounded
metadata validation before allocation, HELLO/CAPABILITIES, fresh sessions and
strict directional sequences. See PROTOCOL.md for the complete wire contract.
Transport has three pending compressed AUs plus one in flight. Socket work is
off the encoding thread. Recovery suppresses dependent AUs until IDR/SPS/PPS;
all suppression/loss outcomes are explicit. Simulator is a protocol endpoint,
without visible rendering. The optional Host hook follows real hardware output;
the existing GPU-native BGRA-to-NV12 and AMD H.264 path remains in use.

All protocol, endpoint, fixture and Host builds pass `/W4 /WX`. Deterministic
protocol tests pass **58,132 checks**, covering fragmentation/coalescing,
truncation, malformed fields/lengths, overflow, maximum allocation, negotiation,
sequence/session violations, CRC/source association, queue bounds, reconnect
parser reset and IDR resynchronization. Includes a seeded 20,000-mutation corpus;
this is not a claim of exhaustive fuzzing or memory-sanitizer coverage.

Synthetic fixtures use NAL-category payloads, not real video acceptance:

| Result | Normal fixture | Slow receiver fixture |
|---|---:|---:|
| Encoded-like inputs | 400 | 400 |
| Admitted | 370 | 120 |
| Disconnected / resync / overflow | 2 / 28 / 0 | 92 / 178 / 10 |
| ACKed / queue-aborted / unconfirmed | 370 / 0 / 0 | 80 / 30 / 10 |
| Fully sent / received-validated | 370 / 370 | 90 / 90 |
| Pending queue peak | 1 | 3 |
| Sessions / reconnects | 1 / 0 | 11 / 10 |
| Protocol errors | 0 | 0 |
| Final receiver drain | Passed | Passed |

All 10 slow-fixture unconfirmed frames exist in the receiver ledger; their ACKs
were not confirmed by Host. They are not 10 lost receiver frames. Every new
session begins at IDR/SPS/PPS. Both fixture verifiers pass exact accounting.

## First real run: preserved failure and completed-prefix proof

Private evidence: `phase3c/live-normal-1`. Requested 30 seconds, 2400x1080,
nominal 60-FPS encoder configuration, 30 Mbps and uncapped submission; actual
source was the existing SweetDisplay desktop. No frames were duplicated.
Run started at 14:26:40 UTC. Host stopped with:

```
Host exit=1; SweetDisplayHost ERROR: E: UNCLASSIFIED content;
bounded evidence frozen; code=13 (0x0000000D)
```

The source pattern counter regressed **2082 -> 2062**, with expected nonce still
matching. Producer published counter was 2089. Classification totals are
**A474 / B0 / C0 / D0 / E1**. Invalid samples at the failure snapshot: 0.
Independent desktop reference hashes did not corroborate the sampled crop;
two later reference samples also differed. The retained crop visually contains
intact pattern rows, which does not establish contemporaneous compositor identity.
The owner is unsure whether desktop/window actions occurred. This answer does
not change E to B/C and is not proof of pipeline corruption. Cause is **UNKNOWN**.
Counter regression alone is insufficient to demonstrate corruption, but absent
corroboration it cannot be silently accepted. Existing fail-fast semantics held.

The failed frame was not submitted to the encoder. Independent verification of
the preceding closed ledgers and actual captured bitstreams gives:

| Completed-prefix measurement | Result |
|---|---:|
| Submitted / encoded / transport-admitted | 474 / 474 / 474 |
| Fully sent / received-validated / ACKed | 474 / 474 / 474 |
| Independently decoded, Host and receiver captures | 474 each |
| AU bytes, byte-identical captures | 29,628,028 |
| Source ID/QPC/PTS/nonce/counter association | All 474 match |
| Visible / coded aperture | 2400x1080 / 2400x1088 |
| Receiver first-to-last frame span | 15.4004375 s |
| Inter-frame receiver cadence, (count-1)/span | 30.713413 FPS |
| Payload bitrate over that span | 15.390746 Mbps |
| Transport drops / resync skips / pending | 0 / 0 / 0 |
| Reconstructed pending/outstanding peak | 1 / 1 |
| Transport latency p50/p95/p99/max | 0.3412 / 0.6127 / 0.7101 / 0.9324 ms |
| Encode wall latency p50/p95/p99/max | 25.2610 / 33.5176 / 34.9469 / 36.0169 ms |
| IDR / SPS / PPS-bearing outputs | 4 / 4 / 4 |

Live encoder calls identify `AMDh264Encoder`, CLSID
`{ADC9BC80-0F41-46C6-AB75-D693D793597D}`, on Radeon 610M; hardware-only MFT
activation, asynchronous/D3D11-aware negotiation and NV12 GPU-native path passed.
The log explicitly records software_fallback=0 and submission limiter disabled.
The measured prefix is slower than previous uncapped PHASE 3B runs. This failed,
short run is not sufficient to attribute the difference to source scheduling,
encoding or the new callback/evidence work. Performance regression exclusion is
still unresolved; byte preservation alone does not prove unchanged performance.

Latency is same-PC sender header time to receiver parse/CRC completion. It is
not encode or visible display latency. Prefix cadence is not a full-run FPS or
60-FPS claim. Source-at-failure snapshot has 827 frames, 4 producer replacements,
348 stale drops, busy snapshot 0, 0 invalid and shared high-water 3. The separate
cumulative contention counter is 56; do not call that a zero-contention run or
mix its lifetime total into session drop accounting. There is no
completed source/encoder final summary; do not infer full-run exact accounting
from that asynchronous snapshot. Exact transport accounting above applies only
to the completed 474-frame prefix.

Both real AU files pass the existing independent H.264 decoder/verifier and are
byte-identical by SHA256. Their decoded frame/source metadata and content marker
records match. `completed-prefix-verification.json` reports
`PASS_COMPLETED_PREFIX_ONLY`, explicitly retaining the original ERROR. Missing
encoder/session/transport final results were not fabricated.

## Shutdown, health and remaining gates

Host exited 1 naturally; pattern exited 0 without forced termination. Receiver
did not finish graceful drain after Host failure and was terminated by cleanup
after its wait (exit 0xFFFFFFFF). Therefore **clean end-to-end shutdown did not
pass**. Real receiver termination/reconnect was not attempted after this failure.
Synthetic recovery does not substitute for that live gate.

Before/after health: adapter/monitor problem codes **0/0**, Secure Boot ON,
HVCI ON, TESTSIGNING OFF; same boot, retained driver/helper, zero selected
relevant crash/bugcheck events. Three runtime resource samples per process span
only about 12.8 seconds, including startup: Host private bytes 33,783,808 to
85,143,552 and handles 982 to 1272; receiver private bytes 798,720 to 864,256 and
handles 68 to 68; driver private bytes 35,643,392 to 35,643,392 and handles 931 to
934. These are first/last observations, not leak or plateau findings. Resource/GPU
observations are retained privately; the short failed run establishes no
sustained resource/latency stability claim.
No driver, certificate, trust, boot/security, phone or USB state was changed.

After stopping live work, independent-clock receiver telemetry was corrected
to mark cross-clock latency unavailable. Separate `*-review` binaries compile
successfully; original live-tested binaries were preserved. The review variant
is NOT YET TESTED live and does not alter this run's evidence.

Remaining acceptance under the clarified policy below: complete a fresh real
run with independent content/transport verification, exact accounting and clean drain, then real
receiver disconnect/reconnect with fresh handshake and IDR/SPS/PPS recovery.
Do not weaken A–E semantics, claim 3C VERIFIED from a prefix, or proceed to 3D.

Final preservation check: SHA256 matches for 1,231 prior evidence files, five
previous binaries, 23 driver/shared source files and three live-tested 3C
binaries. Public documents describe only sanitized summaries; private evidence
and build outputs remain ignored. No publication was performed.

## Follow-up: independently review the boundary and fix transport acceptance

Private review: `phase3c/preflight-2/historical-region-review.json`. The final
five transported source frames 120009, 120011, 120013, 120014 and 120015 match
wire sequences 487..491 in the same session, with matching AU length, CRC,
source QPC, PTS, nonce/counter, ACK and independent decoded output. No duplicate
or replayed FRAME sequence exists in the 474-frame ledger. Captured AU files
remain byte-identical. Original FRAME-only logs lack complete heartbeat-header
records; exact reconstruction of every historical message is not claimed.

Failure frame 120018 is a **source-content counter regression observed before
encoding**. No AU, protocol FRAME or ACK exists for it, and there is no acquired
frame after it: fail-fast ended the run. It is therefore not an observed protocol
sequence/ACK failure or an observed damaged/replayed transported AU. Its source
provenance is still unclassifiable: neither legitimate compositor behavior nor
pipeline corruption is established. Historical ERROR/E1 remains unchanged.

Only the PHASE 3C Host enables `TransportContentPolicy`: valid pattern with
sole reason=4 (counter regression) remains E/UNKNOWN and increments a separate
`transport_counter_only` count while encoding/transport may continue. Acceptance
still requires exact source-to-decoded marker/PTS association, valid frame/clock
metadata, strict session/message sequence, lengths, CRC and ACK. All D and other
E reasons remain fatal. The legacy PHASE 3A/3B verifier and shared ABI are unchanged.

New `content-v2` output directories preserve every previous executable. New
tests explicitly accept payload counter 2082 -> 2062 with increasing wire
sequence, reject duplicate/regressed/replayed wire sequence, and ensure legacy
mode/D/other E remain strict. **58,155 checks PASS**; all builds `/W4 /WX` pass.
Both endpoints now record bounded complete TX/RX message headers, including
handshake, heartbeat and cumulative ACK, enabling independent full-sequence review.

`suite-content-2` normal fixture: 400 inputs = 370 admitted + 1 disconnected +
29 resync; 370 sent/received/ACKed, pending queue peak 1, outstanding peak 1.
Slow fixture: 400 = 120 admitted + 92 disconnected + 178 resync + 10 overflow;
120 = 80 ACKed + 30 queue-aborted + 10 unconfirmed; 90 fully sent/received,
pending queue peak 3, outstanding peak 4, 11 sessions/10 reconnects. Both pass
independent header-sequence/handshake/ACK/source/CRC and exact accounting review.
These are synthetic fixtures, not a substitute for live H.264 acceptance.

Prepared controller `Invoke-Phase3CValidation.ps1 -RunName flow-content-2` runs
35-second normal streaming, actual independent Host/receiver decode and all
verifiers before allowing 45-second receiver termination/reconnect. Its first
receiver is stopped while Host stays active; the new receiver must handshake
with a fresh session and resume at IDR/SPS/PPS. Captured post-reconnect AUs are
compared byte-for-byte and independently decoded. Per-run names are unique and
existing directories are rejected. The controller performs read-only PnP/security
preflight; it never installs drivers or changes system trust/boot configuration.

The launch error was exactly **"İşlem kullanıcı tarafından iptal edildi."**
Windows consent was observed pending, then exited. No controller/run directory
was created; fresh live health, duration, decode, shutdown and real reconnect
remain NOT YET TESTED. Non-elevated PnP inspection also returned Access denied;
that is not evidence of device failure. Last completed live health remains the
earlier 0/0 / Secure Boot ON / HVCI ON / TESTSIGNING OFF snapshot.

Follow-up final preservation PASS: 1,231 prior evidence files, five original
binaries, 23 driver/shared files and 108 prior 3C evidence/build files match
their SHA256 snapshots. Hygiene PASS: 142 publishable files, index and 125
history blobs checked. Six PowerShell scripts parse; whitespace check passes.

## 2026-09-19 — controller shutdown race, preserved real stream

The manual `flow-content-2-normal` execution reports "Receiver exited before
Host drain". Receiver exit is 0 and receiver-result says drained=true. Both
protocol ledgers end with matching type-5 DRAIN/DRAIN_ACK at sequence 1829 after
1,792 exact FRAME ACKs. Sender transport-result has pending=0 and all 1,792
admitted AUs ACKed. Host/encoder final artifacts also exist. Thus the receiver
did not exit before protocol drain: it exited before the controller observed
Host process completion. The unconditional early-exit branch then recorded
HostForcedStop=true. No successful Host exit code was retained. This is a
controller ordering bug, not a justification to retroactively certify clean exit.

Supplemental evidence is isolated in `phase3c/shutdown-review-1`; all 43 original
controller/run files remain SHA256-identical. Two unchanged independent decoder
invocations read the original Host/receiver AU files and write only separate
review outputs. Transport and source/encode reviews explicitly mark
FullRunAccepted=false and preserve original execution ERROR.

| Artifact measurement | Verified value / scope |
|---|---|
| Host observation duration | 35.0267594 s |
| Source | 1,974 / 56.356912 FPS |
| Host | 1,798 / 51.332182 FPS |
| Submitted / encoded / received / ACKed / decoded | 1,792 each |
| Encoded rate, using Host observation duration | 51.160885 FPS |
| Source accounting | 1,974 = 1,798 Host + 175 stale + 1 ready at end |
| Encoder accounting | 1,798 = 1,792 accepted + 6 pressure + 0 rate drops |
| Transport drops / resync / unconfirmed / pending | 0 / 0 / 0 / 0 |
| Shared / encoder / transport queue peaks | 3 / 1 / 1 |
| Classification | A1798 / B0 / C0 / D0 / E0 |
| AU bytes | 112,011,442; all captured sender/receiver bytes identical |
| Actual encoded bitrate | 25.583056 Mbps |
| Encoder latency p50/p95/p99/max | 6.7 / 8.2 / 10.0 / 19.4057 ms |
| Transport latency p50/p95/p99/max | 0.3864 / 0.5660 / 0.6060 / 0.9138 ms |
| IDRs | 15 |
| Hardware path | AMD hardware-only, D3D11/NV12 GPU surface, no software fallback |
| Source/decoded marker and PTS association | 1,792 / 1,792 pass |
| Handshake/message sequence/CRC/ACK correspondence | All recorded messages pass |
| Before/periodic/after health | PnP0/0, SecureBootON, HVCION, TESTSIGNINGOFF |
| Relevant crash/bugcheck events | 0 |
| Pattern / receiver exit | 0 / 0 |
| Host process clean exit | NOT VERIFIED; controller recorded forced stop |
| Real receiver reconnect | NOT YET TESTED; controller stopped before that stage |

Pressure6 is an explicit encoder admission outcome; it is not a transport drop.
Receiver's own startup-inclusive FPS has a different denominator and must not be
substituted for the Host observation rate. These short artifacts do not prove
60 unique FPS or long-term resource stability.

Fix: a receiver-first exit is accepted as a possible normal completion only
when its native exit code is0, final receiver evidence says drained, and the
terminal transcript contains DRAIN/DRAIN_ACK. Controller then gives Host at most
5 seconds to exit. The existing native Host exit-code check remains mandatory;
early failure, missing ACK, protocol error, nonzero Host exit and hung Host still
fail. No binary, wire format, admission/queue or driver change was needed.

Nine isolated process tests cover receiver-first, both already exited, receiver
error, missing final result, missing drain, protocol error, missing ACK, hung Host
and nonzero Host exit. All pass in PowerShell7 and Windows PowerShell5.1. Initial
sandbox Windows PowerShell invocation was policy-restricted; identical unmodified
script succeeded with normal host filesystem/process access. No ExecutionPolicy
flag, UAC/RunAs, registry/security change or live device access was used for these
tests. The 58,155 protocol checks also pass again.

Next controller command uses `-RunName flow-content-3`: unique35s normal run,
independent decode/full verification, then only on success45s real receiver
disconnect/reconnect and fresh IDR/SPS/PPS recovery verification. It preserves
all `flow-content-2` files and still stops on the first failure.
