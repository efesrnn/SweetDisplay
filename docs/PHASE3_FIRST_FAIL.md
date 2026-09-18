# PHASE 3A.1 — first-failure diagnosis

Historical first-failure investigation: **PARTIAL**. Historical soak cause: **UNKNOWN**.
The corrected A–E pilots and new five-minute PHASE 3A.1 observation are now
**VERIFIED**; see [CLASSIFIED_ACCEPTANCE.md](CLASSIFIED_ACCEPTANCE.md) and STATUS.
The fresh PHASE 3A.2 soak is VERIFIED; see [PHASE3A_RESULTS.md](PHASE3A_RESULTS.md).
PHASE 3B has not started. The earlier gates
and proposed next steps below record the investigation at that time, not current
authorization; the owner subsequently authorized the corrected observation/soak.
The previous 1,810-second failed soak remains failed, without filtering its
174 zero-content frames or the 16,148 → 16,122 regression.

Latest result: second cover test and live cleanup fix VERIFIED. The follow-up
foreground session stopped after 17.354111 s; its crop is byte-for-byte identical
to the minimize control. The owner confirms using Show Desktop during this
session. Together with Explorer foreground telemetry, this strongly corroborates
legitimate desktop-content replacement in the latest incident. It does not
retrospectively explain the old soak. No 5-minute observe session, repeat soak or
PHASE 3B implementation followed this stop.

## Authorized retry and short controls

The owner approved the second cover experiment and live cleanup verification.
All three controlled sessions below fail the content test intentionally; each
stops on its first bad row, retains exactly one small crop and disconnects.

| Run | First mismatch (source-relative) | Received rows | Observed content | Clean producer exit |
|---|---:|---:|---|---|
| controlled-cover-2 | 8.039831 s | 445 | nonce=0, counter=0; uniform RGB(48,48,48) | 0, no forced stop |
| controlled-snapshot-1 | 8.1830632 s | 449 | matching nonce; counter 1403 → 1374 | 0, no forced stop |
| controlled-minimize-1 | 8.2069614 s | 457 | nonce=0, counter=0; dark nonuniform crop | 0, no forced stop |

Each has one invalid row, last in the session, and a 120-row frozen history.
All are 2400×1080 BGRA8, slot 0, epoch 798362475321, queue peak 2;
independent frame/slot/generation and sample/crop hash checks pass.

Cover-2: frame/acquisition 405938, presentation 406196, source QPC 857988167935,
producer generation 857887031028. The crop matches the deliberately painted grey
overlay; control-to-acquisition delay 17.7856 ms. Host returns exit 1 / code 13.
The recorded producer HWND receives WM_CLOSE, records WM_DESTROY and exits 0.
The previous cleanup defect is now **VERIFIED fixed for this live control**.

Snapshot-1: frame 406655, presentation 406918, source QPC 858709091517,
producer generation 858606212609. The overlay deliberately painted nonce and
counter 1374 when the feedback counter was 1400. Three more counter increments
reached Host before composition showed the overlay, hence the actual regression
1403 → 1374, not exactly 26. The crop independently decodes the known painted
value, matching the ledger. Control-to-acquisition delay 15.8471 ms. This is
**VERIFIED controlled old content on the desktop**, not a stale-slot reproduction.
It does not establish the cause of the historical 16148 → 16122 transition.

Minimize-1: frame 407254, presentation 407521, source QPC 859372830460,
producer generation 859269959300. The first zero decode arrives 5.6719 ms after
control begin; window telemetry records minimized=1 before that acquisition.
The crop visibly contains a dark coloured gradient, not all-zero pixels. The
minimize transition and zero signature are VERIFIED; identifying the gradient
as a particular wallpaper/compositor animation is only SUSPECTED. The automatic
verifier conservatively leaves this crop UNEXPLAINED_CONTENT_FAILURE rather than
declaring a pipeline pass from window state alone. All three tests have selected
failure-event count 0, unchanged boot/security and both PnP problem codes 0.

## First unexpected short-session failure

