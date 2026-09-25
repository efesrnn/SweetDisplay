# PHASE 3D — VERIFIED

## Final acceptance: `visual-flow-3` (VERIFIED)

The owner reran the unchanged asynchronous-v3 acceptance under a new identity so
the visible behavior could be observed directly. The owner confirms moving real
Display 3 content in the normal simulator window and confirms that a fresh window
resumed moving content after the planned receiver close/restart. The gated
controller and every original verifier report `PASS`; reconnect ran only after
the >=35-second normal stage passed. No acceptance limit changed.

Independent review is in `visual-flow-3-review-1`. It freshly decoded the normal
Host and receiver streams (1,278 each), the reconnect Host stream (1,568), and
the captured fresh-receiver stream (848). Encode/source association, complete
wire/ACK/CRC and AU byte identity, decoded/rendered source association, exact
drop accounting, bounded queues, resource guards, health and shutdown all pass.
All 167 original `visual-flow-3` files remain byte-identical after review, and
all three retained simulator copies equal the tested async-v3 binary.

| Metric | Normal live acceptance | Real reconnect acceptance |
|---|---:|---:|
| Duration | 35.0282436 s | 45.0069927 s |
| Unique source | 1,729 / 49.360168 FPS | 2,105 / 46.770510 FPS |
| Host | 1,280 / 36.541941 FPS | 1,569 / 34.861249 FPS |
| Encoded | 1,278 / 36.484844 FPS | 1,568 / 34.839031 FPS |
| Source classification | A1,280; B/C/D/E0 | A1,569; B/C/D/E0 |
| Encoder latency p50/p95/p99/max | 7.9 / 10.6 / 21.2 / 27.4905 ms | 8.0 / 9.9 / 21.6 / 45.8541 ms |
| Bitrate / IDR | 18.244289 Mbps / 11 | 17.421302 Mbps / 14 |
| Hardware path | AMD H.264 MFT, GPU NV12; Microsoft H.264 MFT D3D11 decode; no fallback | same, reinitialized in fresh receiver |

Normal source accounting is 1,729 = 1,280 Host + 11 producer drops + 436 stale
Host drops + 2 pending at source shutdown; contention is zero. Encoder accounting
is 1,280 = 1,278 output + 2 pressure drops;
rate-limit zero. Transport is 1,278 encoded = sent = ACKed = received. Receiver
accounting is 1,278 = 1,195 admitted + 1 overflow + 82 resync skips;
1,195 = 1,192 submissions + 3 reset discards; 1,192 = 1,191 decoded + 1 decoder
reset discard; all 1,191 decoded frames were accepted Presents. All 64 retained
render samples match the independently decoded source nonce/counter/PTS.

The normal overflow is measured and not hidden. Windows message 161
(`WM_NCLBUTTONDOWN`, consistent with title-bar/window dragging) held the UI
dispatch/pump for 1.520/1.523 seconds. The bounded
three-frame receiver queue overflowed once, 82 dependent H.264 frames were
explicitly suppressed, and valid presentation resumed at the next IDR after
2.1122849 seconds. Sparse diagnostic polling remained nonblocking (maximum
0.0208 ms). This is an explicit bounded continuity event, not metadata corruption,
software fallback, queue growth or an unexplained decoder stall. The owner still
observed changing content. Presentation span is 34.9250786 seconds; receive-to-
Present p50/p95/p99/max is 36.689/61.3065/73.9443/108.2944 ms outside the
separately reported recovery wait. Post-startup private bytes decreased by
47,136,768 and handles grew by 5, within the unchanged guard.

Reconnect sender accounting is 1,568 encoded = 1,281 admitted + 202 disconnected
+ 85 resync; 1,281 = 1,280 ACKed + 1 unconfirmed. The planned WM_CLOSE invalidated
the old session while Host remained active. The fresh receiver negotiated a new
session and received/decoded all 848 captured frames, accepted 847 Presents and
recorded one explicit busy Present. It had zero overflow,
zero resync skip, queues 2/2/1, 48/48 content matches, private bytes -47,562,752
and handles +1. It drained and exited cleanly.

The owner perceived approximately 15–20 seconds with no window. The QPC trace
measures 8.6608007 seconds from old receiver shutdown to the first new accepted
Present (8.7641222 seconds from the old last Present). This comprises 5.621373
seconds before the new process starts—the controller deliberately schedules a
three-second absence and performs a health query—then 0.1056042 seconds to create
the renderer, 0.4895134 seconds to finish the handshake, 2.4326214 seconds waiting
for the next valid SPS/PPS+IDR, and 0.1172929 seconds from IDR admission to the
first Present. The delay is material and remains a documented reconnect-latency
optimization; the acceptance contract required valid bounded recovery and visible
resumption, not an immediate-window or maximum-blackout threshold.

