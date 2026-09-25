# ADB-forwarded SweetDisplay transport

DEVICE PHASE 2C-0 and the bounded DEVICE PHASE 2C-T experiment use the existing
authorized ADB function over the physical USB cable. Wi-Fi is not part of the
path and the Android USB gadget remains `mtp,adb`.

## Deterministic topology

The existing Windows sender is a TCP client, while the Android receiver is a
loopback-only listener. The matching topology is therefore host-side ADB
forwarding:

```text
Windows SweetDisplayHost
  connect 127.0.0.1:48231
       |
       | adb forward tcp:48231 tcp:48231
       v
Android receiver
  listen 127.0.0.1:48231
```

Setup and cleanup are:

```powershell
adb forward tcp:48231 tcp:48231
adb forward --remove tcp:48231
```

The test controller resolves the explicitly selected Android SDK `adb.exe`,
checks one authorized device through `adb get-state`, verifies
`sys.usb.config=mtp,adb`, installs no service and removes the forward during
cleanup. `adb forward --list` is empty after the completed test. A forward is
ephemeral machine state and is never assumed to survive ADB/server/device
restart.

## Byte-stream and connection lifecycle

ADB/TCP packet or read boundaries do not delimit SweetDisplay messages. Both
header and payload are assembled with repeated bounded reads. The Windows Host
creates a new random protocol session for each ready TCP connection. The Android
listener returns its HELLO and CAPABILITIES, validates the Host capabilities,
then emits one exact TELEMETRY acknowledgement for each admitted FRAME.

An ordinary disconnect invalidates the sender's session, aborts its bounded
pending work and closes the Android socket. While the listener is absent,
connection attempts can be accepted by the ADB forward and then closed; these
are counted socket errors, not ready sessions. When the Activity returns, a new
TCP connection and protocol session are established. The sender rejects/skips
dependent AUs until IDR+SPS+PPS is present. Final shutdown sends CONTROL DRAIN;
the receiver drains MediaCodec and replies DRAIN_ACK before the Host exits.

For touch profile 1, the new connection first negotiates its own session and
configuration/ready exchange. A contact message from an older session is fatal
and cannot be accepted by the fresh connection. On disconnect the transport
worker synchronously ends every active Windows contact before invalidating the
session. It clears local injected state only after the all-active UPDATE and UP
both succeed; otherwise it preserves uncertain state and blocks a fresh touch
session. DEVICE PHASE 2C-T remains historically PARTIAL, while the separate
2C-T1 release-reliability gate verified this corrected behavior under repeated
real reconnects. It did not use a different USB mechanism.

## Measured transport results

The 60-second 2400x1080 target run ACKed all 3,250 transmitted frames, sent
203,513,300 bytes, had queue peak three and recorded no overflow, socket error or
protocol error.

The accepted 600-second conservative run measured:

| Counter | Value |
|---|---:|
| Frames seen / admitted / wire-complete / ACKed | 21,190 / 21,020 / 21,017 / 21,016 |
| Disconnected / resync-skipped / overflow | 164 / 5 / 1 |
| Queue-aborted / unconfirmed / pending | 3 / 1 / 0 |
| Queue depth/byte peak | 3 / 75,210 |
| Ready protocol sessions | 2 |
| Socket / protocol errors | 31 / 0 |
| Messages sent / received | 21,642 / 21,610 |
| Bytes sent / received | 527,987,922 / 1,714,624 |
| Transport time / sender FPS | 600.239 s / 35.014 |

The nonzero disconnect/socket/resync counters are localized to the intentional
3.124-second receiver absence. Exact accounting closed with zero pending work.
The single queue overflow was bounded and invalidated dependent state; recovery
waited for a valid recovery AU. The new Android process decoded and drained
12,210 frames after reconnect, and the operator confirmed that visible motion
returned without restarting the Windows virtual display driver.

Windows and Android clocks are independent. These counters prove byte/session
integrity and bounded flow; they do not provide end-to-end latency. Windows QPC
and Android `System.nanoTime` must not be subtracted.

## Safety boundary

ADB forwarding neither creates nor changes a gadget function. This phase made
no ConfigFS write, FunctionFS endpoint, NCM/HID configuration, UDC unbind/rebind,
descriptor change or ADB disable. It also made no root, remount, SELinux, AVB,
partition, recovery, Magisk or fastboot change. Dedicated gadget work remains a
separate Phase 2C-1-or-later authorization gate.