`Test-FirstFailShortChecks.ps1 -RunPrefix short-1` planned a 30-second foreground
control followed by a 300-second observe run under one initial elevation. It
stopped in `short-1-foreground` on the first mismatch. `short-1-observe` was
**never started**. The control's SetForegroundWindow returned 0, so a successful
controlled foreground activation was **not verified**; no focus/security policy
was bypassed. The actual mismatch occurred 9.1995578 seconds after that failed
activation attempt, amid independently observed foreground changes.

| First-failure evidence | Value |
|---|---|
| Source-relative time | 17.354111 s |
| Frame / source acquisition / presentation | 408355 / 408355 / 408622 |
| Source / receive / mutex acquired / sample complete QPC | 860465781134 / 860465918064 / 860465918362 / 860466019908 |
| Epoch / slot / producer generation | 798362475321 / 1 / 860270814375 |
| Previous / observed counter | 2349 / 0 |
| Observed nonce | 0; expected nonce nonzero |
| Received / invalid content rows | 900 / 1; failing row is last |
| Source / producer replacement / Host stale | 921 / 0 / 21 |
| Ready / held at capture / queue peak | 0 / 1 / 3 |
| Geometry / display position | 2400×1080 BGRA8 / (4480,0), SWT0001 at 60 Hz |

Independent evidence checks pass for ordered IDs/QPC, stable generation, slot
resource identity, producer ledger on preceding valid frames and the first-bad-row
stop. The 120-row context includes successful earlier use of slot 1 before this
failure; there is no demonstrated slot/metadata association error. Exactly one
small crop was captured under the same keyed mutex lease as the failed sample.
Both row hashes match. The complete BMP is SHA-256-identical to the minimize
control's BMP, despite a new Host process and new shared-resource names.

At failure, window state says visible=1, minimized=0, cloaked=0, composition ON,
Default input desktop and unchanged mode/coordinates. Foreground identity maps
to Explorer in the post-test process-name lookup. The owner explicitly confirmed
Show Desktop during this session in response to the approximate local-time
question. This is strong corroboration of genuine desktop-content replacement,
not standalone proof that every potential pipeline fault is excluded. Window
visibility flags alone cannot certify that the pattern is present in the pixels.
No new capture or unrelated-app hook was used to reach this conclusion.

Raw outcome remains CONTENT_ORACLE_FAILURE, Host exit 1 / code 13. The automatic
verifier keeps UNEXPLAINED_CONTENT_FAILURE; a separate retained review records
DESKTOP_REPLACEMENT_STRONGLY_CORROBORATED and the owner's statement. This explicit
review does not suppress a bad frame, turn the session into a streaming pass or
relabel the historical soak. No transport corruption or decoder bug was proven.
Clean producer exit 0, no forced stop, both PnP problems 0, selected failure events
0, unchanged Secure Boot/HVCI ON and TESTSIGNING OFF. No test process remains.

## Measured controlled reproduction

`controlled-cover-1` reproduced nonce=0/counter=0 using an ordinary, deliberately
created opaque test window over the encoded cells. This establishes a concrete
legitimate-content mechanism, not the cause of the historical incident.

| Observation | Result |
|---|---|
| First mismatch after first received source timestamp | 8.1152188 s |
| Source acquisition / frame ID | 404593 |
| Presentation number | 404842 |
| Source QPC | 799112769902 |
| Host receive / mutex acquired / sample complete QPC | 799112787925 / 799112788157 / 799112834421 |
| Driver epoch / texture slot | 798362475321 / 0 |
| Producer generation | 799010777101 |
| Previous counter / observed counter | 1483 / 0 |
| Expected / observed nonce | nonzero session nonce / 0 |
| Control begin to source acquisition | 10.5326 ms |
| Received rows / invalid rows | 464 / 1, invalid row is last |
| Frozen preceding context | 120 rows including failure |
| Queue at failure / peak | ready 0, held 1 / 2 |
| Producer replacements / Host stale / busy / invalid | 0 / 0 / 0 / 0 |
| Dimensions / format | 2400×1080 / BGRA8 |
| Active mode / desktop position | 2400×1080 @ 60 Hz / (4480,0) |

