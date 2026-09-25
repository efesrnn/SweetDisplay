# SweetDisplay binary protocol 1.0

Implemented in `windows/protocol/Protocol.h`. PHASE 3C is VERIFIED for bounded
localhost H.264 transport and real receiver reconnect. DEVICE PHASE 2C-T adds
an opt-in, negotiated touch profile to that same byte stream. Historical 2C-T
remains PARTIAL because its disconnect release received an ambiguous Windows API
result; the separate 2C-T1 release-reliability gate is VERIFIED after exact error
localization and corrected stress/live tests. See the phase result documents for
exact scope and preserved failures. This replaces the earlier unimplemented
64-byte JSON-handshake draft.

## Byte framing and limits

All integers are unsigned little-endian, serialized field by field. No native
C++ layout, pointer, Windows handle or compiler padding crosses the wire. TCP
packet, send/recv and future USB transfer boundaries have no message meaning.
Read exactly the header, validate it, then assemble the stated payload.

| Header offset | Bytes | Value |
|---|---|---|
| 0 | 4 | Literal ASCII `SWDP` |
| 4 | 2 | Major 1 |
| 6 | 2 | Minor 0 |
| 8 | 2 | Message type below |
| 10 | 2 | Header size, exactly 48 |
| 12 | 4 | Payload bytes, excludes header |
| 16 | 4 | Flags, zero |
| 20 | 4 | Reserved, zero |
| 24 | 8 | Nonzero session ID selected by Host |
| 32 | 8 | Nonzero sequence, independent in each direction |
| 40 | 8 | Nonzero sender monotonic time in nanoseconds |

| Type | Name | Payload bytes | Current behavior |
|---|---|---|---|
| 1 | HELLO | 24 | Version/role/clock/profile handshake |
| 2 | CAPABILITIES | 32 | H.264 limits/profile |
| 3 | FRAME | 65 through 4 MiB + 64 | One complete Annex B access unit |
| 4 | TOUCH | 32 | Profile 1 only after explicit touch negotiation |
| 5 | CONTROL | 8 | Drain and drain acknowledgement |
| 6 | TELEMETRY | 32 | Exact cumulative FRAME acknowledgement |
| 7 | HEARTBEAT | 8 | Opaque token echo |
| 8 | CAMERA_CONTROL | 8 | Reserved, unnegotiated; rejected |

Unknown types, versions, flags, reserved values and lengths are fatal. No message
has a legal empty payload. A heartbeat's eight-byte token may be zero. Maximum AU
size is 4,194,304 bytes; maximum payload is 4,194,368 bytes. Header extensions/TLVs
are not supported. Close on violation; do not scan arbitrary bytes for magic.

Parser storage is a fixed 48-byte header and 64-byte FRAME prefix plus at most
one bounded payload. Validate FRAME metadata before large payload allocation.
Partial header/payload at EOF is truncation. Parser errors poison that parser;
new connections require a new parser, never old partial bytes. Generic Feed
supports fragmented and coalesced messages, including a partial next message.

## Connection and version negotiation

Host selects a cryptographically random nonzero 64-bit session ID. Host sends
HELLO; Device adopts the session and sends its HELLO. Each sends CAPABILITIES
after receiving HELLO, then enters READY after receiving peer capabilities.
FRAME/control/telemetry/heartbeat before READY fail. Each direction starts
sequence 1 and increments by exactly one for every message, including heartbeat.
Duplicates, gaps, regression or session mismatch close the connection. Message
timestamps must not regress within a direction; equal timestamps are allowed.

The bootstrap header is strictly 1.0. HELLO advertises an inclusive extension
minor range while the header minor remains zero. Legacy endpoints advertise
0..0. A touch-capable endpoint may advertise 0..1 and use profile 1 only when
both peers advertise the touch feature and the exact touch descriptor. A 0..N
peer can negotiate 0, but a 1..N peer cannot. A nonzero header minor is rejected.
Future versions must deliberately add negotiation support; no silent acceptance
of unknown fields or features.

HELLO payload:

| Offset | Type | Meaning |
|---|---|---|
| 0 | u32 | Role: Host 1, Device 2; peer must have opposite role |
| 4 | u16 | Minimum supported minor |
| 6 | u16 | Maximum supported minor |
| 8 | u32 | Clock: 0 independent monotonic; 1 same-PC QPC nanoseconds |
| 12 | u32 | Reserved zero |
| 16 | u64 | Required feature/profile value 1: CRC and exact ACK |

CAPABILITIES payload:

| Offset | Type | Meaning |
|---|---|---|
| 0 | u32 | Codec mask exactly 1, H.264 |
| 4 | u32 | Maximum AU bytes: 1..4,194,304 |
| 8 | u32 | Maximum width: 1..4096 |
| 12 | u32 | Maximum height: 1..2160 |
| 16 | u32 | Required profile value 1 |
| 20 | u32 | Clock, must match this peer's HELLO |
| 24 | u64 | Reserved zero |