Before/after both runs: adapter/monitor Problem Code 0/0, Secure Boot ON, HVCI ON,
CI flags 62465/TESTSIGNING OFF, same boot, relevant failure events zero. Host,
final receiver and pattern exit 0; encoder drain/shutdown is clean. PHASE 3C's
58,155 protocol/regression checks and shutdown tests remain the last unchanged
source regression result; no source changed after them. Historical `visual-flow-1`
+36 handles and late overflow remain UNKNOWN. PHASE 3B and 3C remain VERIFIED.
PHASE 3D is VERIFIED with the measured rates and reconnect-delay limitation.
Hard stop: PHASE 3E was not started; no phone, driver, certificate, deployment,
Windows security, commit or push action occurred.

## Prior run: `visual-flow-2` technical acceptance passed; owner observation absent

The owner-launched `visual-flow-2 -AsyncDiagnostics` controller completed both
gated live stages. The normal 35-second stage passed first; only then did the
controller run the 45-second real receiver-close/fresh-process reconnect stage.
Controller outcome is `PASS_ARTIFACTS`. At that checkpoint PHASE 3D remained
PARTIAL because the acceptance contract still required the owner to confirm
visible motion in the normal window and visible recovery after reconnect. The
later `visual-flow-3` run above supplies that observation.

An independent second review was written under `visual-flow-2-review-1`. Fresh
offline decodes reproduced all 1,642 normal Host/receiver frames, all 2,084
reconnect Host frames, and all 1,244 captured fresh-receiver frames. Encoding,
wire/ACK/CRC, byte identity, decoded source association, rendering association,
resource, stage, health and shutdown checks pass. A SHA-256 manifest confirms
all 167 pre-existing `visual-flow-2` files remained byte-identical during review.

| Metric | Normal live acceptance | Real reconnect acceptance |
|---|---:|---:|
| Duration | 35.0073816 s | 45.0037786 s |
| Unique source | 1,924 / 54.959837 FPS | 2,445 / 54.328771 FPS |
| Host | 1,645 / 46.990090 FPS | 2,091 / 46.462765 FPS |
| Encoded | 1,642 / 46.904393 FPS | 2,084 / 46.307223 FPS |
| Source accounting | 2 producer + 277 stale; contention/pending 0 | 1 producer + 351 stale + 2 contention; pending 0 |
| Encoder accounting | 1,645 = 1,642 output + 3 pressure; rate 0 | 2,091 = 2,084 output + 7 pressure; rate 0 |
| Source classification | A1,645; B/C/D/E0 | A2,091; B/C/D/E0 |
| Encoder latency p50/p95/p99/max | 7.2 / 8.8 / 12.6 / 41.9571 ms | 7.6 / 9.2 / 10.1 / 41.7536 ms |
| Hardware path | AMD hardware H.264, GPU NV12, no fallback | same; fresh receiver reinitialized Microsoft H.264 D3D11 decoder |
| Bitrate / IDR | 23.454595 Mbps / 14 | 23.155981 Mbps / 18 |
| Queue peaks | shared3 / encoder1 / transport1 / receiver3 / decoder2 / render1 | same bounds |

Normal transport is lossless after encoder admission: 1,642 encoded = sent =
ACKed = received = independently decoded = GPU decoded = accepted Presents.
The presentation span is 34.8994229 seconds at 47.020835 active FPS. All 67
bounded rendered samples match the independently decoded AU and original source
nonce/counter/PTS. Receive-to-Present p50/p95/p99/max is
24.6746/59.6961/63.9601/88.2125 ms; longest accepted-Present gap is 77.5504 ms.
The unchanged post-startup resource gate passes: private bytes -46,936,064 and
handles +1. Receiver overflow/resync/reset/render drops are all zero.

The reconnect controller closed the first receiver by WM_CLOSE while Host stayed
active, then started a fresh process/session. Sender accounting is 2,084 encoded
= 1,769 admitted + 222 disconnected + 93 sender resync; 1,769 admitted = 1,768
ACKed + 1 explicitly unconfirmed. The old receiver accounts 524 received = 521
Presented + 2 queue-reset discards + 1 decoder-reset discard. The fresh receiver
accounts 1,244 received = 1,129 admitted + 1 bounded queue overflow + 114 explicit
resync skips; 1,129 admitted = 1,126 submissions + 3 queue-reset discards;
1,126 submitted = 1,125 decoded + 1 decoder-reset discard; 1,125 decoded = 1,124
Presents + 1 stale-generation suppression. There is no silent loss.

