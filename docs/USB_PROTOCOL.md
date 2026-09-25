# USB mapping and implemented protocol

The implemented transport-neutral wire contract is [PROTOCOL.md](PROTOCOL.md):
version 1.0, fixed 48-byte header, binary HELLO/CAPABILITIES, bounded H.264 FRAME,
exact receipt ACK, heartbeat, drain and IDR/SPS/PPS recovery. This supersedes the
old unimplemented 64-byte header/JSON draft. No phone implementation used that
proposal. Its original text is preserved in the private pre-3C snapshot.

PHASE 3C uses localhost TCP through a deadline-aware ByteStream interface.
Implementation and acceptance limits are in [PHASE3C_RESULTS.md](PHASE3C_RESULTS.md).
PHASE 3C is VERIFIED for bounded localhost streaming/reconnect. Visible rendering
belongs to PHASE 3D and has not started; USB support is still unimplemented.

A future WinUSB/vendor-bulk or USB-network adapter must preserve identical
message bytes, partial-read/write semantics, bounded buffering, deadlines and
fresh session/parser state after disconnect. Transfer boundaries have no framing
meaning. Never resume dependent H.264 frames after loss without IDR and parameter
sets. Device and PC monotonic clocks are independent; no direct timestamp
subtraction may be reported as cross-device latency without synchronization.

USB discovery, gadget configuration, endpoints, descriptors and device-side
services are NOT YET TESTED or implemented. USB-C does not prove negotiated link
speed. Distributed hardware requires legitimate USB identifiers. No phone,
WinUSB or gadget operation is part of this phase.

Future platform references: [configfs](https://docs.kernel.org/usb/gadget_configfs.html)
and [FunctionFS](https://docs.kernel.org/usb/functionfs.html). These are design
references, not evidence of Redmi support.
