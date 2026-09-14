# SweetDisplay wire protocol — draft v1

Transport-neutral framing; implementation/interop NOT YET TESTED. Initial post-unlock candidate is TCP over a verified USB network gadget, subject to Windows class support. Later compare FunctionFS vendor bulk + WinUSB. UDP needs a separate fragmentation/loss design before use. USB-C does not establish negotiated speed. Use legitimate assigned USB identifiers for any distributed device; do not ship another vendor's VID/PID.

All integers unsigned little-endian, encoded field-by-field (never memcpy a native struct). Fixed base header is 64 bytes:

| Offset | Bytes | Field |
|---|---|---|
| 0 | 4 | Literal ASCII SWDP |
| 4 | 2 | Major version = 1 |
| 6 | 2 | Header bytes = 64 initially, max 4096 |
| 8 | 2 | Message type |
| 10 | 2 | Flags |
| 12 | 4 | Stream ID |
| 16 | 8 | Session ID (new handshake/reconnect) |
| 24 | 8 | Message/frame sequence ID per stream |
| 32 | 8 | Sender monotonic timestamp, nanoseconds |
| 40 | 4 | Width |
| 44 | 4 | Height |
| 48 | 4 | Format |
| 52 | 4 | Payload bytes |
| 56 | 4 | Stride bytes for packed raw data; zero for codec |
| 60 | 4 | Reserved, zero |

Types: 1 HELLO, 2 HELLO_ACK, 3 FRAME, 4 TOUCH (diagnostic/future only; native HID is separate), 5 CONTROL, 6 TELEMETRY, 7 CAMERA_CONTROL, 8 HEARTBEAT. Format: 0 none, 1 BGRA8, 2 RGB24, 3 H.264 Annex B access unit, 4 H.265 Annex B reserved for later. Flags: bit 0 keyframe, bit 1 discontinuity; other bits zero in v1. Raw frames use neither keyframe flag nor codec dependencies.

Receiver must read exactly header bytes then exactly payload bytes over streams; USB transfer and TCP packet boundaries have no framing significance. Before allocation reject wrong magic/major, lengths outside limits, nonzero reserved fields, unnegotiated formats, invalid dimensions/stride or unexpected session IDs. Local hard limits: 32 MiB payload, 4096 width/height. Caps can lower limits. Checked arithmetic: raw stride >= width*bytes-per-pixel and payload == stride*height. Reject invalid header and close session; do not scan arbitrary payload for magic. Validate known-message semantics before dispatch. Extension bytes, when negotiated, contain 16-bit type and 16-bit length TLVs (length excludes 4-byte TLV header); unknown optional TLVs skipped, critical type high bit unsupported → reject. Initial v1 peers send no TLVs.

HELLO/ACK payloads are UTF-8 JSON bounded to 4096 bytes, with agreed major, session ID as hexadecimal string, supported formats, maximum dimensions/payload and timestamp clock name. Session chosen by host and echoed; streams reset on reconnect. Unknown message types are skipped after bounded framing validation unless a negotiated critical extension requires rejection. Mandatory capabilities must be agreed before FRAME. Other message payload schemas remain reserved; do not claim TOUCH/control interoperability yet.

Whole raw frame per message; H.264 carries one complete access unit, timestamps are presentation timestamps in the sender clock domain. Send parameter sets with the initial/recovery IDR. On congestion prefer dropping raw frames before encoding. If encoded reference frames are lost/dropped, request a new IDR and discard dependent output until recovery. Two-frame application queue, transport timeouts and heartbeat prevent unbounded latency. TCP preserves bytes but may accumulate stale data; measure and reconnect if needed.

PC QPC and phone CLOCK_MONOTONIC are different clock domains. Convert QPC ticks using recorded frequency with overflow-safe arithmetic. Use ping/echo clock-offset estimates with uncertainty for telemetry; never subtract unaligned clocks to claim latency. Physical camera/high-speed visual measurement can validate total display latency.

Calculated BGRA payload only: 800x360x4x10 = 11.52 MB/s; 2400x1080x4x60 = 622.08 MB/s. These are arithmetic budgets, not measured bandwidth. Later camera shares the link; measure sustained effective throughput with display, touch and camera together.

References: [configfs](https://docs.kernel.org/usb/gadget_configfs.html), [FunctionFS](https://docs.kernel.org/usb/functionfs.html). Gadget creation/binding changes device state and is not run during the locked phase.