The one fresh-receiver overflow is localized to startup, not the removed sparse
diagnostic Map: frame 292104 arrived while the bounded three-frame queue was full
during the first recovered frame's 21.1798 ms render; the preceding cold decoder
`ProcessOutput` call took about 35.3122 ms. Sparse nonblocking poll max is
0.0149 ms and Present max is 1.9799 ms. The receiver rejected 114 dependent
frames and recovered on the next SPS/PPS+IDR, frame 292226, after 2167.2629 ms.
The fresh process then presented 1,124 frames over 24.6619929 seconds; 47/47
retained content markers match. Its unchanged resource gate passes with private
bytes -40,747,008 and handles +5. This is bounded, fully accounted reconnect
recovery, not a normal-run loss or integrity failure.

Before/after both stages: adapter/monitor Problem Code 0/0, Secure Boot ON,
HVCI ON, CI flags 62465/TESTSIGNING OFF, same boot, no relevant failure event.
Host, final receiver and pattern exit 0; encoder drains and shuts down cleanly.
The forced old receiver is the planned WM_CLOSE event, not forced cleanup.
Historical `visual-flow-1` +36 handles and late overflow remain UNKNOWN and all
of its evidence is preserved. PHASE 3B and 3C remain VERIFIED. Stop before 3E;
no phone, driver, certificate, deployment or security-setting work was done.

2026-09-20. The current worktree continues the protected VERIFIED PHASE 3C
baseline. The first live Display 3 run completed, but its resource gate failed.
Recorded playback and successful Present calls are component evidence, not a
substitute for the required fresh >=30-second live run and live recovery.

## Latest: live diagnostic reviewed; bounded sparse fix ready (PARTIAL)

The owner completed `visual-diagnostic-1` using the diagnostic-v2 binary. All
original files are preserved; supplemental checks and two independent decodes
are in `visual-diagnostic-1-review-1`. This was a diagnostic capture, not a final
PHASE3D promotion. The unchanged resource and rendering artifact gates pass.

| Metric | Measured live result |
|---|---|
| Source duration |35.0057902s|
| Unique source |1908 /54.505269FPS|
| Host |1673 /47.792094FPS|
| Encoded / received / independently decoded / GPU decoded / accepted Presents |1661 each /47.449293FPS over source duration|
| Source classification |A1673; B/C/D/E0; invalid0|
| Presentation span / longest gap |34.9128498s /70.1676ms|
| Accepted render marker correspondence |67/67 match independently decoded AU and source nonce/counter/PTS|
| Receive-to-Present p50/p95/p99/max |24.5237 /56.6302 /64.9943 /77.1391ms|
| Queue wait p50/p95/p99/max |7.6512 /14.7182 /15.5350 /50.7567ms|
| Sparse blocking Map p50/p95/p99/max |8.6971 /23.4635 /32.5475 /34.0170ms|
| ProcessOutput / Present call maximum |15.8100 /0.4170ms|
| Encoded queue / decoder pending / render bounds |2 /2 /1; shared3, encoder1|
| Receiver overflow / resync / reset / render drops |0 /0 /0 /0|
| Encoder |AMD hardware, GPU NV12;23.727071Mbps;14 IDR/keyframes; no fallback|
| Encoder latency p50/p95/p99/max |7.0 /8.6 /9.3 /29.3378ms|
| Decoder |Microsoft H264 MFT;1661 decoder-bound NV12 GPU surface proofs|

Exact accounting:1908source=1673Host+234stale+1contention; no producer drops or
pending source frames.1673Host=1661encoder accepted+12backpressure;rate-limit0.
1661encoded=1661sent=1661ACKed=1661received=1661independently decoded on each
endpoint=1661GPU decoded=1661Presented. Host and receiver AU bytes/CRC/SHA identity
match. No recovery-point wait occurred in this capture; do not assign it an
artificial zero-latency success for the historical loss case.

PSS first-successful-Present observation is935 handles; the steady plateau is938.
Type delta is ALPC-2,Event+2,IoCompletion+1,Thread+2 (net+3); other recorded types
are stable. The **unchanged** original +3s resource gate measures+1handle and
-46,870,528private bytes, passing +16 /32MiB. The separate five-second-to-end
comparison gives receiver handles0/private+126,976bytes, Host handles+1/private0,
driver handles0/private0. Receiver CPU is6.19% of one core over those samples;
Host21.45%. Receiver GPU combined VideoCodec0 mean6.57%/max8%,3D8.43%/10%,7samples;
dedicated VideoDecode1 is0 and is not relabelled as the combined counter.
First-frame private-memory allocation and later release are not a continuing
growth trend. The historical +36 and historical late-overflow causes remain
UNKNOWN; this run does not retroactively resolve them.

Before/during/after: PnP0/0,SecureBootON,HVCION,CI62465/TESTSIGNING_OFF, same boot.
Host/receiver/pattern exit0, no forced cleanup, no relevant failure events.

### Evidence-driven fix: diagnostic-v3 asynchronous 64-pixel sampling

