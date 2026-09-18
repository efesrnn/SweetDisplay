# Status — 2026-09-18

Statuses: VERIFIED / ASSUMED / SUSPECTED / BLOCKED / NOT YET TESTED.
PHASE 3 subphase summaries additionally use the owner's PARTIAL category.
VERIFIED records a measured result or explicitly identified source/owner evidence.
Runtime performance is not inferred from source compatibility or an advertised mode.

| Milestone | Status | Evidence / next action |
|---|---|---|
| Original Microsoft sample build | VERIFIED | Pinned upstream unchanged; EWDK 26100.6584, Debug x64, zero build warnings/errors |
| SweetDisplay driver and Host build | VERIFIED | PHASE 2 GPU handoff; Debug x64 driver, Release x64 Host; WDK/API/catalog checks and 2,033 metadata/queue checks pass |
| Development signing / installed driver | VERIFIED | Existing certificate reused; PHASE 2 driver oem83.inf active; monitor oem81.inf unchanged; PHASE 1 rollback retained |
| Windows indirect-display baseline | VERIFIED | Owner confirmed Settings Display 3; runtime API confirms active SWT0001 at 2400x1080@60 on separate desktop coordinates |
| PHASE 1: real IddCx frame reception | VERIFIED | 771 real frames / 19.996027 s; 38.507650 FPS; all 2400x1080 BGRA8; ordered IDs/QPC; zero ETW loss |
| PHASE 1: exactly one diagnostic frame | VERIFIED | One BMP; test nonce and paint counter 117 match Display 3/SWT0001 test pattern; visual inspection passed |
| 60 FPS sustained reception capacity | BLOCKED | New VERIFIED soak: Host 55.662408 FPS, distinct pattern 55.660198 FPS. Upstream target presentation cadence skips refresh intervals even without Host; precise scheduling cause remains SUSPECTED. PHASE 3A acceptance does not require exactly 60 FPS |
| PHASE 2: driver-to-host continuous frame transfer | VERIFIED | Complete acceptance-4 + independent verifier: 35 s at 38.77 FPS, reconnect 35 s at 38.60 FPS; bounded slow-consumer/crash/reconnect/security checks pass |
| PHASE 3A: capacity + soak | VERIFIED | Fresh 1810.0007551-second soak: 100,749 received, 55.662408 FPS, A100749/B0/C0/D0/E0, exact 149-drop accounting, queue peak 3, 15-second fresh-process reconnect and clean shutdown. Resource/health review passes; old soak UNKNOWN. See docs/PHASE3A_RESULTS.md |
| Historical soak investigation | PARTIAL | Original 174 nonce mismatches and counter regression remain historically UNKNOWN. Controlled cover/old-content tests establish legitimate compositor mechanisms; old evidence is not relabelled |
| Corrected A–E acceptance / PHASE 3A.1 | VERIFIED | Both B/C pilots and 300.0063274-second observation pass: A16435/B0/C0/D0/E0, source 54.852176 / Host 54.782178 FPS; exact accounting, queue peak 2, clean 15-second fresh-process reconnect, flat resource medians and unchanged health. Historical soak UNKNOWN |
| PHASE 3B: real hardware H.264 | VERIFIED | Latest bounded-admission scope: recovered1810.0008772s sustained run,source101797/56.241409FPS,Host93168/51.474008FPS,93069 submitted=encoded=decoded/51.419312FPS.99 explicit transient pressure drops; strict all-Host assertion FAIL preserved. Exact accounting,queue3/1,stable observed resources/latency. Clean654+509 and forced-reconnect1093 frames decode/content verified; latter1 explicit pressure drop. PnP0/0/security unchanged. Not lossless-all-Host or60FPS proof. See docs/PHASE3B_RESULTS.md |
| PHASE 3C: versioned transport | NOT YET TESTED | PHASE 3B report/stop gate reached; no transport work authorized or started |
| PHASE 3D: live device simulator | BLOCKED | Gated on encoding/protocol; implementation NOT YET TESTED |
| PHASE 3E: simulated touch | BLOCKED | Gated on working video simulator; implementation NOT YET TESTED |
| PHASE 3F: pipeline automated tests | PARTIAL | Existing PHASE 2 tests retained; 3A evidence verifier added; future codec/protocol/simulator/touch tests NOT YET TESTED |
| Phone daemon interface contract | NOT YET TESTED | Logical protocol contract follows actual implementation; hardware assumptions remain unverified |
| Phone hardware work | BLOCKED | Owner reports bootloader timer has about six days remaining; no phone commands or state changes |
| Repository publication hygiene | VERIFIED | Latest completed check: 121 publishable files, index and 50 history blobs pass. PHASE 3B raw evidence is private; 560 prior3A and302 prior3B files,five binaries,driver package/shared ABI and76 first-recovery artifacts unchanged by final SHA256 check; no push |