Current endpoints advertise the hard maxima. Host checks each outgoing FRAME
against receiver limits. Receiver enforces hard bounds and advertised source
capabilities. An incompatible limit fails the session; no automatic transcoding
or software fallback occurs.

## FRAME payload

The 64-byte metadata prefix is followed immediately by one H.264 Annex B AU.
Dimensions are the visible image, not the encoder's padded coded aperture.

| Offset | Type | Meaning |
|---|---|---|
| 0 | u64 | Nonzero source frame ID |
| 8 | u64 | Nonzero source time, nanoseconds |
| 16 | u64 | Encoder presentation timestamp, 100 ns units, <= INT64_MAX |
| 24 | u32 | Codec 1 = H.264 Annex B |
| 28 | u32 | Nonzero even visible width <=4096 |
| 32 | u32 | Nonzero even visible height <=2160 |
| 36 | u32 | AU byte count, 1..4 MiB; payload must equal 64 + count |
| 40 | u32 | Bit 0 IDR; bit 1 SPS; bit 2 PPS; bit 3 MFT clean point |
| 44 | u32 | CRC32/IEEE of AU bytes only |
| 48 | u64 | Nonzero original source clock ticks (QPC on Windows) |
| 56 | u64 | Source clock frequency, 1..1,000,000,000 Hz |

Other FRAME flag bits are forbidden. CRC uses reflected polynomial 0xEDB88320,
initial value 0xFFFFFFFF, final XOR 0xFFFFFFFF. CRC detects accidental corruption;
it is not authentication. Source nanoseconds must equal overflow-checked integer
conversion `ticks/frequency*1e9 + (ticks%frequency)*1e9/frequency`.

NAL inspection requires a leading Annex B start code, VCL, clear forbidden bit,
NAL types 1..23 and agreement of actual IDR/SPS/PPS with metadata flags. This is
framing/category validation, not a full H.264 decoder. The independent offline
decoder supplies additional real-bitstream/content evidence.

Each session's first FRAME must include IDR + SPS + PPS in that AU. Subsequent
source frame ID, source time and PTS must increase strictly. Source ID gaps are
allowed: upstream replacement/admission drops differ from wire message loss.
PTS may start at zero. A new transport session may continue an existing encoder's
PTS and source IDs; it need not restart the encoder.

## Acknowledgements, control and heartbeat

Device sends one TELEMETRY after each fully parsed and CRC/metadata-validated
FRAME. This is receipt validation, not a claim that a decoder rendered the frame.

| TELEMETRY offset | Type | Meaning |
|---|---|---|
| 0 | u64 | Cumulative validated FRAME count in this session |
| 8 | u64 | Last FRAME's header sequence |
| 16 | u64 | Last source frame ID |
| 24 | u64 | Cumulative validated AU bytes in this session |

Sender allows one unacknowledged FRAME at a time and verifies all four fields
exactly. CONTROL is u32 operation plus u32 reserved zero: 1 DRAIN, 2 DRAIN_ACK.
Host drains its pending queue then requests DRAIN; Device replies DRAIN_ACK and
closes normally. An idle Host sends an eight-byte HEARTBEAT token about once per
second; Device echoes it. A malformed/unexpected response terminates the session.

## Negotiated touch profile 1

TOUCH is rejected unless both peers negotiated extension minor 1, feature bit 2
and the exact profile descriptor. Its fixed 32-byte payload is either a
configuration/ready record or one contact record. All fields are little-endian;
the profile code validates reserved bits, action sequence, active-contact mask,
coordinate bounds, pressure range and nondecreasing device timestamp. The
configuration carries only the currently discovered logical target geometry and
an opaque topology token. It is not a monitor number or desktop-coordinate
contract.

The Android Activity captures ordinary public `MotionEvent` data. The Windows
host discovers the active SweetDisplay target at session start and maps the
normalized content coordinates once into its current desktop rectangle. It
rejects a changed target token, stale session ID, malformed transition, or a
message before ready; an endpoint disconnect releases its local contact state.
The protocol test suite includes a reconnect case that rejects a validly formed
TOUCH message from the retired session after a fresh handshake. This transport
is an ADB-forwarded development link, not USB HID, a gadget change, an
accessibility service, or a system input driver.

Profile 1 has live evidence for independent Windows pointer delivery of one and
two real phone contacts. DEVICE PHASE 2C-T remains PARTIAL because its historical
controlled-disconnect run recorded timeout then invalid-parameter around release.
The separate DEVICE PHASE 2C-T1 gate identified the timeout as an old
`InjectTouchInput` failure and localized the invalid-parameter record to late,
non-authoritative `GetLastError` capture. Its corrected single-owner release path
passed Windows-only stress plus repeated real single- and two-contact disconnects
with zero accepted API failures and zero final active contacts. See
[the touch lifetime](TOUCH_PROTOCOL.md) and
[DEVICE PHASE 2C-T1 results](device/DEVICE_PHASE2CT1_RESULTS.md). No later input
or USB-gadget phase is implied. No camera operation is implemented.