Even the clean live capture retains a34ms synchronous diagnostic Map. Earlier
55.56FPS controls place cold/recreated-decoder overflow inside that wait.
The new v3 executable removes that measured blocking contribution without
changing H264, GOP, protocol, frame admission, decode or video queues.

The existing64x1 BGRA staging texture is one owned slot. It is copied at the
selected render, tagged with session/frame/sequence/source/PTS/generation and
actual Present result, then polled with `D3D11_MAP_FLAG_DO_NOT_WAIT` in later
loop iterations. A busy resource stays owned; it is never overwritten. An
occupied sampling slot records an explicit diagnostic skip, not a video drop.
No frame or H264 reference is manufactured. Normal video remains on GPU; only
64 diagnostic pixels can be read. The standalone historical ResourceProbe mode
retains its explicit legacy comparison branch; the v3 simulator always selects
the asynchronous path. Microsoft documents the nonblocking return behavior in
[ID3D11DeviceContext::Map](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-map)
and [D3D11_MAP_FLAG](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_map_flag).

Deferred samples have their own immutable CSV. `Read-VisualSamples.ps1` joins
them only after checking complete accounting and every identity/timestamp field
against the original render ledger. Old-session samples may describe a frame
already Presented before reset; they never trigger a new presentation. Shutdown
settles the bounded diagnostic slot before ACK/exit, with a finite deadline.
The main render ledger is never rewritten. Resource thresholds are unchanged.

Validation under new names:

- `async-components-1`: two sessions200decoded/196Presents (four intentional
  minimize render drops),18marker matches; WM_CLOSE59decoded/55Presents,6matches,
  exact close discards; corrupted SPS/PPS rejected. All PASS.
- `async-association-1`:13controls PASS, including generation/frame/PTS/result
  mismatch, duplicate/missing samples, invalid ready time, pending slot, wrong
  bound, blocking mode and missing evidence. These failures are not accepted.
- `async-stage-replay-1`: actual recorded receive cadence55.554831FPS;
  1700received=1700decoded=1700Presents, no overflow/resync/discards; queue1/2/1;
  60/60 sparse samples match the independent source oracle.161nonblocking polls,
  101not-ready returns, zero occupied-slot skips; Map-call max0.0099ms,
  render-call max12.6038ms. This is a controlled improvement, not live60FPS proof.
- `async-reconnect-1`: recorded socket reconnect PASS, new session and IDR;
  old84/new60Presents,6+4markers. Explicit sender50disconnected+103resync+
  1unconfirmed, receiver close/reset discards unchanged and fully accounted.
- `async-resource-1`: three renderer/nine decoder sessions,2660decoded/Presents,
  165/165 deferred samples matched. First long session settles938 handles;
  renderer cleanup257/258/258; final two cleanup private allocations decrease
  slightly. No continuing handle trend demonstrated in the measured interval.
- `phase3d-async-regression-1`:58,155 protocol tests plus normal/slow transport
  PASS; `phase3d-async-shutdown-1`:9 shutdown cases PASS. No3C source was changed.

v1 and v2 binaries are retained. v3 is built separately in
`out/device-simulator-visual-async-v3`; live selection requires `-AsyncDiagnostics`.
The next unused acceptance identity is `visual-flow-2`:35s normal, independent
decode/content/resources/timing gates, then45s real reconnect **only if all normal
gates pass**. The controller is prepared for manual Administrator PowerShell;
no automatic RunAs or live acceptance has been started by this investigation.
At that historical checkpoint PHASE3D remained PARTIAL until those fresh live
gates and visible-motion review passed. The later final section records the
completed gate. The old failures/UNKNOWN records remain unchanged. No3E, phone, driver,
certificate or boot/security work is authorized by this change.

## Continued investigation — diagnostic v2 (PARTIAL)

No acceptance threshold, GOP, protocol, queue policy, driver, certificate or
security setting changed. The original v1 executable and `visual-flow-1` are
preserved. New build output is `out/device-simulator-visual-diag-v2`; default
acceptance scripts still select v1. `-StageDiagnostics` explicitly selects the
instrumented diagnostic executable, not an accepted replacement.

**VERIFIED component observations; historical cause remains UNKNOWN:**

* `resource-probe-1` retained the previously observed delayed +31 handles during
  recorded playback. `resource-probe-2` did not reproduce that jump: first
  successful Present933, then936/937, with937 unchanged from frame180 through
  frame1700. Decoder recreation ends at937/938/939; renderer cleanup ends at
  257/258/258. The first probe's cleanup counts were280/280/280. Neither series
  proves an unbounded leak; neither explains the original935->971 conclusively.
* Both three-renderer/nine-decoder-session probes independently match recorded
  AU content:2660 decoded in each, with116 and169 accepted rendered marker
  matches respectively. First probe1835 Presents, second2660; these are not
  lossless presentation claims for the first probe. Raw subtype inventories,
  private/working-set observations and repeated cleanup results are retained.
