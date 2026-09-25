# Touch profile 1 and Windows injection lifetime

This document describes the implemented negotiated touch extension and the
Windows release state machine verified by DEVICE PHASE 2C-T1. The authoritative
32-byte wire fields remain in [PROTOCOL.md](PROTOCOL.md); detailed acceptance
results are in [DEVICE PHASE 2C-T1 results](device/DEVICE_PHASE2CT1_RESULTS.md).

## Negotiation and trust boundary

TOUCH is legal only when both peers negotiate protocol minor 1, the touch
feature and the exact profile-1 descriptor. Windows sends the dynamically
discovered SweetDisplay geometry and an opaque topology token; Android must echo
the exact configuration in READY before sending contacts. A new TCP connection
has a new protocol session and touch state. Messages before READY, from a retired
session, with a changed topology token or with malformed state are fatal.

The profile carries public Android `MotionEvent` information over the existing
ADB-forwarded byte stream. It is not USB HID, a gadget function, a Windows input
driver, an accessibility service or an Android input-node interface.

## Contact state

Profile 1 supports slots 0 and 1. Coordinates are normalized to 0..65535 in the
already rotation-corrected Android content rectangle. Each record carries one
action, pressure validity, the complete active-contact mask and a nondecreasing
device monotonic timestamp. The receiver enforces:

- `DOWN`: the slot was inactive and is present in the new mask;
- `MOVE`: the slot was active and the mask is unchanged; and
- `UP`/`CANCEL`: the slot was active and is absent from the new mask.

Windows maps a coordinate once into the current SweetDisplay desktop rectangle.
It retains three distinct facts per slot: desired Android position, last
successfully delivered injection position and membership in the injected mask.
A pending MOVE never changes the delivered position until its API frame succeeds.

## Single-owner lifecycle

The transport worker owns all touch operations. Its ordering is:

1. handshake and exact configuration/READY exchange;
2. `Begin`, owner-thread claim and session state creation;
3. lazy `InitializeTouchInjection` on that same thread;
4. contact acceptance plus bounded `Tick` processing;
5. session close/replacement request to `End` on that same worker;
6. final release before transport session invalidation and queue cleanup; and
7. bounded worker join before Host teardown completes.

No main/UI thread injects or independently clears contacts. A different-thread
release request fails closed and is recorded as uncertain.

## Injection frames

The state machine follows Microsoft's
[`InjectTouchInput`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-injecttouchinput)
contract:

- a new contact uses `INRANGE | INCONTACT | DOWN`;
- every other active contact in that frame uses
  `INRANGE | INCONTACT | UPDATE`;
- movement and the 50 ms stationary keepalive include every active contact as
  `INRANGE | INCONTACT | UPDATE`;
- ordinary terminal release first sends an all-active UPDATE, then sends `UP`
  for terminal contacts while any surviving contacts remain UPDATE; and
- the UP coordinate is the last successfully delivered coordinate established
  by the immediately preceding UPDATE.

Disconnect, process loss, Surface/activity closure, session replacement and
normal Host shutdown share the same release owner and path: flush any pending
move, send one all-active `RELEASE_UPDATE`, then one all-active `RELEASE_UP`.
Local active state is cleared only after both calls succeed.

## Failure policy and evidence

Each initialization/injection call records stage, session, full submitted
contact count, IDs, flags, coordinates, return value, immediately captured
`GetLastError` and attempt. `ERROR_NOT_READY` alone receives a bounded maximum
of two 2 ms retries. Other errors, including `ERROR_TIMEOUT` and
`ERROR_INVALID_PARAMETER`, are never hidden or speculatively retried.

If release fails, the injected mask is retained, `RELEASE_UNCERTAIN` is written,
and a fresh session is rejected. Because failed injection calls have no general
documented partial-success guarantee, the implementation neither assumes the
target received the frame nor silently forgets the contact. An independent
Windows pointer target supplies target-side DOWN/MOVE/UP evidence for acceptance.

The production API ledger and target log are bounded and stored only in ignored
private evidence. Public source documents do not publish machine topology,
session values or unique device identifiers.