## Transport bounds, loss accounting and recovery

`ByteStream` abstracts deadline-aware partial reads/writes. Current TCP adapter
listens exclusively on 127.0.0.1, default port 48231. It uses nonblocking sockets,
20 ms select polling, requested 64 KiB socket buffers and TCP_NODELAY. Handshake
deadline is 2 s; FRAME send/ACK deadline is 1 s. No firewall rule is added.

One network worker owns socket I/O. Encoder callback copies compressed AU bytes
into a queue; it never reads back full GPU images or performs socket I/O. Queue
capacity is three pending AUs plus one in flight, with at most 3*(4 MiB+64)
pending bytes. Queue age limit before send is 250 ms. A full queue rejects the
newest AU, aborts continuity and closes the connection; it does not retain an
unbounded backlog. The age limit is not a guaranteed total end-to-end latency.

Any disconnect/queue loss flushes pending entries with explicit outcomes and
invalidates receiver reference continuity. After fresh HELLO/CAPABILITIES,
dependent frames are suppressed until an AU includes IDR + SPS + PPS. Skips are
counted separately. No duplicate, software re-encode, forced encoder restart or
arbitrary P/B-frame resume is used. Current AMD output supplies parameter sets
with its natural IDRs; recovery can therefore wait for the next GOP boundary.

Exact sender ledgers use:

```
encoded_seen = admitted + disconnected + resync_skipped + queue_overflow
admitted = acked + queue_aborted + unconfirmed
```

The second equality applies after shutdown settles all admitted work. During a
run, pending/in-flight work must also be included. `wire_complete` is separately
recorded: full send does not prove receipt. `unconfirmed` means a send attempt
without a validated ACK, which might already exist in receiver evidence. Never
call all such frames lost; reconcile receiver IDs/bytes. Queue expiry, socket
errors and protocol errors are separate event counters. Upstream source/shared
texture drops, encoder pressure and transport recovery skips are distinct.

Every TCP reconnect creates fresh session/parser/sequence/ACK state. No pending
old bytes or AUs are replayed. Clean Finish drains within bounded deadlines and
joins the worker. Diagnostic ledgers are capped (100,000 sender rows, 40,000
receiver frames); optional compressed receiver evidence is capped at 512 MiB.
Limit exhaustion fails explicitly. Evidence contains no continuous raw video.

New validation builds also retain up to 100,000 complete message-header rows per
endpoint, including both directions, handshake, heartbeats and exact ACK fields.
This permits offline checking of every sequence, rather than inferring gaps in
FRAME-only ledgers. No protocol bytes or queue limits changed for this logging.

## Source-content counters versus transport sequence

The diagnostic pattern nonce/counter is encoded image content, not a protocol
sequence. It is retained in source and decoded evidence, not in the fixed wire
header. Legitimate compositor output can revisit an earlier image/counter.
Transport ordering depends on session and message sequence; frame identity,
source clock and PTS retain their independent strict checks.

For PHASE 3C only, an E source classification whose sole reason is counter
regression may continue provisionally when binary pattern cells, nonce, producer
ledger, publication time and resource identity are valid. It remains E/UNKNOWN
in classification evidence and increments `transport_counter_only`; it is not
relabelled A or C. An independent decoder must then match that frame's actual
source nonce/counter/PTS, while framing, CRC, ACK and session/sequence checks all
pass. A run is not accepted merely because the Host continued.

D integrity failures, E with additional reasons, malformed/ambiguous source
markers, invalid resource/clock association, protocol duplicate/regression,
session violation, damaged AU/CRC or decoder mismatch remain failures. Legacy
PHASE 3A/3B acceptance and binaries are unchanged. Historical failed runs are
not retroactively reclassified under this phase-specific transport policy.

## Clock scope and future device adapter

Windows endpoints on this PC can compare QPC-derived nanoseconds. Measured
transport latency is sender header timestamp to receiver completed validation,
not capture-to-display or encode latency. Clock 0 peers are on independent
monotonic clocks: do not subtract their timestamps. The post-live review build
marks such latency unavailable (`latency_valid=0`); it is not live-verified.
Clock synchronization/uncertainty estimation is future work.

A Linux endpoint can implement these bytes/state rules with its own monotonic
clock (clock 0), incremental parser, CRC, exact ACK and IDR recovery. The raw
source ticks/frequency remain opaque source metadata. Future WinUSB/vendor-bulk
adapters must implement bounded partial ByteStream reads/writes, cancellation,
deadlines and disconnect; USB packet boundaries cannot alter framing. USB setup,
device identifiers, Linux daemons and a visible decoder are not implemented here.
Loopback is a local prototype trust boundary, not authenticated remote transport.

Microsoft references: [exclusive socket binding](https://learn.microsoft.com/en-us/windows/win32/winsock/using-so-reuseaddr-and-so-exclusiveaddruse),
[select](https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-select),
[QPC clock scope](https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps).