* `resource-idle-1` isolates renderer-only operation:849 initially,847/848 then
  stable848 for the rest of45seconds;227 after renderer cleanup. No decoder
  exists in this control (the legacy probe row named `decoder_destroyed` is only
  a scope marker in idle mode). Delayed +31 did not occur here either.
* Type-aware PSS data from the earlier +35 first-Present-to-steady difference
  contains ALPC+4,Event+6,File+1,IoCompletion+1,IoCompletionReserve+1,Thread+4,
  unnamed/unknown+11,WaitCompletionPacket+7. These types are not allocation-stack
  ownership proof. New probe2 includes thread-start module basenames, without
  object names or addresses; it cannot retroactively attribute probe1's jump.

The +16 handle guard is **unchanged**. Cold initialization and post-warmup
resource behavior are reported separately, but no failed live run is promoted.
The initialization explanation for all36 historical handles is NOT VERIFIED.

### Timing and bounded observer

`StageTrace.h` preallocates262144 rows per producer thread. Hot-path records use
QPC and thread-owned memory only: no I/O, allocation or observer lock. Logs flush
after workers stop. Overflow is counted and fails the diagnostic result; it is
never silently truncated. A bound/exhaustion self-test passes (one measurement
26.79ns per record, not a guarantee under load). Each event carries session,
frame identity, generation, stage, value and applicable HRESULT. `PeekMessage`
and dispatch now have separate timings in addition to whole-pump timing.

Measured stages cover Channel receive/framing, connection validation, evidence
writes, ACK, encoded queue admission/wait/pop, input acceptance, output calls,
decoded-to-render handoff, video blit, sparse diagnostic Map, Present and reset.
Channel receive includes its existing framing/logging work and socket waiting;
it is not labelled pure CPU parse time. No independent decoded queue exists:
render executes inline after output, and its handoff interval is measured.
Recovery starts at the exact overflow and ends at admitted IDR/SPS/PPS; it is
reported separately from processing and queue latency.

`ResourceWatch.h` samples totals/types on its own thread at lifecycle requests,
one-second totals and five-second periodic snapshots. It logs observation
begin/end and coalesced reason bits; exact request-stage times are in the stage
journal. Asynchronous snapshots are **not** claimed to be instantaneous at the
request. Requests cover connection, renderer/decoder creation, first submission,
first decoded frame, first successful Present, reset/release and cleanup/shutdown.
PSS remains diagnostic-only and its overhead is observable: stage-replay3
snapshots took approximately2–4ms; an earlier stressed component case reached
23.84ms. The baseline replay below reproduced cold overflow **without** PSS.

### Independent reproductions (recorded source, not live acceptance)

| Run | Actual receive cadence | Received / decoded / Presented | Receiver overflow / resync | Render marker matches |
|---|---|---|---|---|
| stage-replay-1 |32.1255FPS|1700 /1700 /1700|0 /0|101|
| stage-replay-2 |55.5558FPS|1700 /1581 /1580|1 /114|56|
| stage-replay-3 |55.5553FPS|1700 /1462 /1460|2 /228|52|

The first attempt used Sleep pacing and did **not** achieve its intended cadence.
Only the recorded fixture sender was changed to a high-resolution waitable timer
for the following controls. Live source cadence, encoder and protected sender
implementation were not changed. No duplicate AU was manufactured.

In stage-replay2 the first decoded frame's ProcessOutput took about31ms, followed
by41.4874ms of render work. Overflow on received frame6 occurred during the
30.724ms blocking sparse Map within that render. Recovery on frame121 took
2069.8957ms after loss. This establishes a cold-path blocking mechanism in the
reproduction, not the exact unrecorded stage of the old live run.

Stage-replay3 adds the background PSS observer. Overflow on frame6 again overlaps
first-frame Map. The second overflow is on frame126 during the **first render
after decoder recovery**, with39.9891ms in Map; recovery is on frame241 after
2069.9616ms. It is not a reproduction of the old late-run overflow. All1700 AUs
were received/ACKed; receiver accounting is1700=1470admitted+2overflow+228resync,
1470=1464submitted+6queue-reset discards,1464=1462decoded+2decoder-reset discards,
1462=1460Presents+2stale-generation suppressions. Queue bounds3/2/1 hold.

The historical second loss has41 intervening received frames without an IDR;
the next valid recovery point arrives1520.6591ms later. Its1574.3798ms visible gap
is therefore **not** evidence of a1574ms decoder call. The exact historical
blocking stage remains UNKNOWN; no old failure is removed or reclassified.

### Component/regression gates and next action

