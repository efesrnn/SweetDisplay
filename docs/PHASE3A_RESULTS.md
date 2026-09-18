# PHASE 3A — VERIFIED, 2026-09-16

The fresh `classified-flow-1-soak-observe` run passed the corrected A–E
acceptance rules, independent evidence verification and resource review.
The controller completed normally in the background; no replacement soak or
supplementary live test was needed. PHASE 3B has not started.
The original `phase3/soak-1` remains historically **UNKNOWN**, not VERIFIED.

## Completed live measurements

All sessions used actual SWT0001 desktop frames, 2400x1080 BGRA8, the unchanged
PHASE 2 driver and three shared D3D11 texture slots. No frames were fabricated
or duplicated to increase the rate. Repeated compositor content is not counted
as a new distinct pattern counter.

| Metric | Five-minute observation | Fresh main soak | Fresh-process soak reconnect |
|---|---:|---:|---:|
| Duration, seconds | 300.0063274 | 1810.0007551 | 15.0017173 |
| Source acquisitions | 16,456 | 100,898 | 832 |
| Received / acknowledged | 16,435 | 100,749 | 790 |
| Source FPS | 54.852176 | 55.744728 | 55.460317 |
| Host FPS | 54.782178 | 55.662408 | 52.660638 |
| A: valid visible pattern | 16,435 | 100,749 | 790 |
| B / C / D / E | 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 |
| Classification transitions | 0 | 0 | 0 |
| Producer replacements | 0 | 29 | 34 |
| Host stale drops | 1 | 14 | 8 |
| Contention drops | 20 | 106 | 0 |
| Busy / invalid / pending / held at end | 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 |
| Queue high-water | 2 | 3 | 3 |
| Clean disconnect | yes | yes | yes |

Exact main accounting: **100,898 = 100,749 + 29 + 14 + 106**.
Exact reconnect accounting: **832 = 790 + 34 + 8**.
Main IDs advance from 429,993 to 530,890; fresh Host starts at 530,896 and ends
at 531,727, preserving driver epoch. Slots are exactly 0, 1, 2 (maximum index 2,
capacity 3). Frame/presentation IDs, QPC ordering, geometry, resource association,
acknowledgements and per-frame classification agree across raw logs and reports.
No D integrity failure, E unclassified event, invalid metadata, image dump or
unaccounted frame loss occurred. B/C were not exercised naturally during this
soak: the separate controlled pilots verified B488 and C474 respectively, with
no D/E. See [CLASSIFIED_ACCEPTANCE.md](CLASSIFIED_ACCEPTANCE.md).

The observation also passed its own 15.001639-second reconnect with 823 received
frames. Main Host, fresh Host and producer-window exits were all 0. The producer's
PID-checked window close succeeded without forced termination in both stages.
Only the existing SweetDisplay device helper remained after the tests.

## Timing and achievable rate

The main soak contains 100,745 distinct valid pattern counters: **55.660198/s**.
60 Hz display mode remains VERIFIED; sustained 60 unique FPS remains unverified.
The previous ~38.5 FPS was workload-limited, not a demonstrated pipeline ceiling.
Earlier no-Host and metadata controls locate the dominant cadence shortfall
upstream of shared-texture handoff; the exact compositor scheduling cause remains
SUSPECTED. This new soak verifies stability at the measured achievable rate,
under the owner's corrected gate that does not require exactly 60 unique FPS.

| Main-soak timing, ms | p50 | p95 | p99 | Maximum |
|---|---:|---:|---:|---:|
| Source QPC interval between received records | 16.7449 | 32.8303 | 34.3388 | 171.7668 |
| Host receive interval | 16.8781 | 32.0125 | 35.2156 | 171.9108 |
| Diagnostic sample-completion interval | 16.7316 | 33.8064 | 39.8728 | 186.9754 |
| Mutex acquisition wait | 0.0379 | 0.0618 | 0.0811 | 0.2357 |
| Source to sample completion | 3.5612 | 18.0483 | 22.8517 | 175.0242 |

Percentiles use nearest rank. Received-record intervals include dropped source
IDs; they are not a complete independent upstream-arrival trace. Mutex/sample
timings are not isolated full-frame GPU-copy execution times. GPU execution
timing and engine utilization were not separately measured in this new run.

## Resource and health review

After excluding the first 120 seconds, each role has 304 samples spanning about
125–1806 seconds. No continuing memory/handle growth was observed.

| Role | Private bytes min–max | End delta, bytes | Handles min–max / delta | CPU, % of one logical processor |
|---|---:|---:|---:|---:|
| Driver | 35,213,312–35,246,080 | +4,096 | 936–938 / 0 | 3.2783 |
| Host | 27,643,904–27,738,112 | -61,440 | 774–778 / -2 | 2.5154 |
| Pattern | 26,673,152–26,796,032 | -90,112 | 789–797 / -8 | 3.7197 |

Driver private-memory quarter medians are 35,241,984 / 35,241,984 / 35,246,080 /
35,246,080 bytes: one 4 KiB step followed by a plateau, not continuing growth.
Driver handle quarter medians are all 936; Host all 774; pattern 791/789/789/789.
Host/pattern memory medians flatten or decline. After cleanup driver handles
fall to 933; a later settled read shows 931 handles and 35,233,792 private bytes.
These observations support this test-duration acceptance, not an indefinite leak
proof. The tiny positive linear driver-memory slope does not override its plateau.

All 56 recorded before/periodic/after health snapshots have PnP problem codes 0,
Secure Boot ON, HVCI ON, TESTSIGNING OFF, CI flags 62465 and the same boot.
Selected driver/application/system failure events: 0. No crash/bugcheck was
observed during the interval. Driver DLL/INF/CAT and shared ABI hashes match
the PHASE 2 baseline. No deployment, boot, certificate, security or phone change
was performed.

## Evidence preservation and scope

The controller ended `TESTS_PASS_RESOURCE_REVIEW_REQUIRED`; stage result PASS,
execution PASS_CLASSIFIED. Independent replay with minimum duration 1810 and
required reconnect also passes. Original evidence was inventoried and hashed
before this final review; all 130 original files retained identical lengths and
SHA256 hashes afterward. New review outputs are separate in ignored
`phase3a1/classified-flow-1-review-1`; original run reports were not overwritten.

Acceptance combines protocol/resource invariants, producer-ledger checks and
sampled-region desktop corroboration. It does not certify every pixel outside
the sampled rows, provide a historical atomic compositor oracle, or prove the
cause of the old soak anomaly. No unexpected interval was discarded. Historical
strict-oracle evidence and its UNKNOWN diagnosis remain intact.

Repository hygiene passes: 106 publishable working-tree files, index and 50
reachable history blobs checked. No raw evidence was published or force-added.
PHASE 3A is VERIFIED within this scope. Stop here before PHASE 3B.