Exactly one 656×96 BGRA crop at (24,24) was saved under ignored private evidence.
Visual inspection shows a uniform dark-grey rectangle. Independent BMP decoding
checks every pixel's RGB components are (48,48,48), matching the deliberately
painted window. The original two sampled rows and the crop's corresponding rows
have the same 64-bit hash. Both operations used the same texture while the same
keyed mutex lease remained held. The crop independently decodes nonce=0/counter=0.
The threshold decoder checks blue >200; grey, black and many other non-pattern
contents all decode as zero. Therefore zero decoded bits do not imply zeroed
texture memory or a black desktop frame.

The producer HWND/PID, foreground window and Default input desktop remained
identifiable. The failure snapshot reports the pattern visible, not minimized,
not cloaked, DWM composition enabled and unchanged SWT0001 topology. Ordinary
visibility/cloak flags do not establish that every pattern pixel is unobstructed.
Actual GPU pixel evidence and the controlled overlay event provide that distinction.

The received GPU crop is correct for this controlled compositor output. Frame IDs,
QPCs, generation and Host slot/resource association pass independent checks; no
metadata association bug is demonstrated. The physical IddCx source COM pointer
is not exposed by the unchanged driver ABI, so this is not a new end-to-end
physical-pointer trace. Resource names and Host pointers are retained privately.
The `contention_total=45` snapshot is the driver's cumulative counter; it must not
be reported as 45 drops during this 464-frame session.

The Host returned exit 1 / ERROR_INVALID_DATA 13 (0x0000000D):
`first content-oracle failure; bounded evidence frozen`.
This is the intended fail-fast result, not a streaming acceptance pass.

The first runner's `CloseMainWindow()` selected the control overlay instead of
the producer window. After a five-second timeout it terminated its own producer.
Consequently **clean producer shutdown was not verified in this run**. This is
a diagnostic harness defect, not evidence of a driver fault. The runner now posts
WM_CLOSE only to the producer HWND recorded by the Host, after checking its PID,
then checks exit status. Its first retry was cancelled at UAC before any process
ran. After renewed owner authorization, controlled-cover-2 and the two additional
controls above verified normal exit without forced termination.

Before/after checks: both SweetDisplay devices problem 0, no selected crash,
bugcheck or display-driver failure event, same boot, Secure Boot ON, HVCI ON,
normal signature enforcement ON, TESTSIGNING OFF. Driver deployment and shared
frame ABI were not changed. No phone action occurred.

## What is and is not established

| Candidate | Evidence/status |
|---|---|
| Normal window covers sampled pixels | VERIFIED for controlled-cover-1 |
| Black/zero-filled desktop or freshly cleared texture | Not this capture: RGB is 48, not 0; historical case UNKNOWN |
| Stale reuse, wrong slot, mutex ownership or metadata pairing | No bug proven; historical cause UNKNOWN |
| Renderer reset/recreation | No reset seen in this controlled run; historical cause UNKNOWN |
| Decoder implementation error | No bug proven: independent crop decoding agrees |
| Oracle assumption that pattern always stays unobstructed | Demonstrably insufficient to classify pipeline corruption |
| Historical counter regression | UNKNOWN; controlled old desktop content reproduces a regression, not its historical cause |
| Historical 174 zero frames | UNKNOWN; no image/transition evidence was retained then |

No pipeline repair or architecture redesign is justified by these results.
Matching a failure signature alone does not establish the same historical cause.

## Opt-in implementation and acceptance semantics

`SweetDisplayHost --first-fail --nonce HEX --output <private-directory>` retains
the existing metadata validation and two-row sample, then checks zero bits,
nonce mismatch, counter regression, unpublished counters, expired producer
ledger entries and render timestamps later than source acquisition. Duplicate
content can be legitimate; it is not counted as an additional unique presentation.
The diagnostic ledger is a separate bounded 4,096-entry mapping, not a driver ABI
or video transfer replacement. Producer identity/generation cannot silently alias
an existing mapping. A valid Host counter feedback field triggers only controlled
test-window experiments.

First invalid content freezes up to 120 metadata records and exactly one small
crop while still holding the sampled texture. It releases/acknowledges that frame,
disconnects and exits; it does not continue through bad content. The normal video
path does not gain a full-frame CPU readback. Capture is refused outside the
repository's private evidence tree or if it would overwrite first-failure evidence.

