# Touch — DEVICE PHASE 2C-T PARTIAL

Phase 2C-T validates a deliberately narrow development link: ordinary Android
`MotionEvent` capture travels through the already authorized ADB-forwarded
SweetDisplay connection and becomes Windows touch injection only after an
explicit profile/configuration/ready exchange. It is not a USB HID gadget,
does not enumerate a new device, and does not modify ConfigFS, UDC, USB
composition, Android input state, accessibility, SELinux or AVB.

The Windows host dynamically identifies the active SweetDisplay target and
derives its current geometry and opaque topology token. The phone normalizes the
content rectangle to 0..65535; the host maps it once to that target. It neither
hard-codes a monitor index/origin nor uses raw evdev input. Real corner and
drag controls reached the independent Display 3 target without observed swap,
mirror, duplicate rotation or inset offset. One and two independent contacts
produced the expected pointer down/move/up transitions.

Every contact record includes a bounded slot, action, normalized position,
optional pressure, active-contact mask and monotonic device timestamp. The host
rejects stale sessions, unexpected state transitions and changed topology.
DEVICE PHASE 2C-T remains PARTIAL because its historical controlled restart
recorded timeout then invalid-parameter around release; that historical outcome
is not relabelled.

The separately authorized DEVICE PHASE 2C-T1 follow-up is VERIFIED. Its audit
proved that the old timeout was returned by `InjectTouchInput`, while the old
invalid-parameter value was captured too late to attribute to that API. The
transport worker now solely owns initialization, injection and release; desired
and successfully delivered coordinates are separate; active contacts receive
50 ms UPDATE keepalives; teardown sends matching all-active UPDATE then UP; and
failed release retains state and blocks reuse. Windows-only stress and repeated
real single-/two-contact disconnects ended with no stuck contact or unexplained
API failure. This does not authorize HID, calibration, native input, Phase 2C-1
or Phase 3E.

The relevant profile and acceptance details are in [the protocol](PROTOCOL.md),
[the injection lifetime](TOUCH_PROTOCOL.md), [DEVICE PHASE 2C-T results](device/DEVICE_PHASE2CT_RESULTS.md)
and [DEVICE PHASE 2C-T1 results](device/DEVICE_PHASE2CT1_RESULTS.md). Windows
injection follows [InjectTouchInput](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-injecttouchinput)'s documented initialization and
pointer-lifecycle requirements.