Current acceptance is defined in docs/CLASSIFIED_ACCEPTANCE.md. Both new live
classification pilots passed without D/E; legitimate B/C content no longer fails
pipeline integrity. The new 300-second observation and fresh-process reconnect
are VERIFIED. The same UAC controller completed the fresh 1810.0007551-second
soak, reconnect and clean shutdown in the background. Independent artifact
verification and resource review passed without another soak or live test.
PHASE 3A is VERIFIED at the measured achievable rate; 60 FPS is not proven.
The old soak remains UNKNOWN. See docs/PHASE3A_RESULTS.md. The latest attached
request authorizes PHASE 3B only. Recovered sustained hardware encode and both
reconnect paths are now VERIFIED under its bounded-admission criteria; explicit
99 sustained and1 post-crash admission drops prevent a lossless-all-Host claim.
All admitted outputs independently decode/content match. Early slowdown and
underlying transient token timing remain UNKNOWN; see docs/PHASE3B_RESULTS.md.
The final report/stop gate is reached. Do not start PHASE 3C.

Earlier investigation: second grey cover, old-counter overlay and minimize controls
all stopped on their first bad content with one small private crop per session.
The corrected PID-checked producer-window close is now VERIFIED: normal exit 0,
no forced kill, repeated in four sessions. Metadata/slot identity checks pass.
The follow-up short series stopped at 17.354111 s (frame 408355, slot 1); its BMP
exactly matches the minimize control. Explorer was foreground and the owner
confirms Show Desktop during the test. Legitimate desktop replacement is strongly
corroborated for that latest event; no pipeline bug is proven. Controlled foreground
activation itself returned 0 and is not VERIFIED. At that earlier checkpoint the
five-minute observe run had not started. The newer corrected observation above
supersedes that pending gate; historical soak cause remains UNKNOWN. No driver,
security or phone changes were made.
See docs/PHASE3_FIRST_FAIL.md for evidence, acceptance limits and next bounded steps.

PHASE 1 and PHASE 2 remain complete. The historical failed long test reached
30 minutes 10 seconds, but Host exited 1 with ERROR_INVALID_DATA (13 / 0x0000000D),
"integration validation failed; inspect report". Nonce mismatches cluster at
90.7196575–94.3163347 seconds; one counter regression occurs just before them.
Transient desktop dimming/overlay is SUSPECTED, not proven. The runner stopped
before its fresh-process reconnect. No later encoding/protocol/simulator work
was started. Do not bypass this failure or call the run VERIFIED.

Raw throughput: source 55.469001 FPS, Host 54.325356 FPS; 44 producer replacements,
1,996 Host stale drops and 30 contention drops, peak three slots. Source interval
p50/p95/p99/max: 16.7310 / 33.3764 / 33.9865 / 162.0360 ms. The 174 content
mismatches are additional acceptance failures, not queue drops. All metadata IDs
and source timestamps remain ordered and all geometry is 2400x1080 BGRA8.

Resource observations after warm-up show bounded variation, no continuing growth:
driver handles 933–935, returning to 928 after cleanup/settling; private bytes
also fall after cleanup. Both PnP classes remain OK / problem 0, selected failure
events 0, and boot/security state unchanged. Installed driver DLL/INF/CAT and
shared ABI hashes match the PHASE 2 backup. See docs/PHASE3_VALIDATION.md.
No background timer, scheduled phone work or automatic next phase was created.

PHASE 2 deployment history: Secure Boot/HVCI stayed ON, TESTSIGNING OFF and normal signature enforcement ON.
No boot/security setting or phone state was changed. No new certificate or trust
entry was needed for PHASE 2. Only the explicit frame-handoff source change
required a driver-package update; the existing monitor package was preserved.

The existing helper and installed deployment were retained during final tests.
Both device classes remained OK / problem 0; SWT0001 stayed 2400x1080@60. Host
clean disconnect and forced termination did not restart the driver or machine.
The final event query found zero matching driver/system failures. Exactly one
PHASE 1 BMP exists; PHASE 2 only sampled two small pixel rows in memory.

Detailed commands, measurements and corrections: docs/TEST_LOG.md.
Architecture, measurements and reproduction: docs/FRAME_HANDOFF.md.
Raw frame, local topology, certificate and process details remain ignored.