`diagnostic-components-1` failed its unchanged200-decoded-frame requirement:
two overflows happened while the UI pump was active. Overlapping pump calls
were65.18/62.41ms; maximum pump duration555.14ms. This run overlapped the protocol
regression workload; precise OS/message scheduling cause is UNKNOWN. Its error
and partial output are preserved. No gate was bypassed.

Additional PeekMessage/dispatch instrumentation was then built.
`diagnostic-components-2` passes: two sessions,200 decoded/196 Presents,18 marker
matches, resize/minimize/restore; WM_CLOSE case59 decoded/55 Presents with exact
discard accounting and clean exit; malformed SPS/PPS rejects as expected.
`diagnostic-reconnect-1` passes recorded-stream fresh-process reconnect:84 then60
Presents,6+4 marker matches, new session/IDR/SPS/PPS and clean close/drain. These
are component evidence only; normal-operation stall elimination is NOT VERIFIED.

Protected PHASE3C regression:58,155 deterministic checks plus normal/slow
integration and9 shutdown-policy cases pass under new evidence names.
SHA256 audit matches60 original failed-run files,409 protected3C/source/binary
files,1231 earlier evidence files,23 driver/shared and5 earlier binaries.
Read-only current PnP is0/0 and helper remains present. Secure Boot state registry1,
running HVCI state1 and CI flags62465/TESTSIGNING_OFF match the baseline; the
administrator UEFI query remains part of the next live preflight.

At that point the next step was one **diagnostic**, not acceptance,35-second real Display3 capture using
`Test-VisualTransport.ps1 -StageDiagnostics` and unused `visual-diagnostic-1`.
The agent's process is not Administrator and automatic RunAs is not used; the
owner launched that prepared command from Administrator PowerShell; its result
and subsequent bounded sparse fix are documented above. It did not start a
reconnect or final acceptance run. Separating UI-message stalls remains a
candidate pending evidence rather than an implemented architecture change. Do not
prewarm with fabricated H264 reference state. Do not change GOP or add IDR
requests before root-cause results justify it. Protocol CONTROL remains only
DRAIN/DRAIN_ACK. Fresh final >=35s normal and45s real reconnect remain gated;
PHASE3D stays PARTIAL and PHASE3E is not started.

## First real live result — visual-flow-1-normal

Owner ran the prepared Administrator controller and reports visibly moving
simulator content. Original controller outcome ERROR is preserved:
`Short-run resource growth requires investigation`. The45-second live reconnect
stage was not started. Owner observation and read-only independent review are
retained separately in `phase3d/visual-flow-1-review-1`; no original file changed.

| Metric | Measured value |
|---|---|
| Source duration | 35.0045096s |
| Source / Host | 1930 /1493;55.135753 /42.651647FPS |
| Encoded / received / ACKed / independently decoded | 1487 each;42.480241FPS over source duration |
| Live GPU decode / accepted Present | 1327 /1326;37.909401 /37.880834FPS over source duration |
| Changing presented diagnostics | 61/61 match both independent decoded AU and original Display3 nonce/counter/PTS association |
| First-to-last accepted presentation span | 32.6451542s |
| Receive-to-Present acceptance p50/p95/p99/max | 27.9957 /62.5204 /77.1868 /144.4038ms |
| Longest inter-presentation gap | 1574.3798ms; resync interruptions are not hidden by the per-frame latency distribution |
| Shared / encoder / receiver-encoded / decoder / render queue peaks | 3 /1 /3 /2 /1 |
| Source classification | A1493 /B0 /C0 /D0 /E0;invalid0 |
| Encoder | AMD hardware MFT, GPU NV12 input, no software fallback;13 IDRs;21.242296Mbps |
| Decoder | Microsoft H264 MFT;1327 decoder-bound NV12 GPU surface identity proofs |
| Simulator GPU counters | Video Codec0 mean6.29%,max8%;3D mean10.86%,max15%;7 samples. Dedicated Video Decode1 reports0, so do not label the combined codec counter a dedicated decode metric |
| Shutdown and health | Host/receiver/pattern exit0, no forced cleanup;PnP0/0,SecureBootON,HVCION,TESTOFF;relevant failure events0 |

Exact accounting:
- 1930 source=1493 Host+12 producer drops+424 stale Host queue drops+1 pending upstream.
- 1493 Host=1487 encoder accepted+6 encoder pressure drops; rate-limit0.
- 1487 encoded=1487 received=1487ACKed; transport overflow/disconnected/unconfirmed0.
- 1487 received=1335 receiver admitted+2 queue overflow+150 resync skipped.
- 1335 admitted=1329 decode submissions+6 queue reset discards.
- 1329 submitted=1327 decoded+2 decoder reset discards.
- 1327 decoded=1326 accepted Presents+1 old-generation presentation suppressed.

