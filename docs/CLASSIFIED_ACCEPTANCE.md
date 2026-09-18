# Classified PHASE 3A acceptance

The original 1,810-second soak remains failed with historical cause UNKNOWN.
These semantics apply only to newly executed `--classified` sessions. Legacy
`--first-fail`, its evidence and its stricter pattern-only verifier remain intact.
No PHASE 3B work is authorized by this document.

## Frame classifications

| Class | Meaning | Evidence needed / action |
|---|---|---|
| A | PATTERN VISIBLE AND VALID | Correct nonce, nonzero published counter, producer ledger timestamp not after acquisition, exact RGB black/white cells, no counter regression |
| B | LEGITIMATE NON-PATTERN DESKTOP CONTENT | Metadata/resource invariants pass; both independent current desktop reads exactly match the received sampled RGB rows |
| C | STALE BUT LEGITIMATE COMPOSITOR CONTENT | Valid known pattern counter below the session high-water counter, plus both independent current desktop reads match |
| D | PIPELINE/METADATA INTEGRITY FAILURE | Independently invalid geometry/format, packet/ID/QPC/presentation order, slot/resource/generation, ownership/ack, mutex/GPU operation or exact accounting; stop |
| E | UNCLASSIFIED / UNKNOWN | Unexpected content without adequate independent corroboration, including unavailable/mismatching reference; save one bounded crop and stop |

Nonce absence or counter regression alone cannot create class D. B/C never
override an independently detected metadata/resource failure. A regression does
not lower the session's pattern high-water counter. Repeated unchanged content
does not create fake frames or establish unique presentation rate.

The driver/Host ABI, three shared textures and one GPU copy are unchanged. The
new mode changes diagnostic acceptance, not normal frame transport. Outside the
classified mode the existing strict checks remain in effect.

## Independent content corroboration and its limits

Only on unexpected pattern content, the Host reads the same two 640-pixel rows
at display-local (32,48) and (32,96) using the documented screen-DC path:
[GetDC(NULL)](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdc),
[BitBlt](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt)
with SRCCOPY/CAPTUREBLT/NOMIRRORBITMAP, and
[CreateDIBSection](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createdibsection)
with GdiFlush before reading the bits. Active SWT0001 coordinates are resolved
through QueryDisplayConfig; the thread temporarily uses physical-pixel DPI
awareness and restores its previous context. No global setting changes.

Two successive reference reads must match every RGB byte of the held received
texture's existing diagnostic rows. GDI's unused alpha byte is excluded. Equality
is byte comparison, not just equality of hashes. Reference QPC start/end, RGB
hashes, coordinates and API errors are logged per classified frame. No reference
image is saved and no full-frame CPU readback is introduced. Handles/DCs/DIBs
are released on every path.

This corroborates the **sampled region** against independently observed current
desktop content. GDI does not supply the IddCx presentation ID and is not an
atomic historical compositor oracle. Dynamic content may have changed by the
reference read: disagreement stops as E, not automatically D. Unsupported capture,
topology uncertainty and unavailable data also stop as E. Identical samples do
not certify every pixel outside the diagnostic region, and this is not a claim
of full-frame corruption detection. A physical source COM pointer remains absent
from the unchanged driver ABI. Those limits must accompany acceptance claims.

Normal A frames retain producer-ledger validation and all existing transport
invariants. Exact RGB cell checks additionally reject threshold-only decoding as
sufficient pattern proof. The separate bounded producer ledger is diagnostic
metadata, never a replacement video transport.

## Bounded failure evidence and integrity checks

Every received frame is retained in bounded-duration CSV logs with IDs, source /
receive / mutex-acquired / sample-complete QPC, presentation, slot, epoch, resource
identity and A–E classification. No bad interval is removed. In-memory pre-event
history stays at 120 records; producer ledger stays at 4,096 records. Only the
first E/content diagnostic failure saves one 656×96 crop while holding the same
texture mutex; no continuous images. Unsafe metadata/resource failures stop before
accessing an invalid slot and may therefore have an error record without an image.

Source and contention totals in the existing driver are atomics outside its queue
lock. A momentary accounting mismatch is re-read at most ten times, 1 ms apart,
without discarding a received frame or editing counters. If it does not resolve,
stop with D. Final accounting must be exact; independent verification checks
source = acknowledged + producer replacements + Host stale + busy + invalid +
contention + pending. Connection/ack/geometry/ordering failures are not retried.
No classified-mode automatic device reconnect hides an interrupted session.

The test harness profiles Host/pattern/driver memory and handles every five seconds,
checks read-only PnP/security health every 30 seconds, closes its exact producer
HWND after checking its PID, and verifies exit 0. Observation/soak stages require
a separate fresh Host process for a further 15 seconds before producer shutdown.
The verifier checks that reconnect preserves driver epoch and advances source IDs.
Resource trends still require review; a test-duration pass is not a leak proof.

## Authorized sequence

1. Two 20-second control pilots must exercise B (grey cover) and C (old content)
   without E/D before relying on the new runtime classifier.
2. Run a 300-second observation plus fresh-process reconnect. Review classification
   counts, exact accounting, resources, shutdown, health and any transitions.
3. Only if that passes, run a fresh 1,810-second soak plus reconnect. No 60 unique
   FPS requirement and no duplicated frames. Stop on first E/D or health failure.
4. Mark PHASE 3A VERIFIED only if the new soak and resource review pass. Stop before
   PHASE 3B. The historical soak remains UNKNOWN regardless of new outcomes.

Scripts: `Test-ClassifiedPhase3.ps1 -Stage Pilot|Observation|Soak -RunPrefix <fresh>`
and `Verify-ClassifiedEvidence.ps1`. Raw evidence stays under ignored private
phase3a1 storage; old and new runs have different names and are never overwritten.

Live pilots: VERIFIED. Cover A458/B488/C0/D0/E0; old-content A461/B0/C474/D0/E0.
Both have exact accounting, bounded queue (peak 3), no saved image, clean shutdown
and unchanged security/PnP health. 17 classifier tests and 11 classified-evidence
fixtures pass. After an earlier cancelled UAC launch, the one-UAC controller ran
the five-minute observation successfully: 300.0063274 s, A16435/B0/C0/D0/E0,
16,456 source / 16,435 acknowledged, source 54.852176 / Host 54.782178 FPS.
Queue peak 2; 1 Host stale + 20 contention drops, nothing pending. A separate
15.001639-second Host reconnect passed with A823 only. Post-warmup resource medians
were flat, slopes nonpositive, and security/PnP health unchanged. This new PHASE
3A.1 observation is VERIFIED. The fresh 1810.0007551-second soak also passed:
A100749/B0/C0/D0/E0, source 55.744728 / Host 55.662408 FPS, exact 149-drop
accounting and queue peak 3. Its fresh-process reconnect, clean shutdown and
resource/health review passed. PHASE 3A is VERIFIED; historical soak UNKNOWN.
See [PHASE3A_RESULTS.md](PHASE3A_RESULTS.md) for full metrics and scope.

The optional `Test-ClassifiedObservationAndSoak.ps1` performs the two remaining
fixed stages under one initial UAC. The agent must review the completed observation
report/resources and write a report-hash-bound continuation decision within five
minutes. This is a work-review gate for an already authorized test, not permission
to change Windows configuration. An absent/negative/stale decision stops before
soak. The observation/evidence-review/soak-start transition has now been exercised;
both stages and final independent resource review have now passed. No new soak
was started during the final artifact review; original evidence is preserved.