Acceptance is separated as follows without relaxing the streaming verifier:

- Every unexpected zero/mismatch/regression still terminates and fails the content
  test. No bad interval is discarded, no UNKNOWN case becomes PASS.
- `CONTROLLED_COMPOSITOR_REPLACEMENT` means this controlled crop matches known
  replacement desktop content. It is a diagnostic classification, not a soak pass.
- `CONTROLLED_OLD_CONTENT_ON_DESKTOP` requires the explicitly painted old counter,
  matching nonce and recorded control timing. Verified in controlled-snapshot-1;
  the historical soak remains unchanged and failed.
- `NO_ANOMALY_REPRODUCED` only reports no anomaly during a bounded test; never fixed.
- Unknown content remains `UNEXPLAINED_CONTENT_FAILURE` and blocks the next phase.
- Forced producer cleanup is reported separately and never satisfies clean-shutdown
  acceptance. Verifying a saved crop does not change the recorded run outcome.

`Verify-FirstFailEvidence.ps1` independently checks metadata order, resource-slot
identity, render ledger correspondence, first-failure position, bounded history,
BMP contents/hash and baseline health. The first run's forced shutdown omitted
its producer result file; its forensic verification explicitly used the current
machine's measured QPC frequency of 10,000,000 Hz. This does not supply missing
shutdown evidence. The original capacity/soak verifier is unchanged.

## Supported desktop observations

Only read-only state queries and notifications for our own windows are used:
foreground HWND/PID, IsWindowVisible/IsIconic, window coordinates,
[DwmGetWindowAttribute](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmgetwindowattribute),
DWM composition, DXGI present-test status, QueryDisplayConfig,
[WTSRegisterSessionNotification](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsregistersessionnotification),
[RegisterPowerSettingNotification](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerpowersettingnotification),
and device notifications. Input-desktop observation uses
[OpenInputDesktop](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-openinputdesktop)
with read-only rights; failures are recorded, never bypassed. Registration handles
are released. No unrelated application hooks, desktop switching, GPU reset,
security change or deliberate secure-desktop transition is used.

## Remaining bounded investigation

1. Cover, old-counter and minimize controls have run with clean shutdown verified.
   Do not infer old-soak cause from similar signatures.
2. The short foreground series stopped at its first unexpected mismatch. The
   planned 300-second observe run was not started. All 300/300/600-second
   uninterrupted sessions remain NOT YET TESTED. Longer testing cannot
   retroactively explain old data; no clean observation is claimed.
3. Any later controlled focus experiment must establish successful activation
   without bypassing Windows foreground/security restrictions. Do not classify
   SetForegroundWindow returning 0 as a successful test transition.
4. Inspect actual first-fail evidence. If historical cause remains UNKNOWN, stop;
   no blind 1,800-second repeat, encoding, protocol, simulator or touch work.

Build commands (tools/tests only; no deployment):

```bat
scripts\windows\Build-Phase3Tools.cmd D:
scripts\windows\Build-HandoffTests.cmd D:
out\host-tests\FrameHandoffTests.exe
```

After UAC approval, the bounded harness command is:

```powershell
scripts/windows/Test-FirstFailDiagnostic.ps1 -RunName controlled-cover-2 -Seconds 30 -ControlCase cover
```

Use a fresh run name if evidence already exists. Diagnostic evidence stays under
`docs/evidence/private/phase3a1/`; all raw identifiers, process addresses and images
are ignored. Publication hygiene must pass before any later commit/push.

Validation so far: Host/pattern/tools Release x64 build; 2,033 existing metadata/
queue checks, 16 new oracle/ledger checks and 13 strict capacity-verifier fixtures
pass. No pipeline bug was fixed, so no claim of rerunning the full PHASE 2 live
slow-consumer/crash/reconnect suite is made.

Retry closeout: three diagnostic script parsers and independent verification of
the four new failed-session evidence sets pass. Public hygiene: 97 publishable
files, index and 50 history blobs pass. All five first-failure crops, including
the initial cover run, remain ignored. Driver DLL/INF/CAT and frame ABI hashes
are unchanged. This retry added the short-series launcher and completed-session
evidence checks; no C++ video path, deployed driver or Windows setting changed.