The original resource comparison starts at935 handles and ends at971: growth36
exceeds the unchanged16-handle guard. Private-byte growth21082112 (20.11MiB) is
below the32MiB guard. Handles reach971 early and remain mostly flat, with a brief
967 sample before decoder recreation. Private memory has initialization and
recreation steps rather than a proven monotonic leak. Root cause remains UNKNOWN;
startup/allocator and reset contributions require targeted measurement. The
late source-to-Host slowdown and two receiver overflows also need investigation.
Do not raise thresholds or promote this failed run merely from the visual result.

Live movement/content correspondence is demonstrated. Overall PHASE3D remains
PARTIAL pending resource/overflow diagnosis, an accepted normal run, and real
live reconnect/recovery. Historical component results below are retained.
## Implemented path

The new executable is built from `windows/simulator/SweetDisplayDeviceSimulator.cpp`
and `VisualPipeline.h` into `out/device-simulator-visual-v1/`. The original
PHASE 3C endpoint, Host, protocol, encoder, driver and shared ABI are unchanged.
Dedicated Visual controller/verifier adapters retain the existing source,
hardware encode, independent decode, wire/CRC/ACK and health acceptance checks.
The protected PHASE 3C scripts are not edited.

The selected decoder is Microsoft H.264 Decoder MFT,
`CLSID_CMSH264DecoderMFT`, `{62CE7E72-4C71-4D20-B15D-452831A87D9D}`.
It receives a non-null `IMFDXGIDeviceManager` using a hardware D3D11 device.
`MF_SA_D3D11_AWARE` must be true. The code never detaches the manager to retry
with CPU output. Every decoded sample must expose `IMFDXGIBuffer`, the same
device, NV12/default-usage output, zero CPU access and `D3D11_BIND_DECODER`.
The recorded runs measured format103, bind512, usage0, CPU-access0, array11
on the AMD adapter. Unsupported negotiation or CPU buffers cause failure.

