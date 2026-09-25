# DEVICE PHASE 2C-T — real touch transport and Windows input

**Status: PARTIAL.** This is a bounded stock-Android/ADB development experiment,
not a USB-gadget or production-input result. It stops here.

## Scope and unchanged safety boundary

The owner had already enabled the Android development-install setting manually
and retained it as the dedicated-device baseline. This phase used the existing
ordinary receiver application and ephemeral ADB TCP forwarding. Before and after
each live controller run, the Android USB composition was `mtp,adb`.

Nothing changed Android USB functions, ConfigFS, FunctionFS, NCM, HID, UDC,
input nodes, SELinux, root state, partitions, AVB, recovery or Fastboot. No
custom image was built, booted or installed. No new Windows driver, certificate,
security setting or monitor association was created. Raw Android/Windows logs,
desktop topology, package material and local evidence remain ignored/private.

## Implemented bounded profile

Protocol profile 1 is enabled only when both endpoints negotiate it explicitly.
It uses a 32-byte configuration/ready record followed by 32-byte real contact
records. A record contains a bounded contact slot, action, normalized content
coordinates, optional pressure, active-contact mask and device monotonic time.
The Windows host dynamically discovers the active SweetDisplay target and binds
the session to its opaque topology token and geometry. It maps normalized content
coordinates exactly once into that target; no display number, desktop origin or
fixed rotation is embedded in the protocol.

Malformed records, invalid contact transitions, topology changes, a message
before ready, or a message whose session differs from the active connection are
fatal. The protocol test suite now creates a fresh touch-capable connection and
proves that a correctly formed contact record from the retired session is
rejected. The test build and deterministic suite pass.

## Live observations

The completed single-/multi-contact injection run had a clean Host and test
target exit, one ready connection, zero protocol errors, clean H.264/video
classification, and `mtp,adb` before/after. It accepted 103 real phone contact
messages. The independent target received the expected down/move/up lifecycle
for the mapped corner/drag controls and for two independently released contacts.
The operator completed the requested phone interaction checks. These results
establish that real `MotionEvent` data, rather than synthetic host input, reached
the correct Windows target through the negotiated link.

The controlled reconnect run forced the receiver process to stop while one
contact was active, then started a new receiver process. It had two ready
connections, zero protocol errors, clean video classification, one real touch
record before the interruption, and `mtp,adb` before/after. The independent
target recorded the original contact's terminal UP. The new process negotiated a
fresh session; protocol tests independently reject a retired-session contact.

## Why the result is PARTIAL

The external target did not retain an active contact, which is the required
safe outcome. But the host evidence also records `ERROR_TIMEOUT`, followed by
`ERROR_INVALID_PARAMETER`, while submitting the release update around that
controlled disconnect. The current host then clears its local active-contact
state. A target-side UP is strong evidence of release, but it is not a clean
Windows API acknowledgement of every intended release call.

Therefore the following are verified: real one-contact transport/injection,
correct target mapping for the tested controls, real two-contact delivery,
fresh reconnect, protocol-level stale-session rejection, and target-side release
of the interrupted contact. The following remains unknown: why the Windows API
reported the release-update errors and whether every equivalent timing race gets
the same clean release acknowledgement. No acceptance criterion was weakened to
mask that ambiguity.

## Required stop

No Phase 2C-1, Phase 3E, USB HID/configuration, native input, calibration or
other follow-on device work was started. Any future attempt must begin with a
new explicit authorization and retain this evidence, including the failed API
acknowledgement, rather than relabeling it as a pass.