This is runtime DXVA-surface evidence in addition to successful decode, not an
inference merely from NV12. Per-process GPU Video Decode/Video Codec and 3D
counters are requested by the live controller; fresh live utilization remains
NOT YET TESTED. An unavailable counter must not be reported as zero utilization.
Microsoft documents the non-null device-manager integration and the distinct
software fallback renegotiation. Its `AVDecVideoAcceleration_H264` property has
no effect through IMFTransform and is deliberately not used as proof.
[D3D11 decoding](https://learn.microsoft.com/en-us/windows/win32/medfound/supporting-direct3d-11-video-decoding-in-media-foundation),
[H.264 property](https://learn.microsoft.com/en-us/windows/win32/codecapi/avdecvideoacceleration-h264-property).

Decoded NV12 array slice -> D3D11 video processor -> letterboxed BGRA DXGI
flip swapchain -> nonblocking Present. Coded 2400x1088 is cropped using the
decoder's verified 2400x1080 aperture. No full-frame CPU readback, CPU color
conversion or software codec fallback exists in this normal path. Sparse
validation reads only 64 backbuffer pixels approximately twice per second.
These measure nonce/counter after GPU conversion and scaling. Offline CPU
decoding remains a separate, unchanged independent evidence oracle.

## Bounds and session policy

- Encoded queue: three AUs, each at most 4MiB, plus one active input.
- Decoder association map: at most16 pending samples; explicit failure on exhaustion.
- Render stage: one immediately consumed decoded surface; no accumulating queue.
- Output poll: at most32 attempts. Frame/message/evidence byte caps remain enforced.
- Queue overflow discards the pending dependent chain and resets the decoder;
  subsequent non-IDR frames are counted as resync skips until IDR+SPS+PPS.
- Receipt ACK retains its PHASE 3C meaning; it does not assert presentation.
- Session invalidation increments an epoch. Only nonblocking Present is serialized
  with invalidation; network receipt does not wait on GPU conversion/readback.
  Old-generation surfaces cannot be presented as the new generation.
- Session reset flushes/recreates the MFT, clears the window and labels it waiting
  for a new IDR. Minimized/occluded windows are explicit nonpresented frames.
- WM_CLOSE stops reception, joins the worker, accounts queued/decoder leftovers,
  releases graphics state and exits. Final DRAIN waits for bounded decode drain.

Counts distinguish transport receipt/admission, queue overflow/reset/resync,
decode submission/output/reset discard, GPU render submission, accepted Present,
minimized/occluded/busy/stale-generation drops. Source content counters are never
required to increase monotonically; source IDs/PTS/session associations are checked.

## Verified component results

All paths below are relative to ignored `docs/evidence/private/phase3d/`.

| Evidence | Measured result | Scope |
|---|---|---|
| `replay-1`, `replay-2` | 200 decoded, zero accepted Presents; occluded desktop context | Preserved failures, not accepted |
| `replay-3` | 200 decoded,196 Presents,4 controlled minimized drops;17 presented diagnostic samples match independent decode | Recorded component result |
| `components-2/replay` | 200 received/submitted/decoded,196 Presents,4 minimized drops;18 diagnostic matches, two sessions, resize/minimize/restore; queues1/2/1 | Final-build recorded component PASS |
| `components-2/close` | 61 received,60 submitted,59 decoded,55 Presents;1 queued shutdown discard,1 decoder shutdown discard,4 minimized drops;6 diagnostic matches; exit0 | WM_CLOSE and exact shutdown accounting PASS |
| `components-2/corrupt` | Malformed SPS/PPS produces no corresponding output; exact drain count rejects it, exit1 | Expected rejection PASS |
| `network-reconnect-1` | Real localhost fixture disconnect/reconnect; two sessions and fresh IDR/SPS/PPS | Protocol+visual component PASS; source is recorded |

The final network fixture accounts300 generated replay submissions as246 admitted
+26 disconnected+28 resync skipped;246 admitted=245 ACKed+1 unconfirmed.
First receiver:65 received,64 submitted,63 decoded/presented, one queue discard
and one decoder discard on WM_CLOSE;6 rendered diagnostic matches. Fresh receiver:
180 received/submitted/decoded/presented,17 diagnostic matches, no render drop.
Both have encoded/decoder/render peaks1/2/1. Source sender remained active.
Replay timing is artificial test pacing and is not a live source/capacity claim.

The first experimental `replay-1/visual-result.json` contains a premature
`PASS_COMPONENT_RUN` label written before the final assertion. Its process failed
with zero Presents, so it is NOT accepted. That historical file is unchanged;
subsequent results explicitly say `COMPONENT_MEASUREMENTS_ONLY` and require
external verification. The initial link failure was fixed by linking the official
`wmcodecdspuuid.lib`; all build attempts remain in `preflight-1/build-*.txt`.

PHASE 3C regression: `phase3c/phase3d-regression-final-1` passes58,155 deterministic
protocol/parser/queue/resync checks plus normal and slow receiver integration.
Final simulator build uses EWDK26100.6584 and `/W4 /WX`, zero build warnings/errors.
`preflight-1/final-preservation.json` independently confirms409 protected files
unchanged, including previous PHASE 3C evidence, binaries and protected source.

## Live acceptance preparation and remaining gates

`Invoke-Phase3DValidation.ps1 -RunName <new-name>` is prepared for an existing
Administrator Windows PowerShell token. It does not request elevation or change
policy. It refuses existing controller/normal/reconnect paths, starts a35-second
real classified uncapped2400x1080/60-target/30Mbps hardware stream, independently
decodes actual captured AUs and checks source/wire/render correspondence. Only
after those checks pass does it start a45-second live run with controlled receiver
WM_CLOSE and fresh-process recovery while Host remains active.

The simulator window must remain visible for the live observation. The normal
gate requires at least30 seconds between actual accepted changing presentations,
not merely elapsed controller time. Initial guard: receipt-to-Present acceptance
maximum500ms, with ten-second latency bins. Short resource growth exceeding32MiB
or16 handles after startup triggers investigation; passing is not a long-term
leak-free claim. The controller retains CPU/memory/handle and GPU-engine observations.
Presentation timestamps are CPU-side accepted-Present times, not scan-out/photons.

During initial preparation the tool token was not Administrator. Therefore the installed Display3
device-interface test was deferred to the owner; no RunAs/UAC bypass was attempted.
The owner subsequently ran the command and observed the
changing simulator window; it stopped at the resource gate above. A successful controller ends at `PASS_ARTIFACTS`, leaving the
phase PARTIAL until independent final review and visible observation are complete.

Initial read-only preflight: adapter/monitor Problem0/0, HVCI1, CI status0/flags62465
(TESTSIGNING OFF), Secure Boot state registry1. Direct firmware
`Confirm-SecureBootUEFI`, before/during/after live health and crash-event checks
subsequently passed in the owner-run normal test above. No deployment,
certificate, boot/security configuration or phone change was made.

Remaining acceptance: accepted normal live run after resource/overflow diagnosis;
resolved resource/overflow behavior; live receiver recovery and source
correspondence and final health checks for that recovery. Normal-run direct security/crash checks and owner observation now exist.
PHASE 3D remains PARTIAL. Historical PHASE 3A UNKNOWN evidence and all prior
VERIFIED results are retained. Stop after3D; no3E/touch/USB/phone work.

Final preparation hygiene PASS:156 publishable files,index and125 history blobs.
An additional hash audit confirms1,231 earlier evidence entries and23 driver/shared
baseline entries unchanged. The current visual executable equals the binary
retained with the final components-2 tests. No live test was started by this agent.
