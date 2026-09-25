# DEVICE PHASE 2C-1E2 — ephemeral NCM IP + TCP data-plane proof

## Result

**DEVICE PHASE 2C-1E2: BLOCKED**

The locked/noninteractive screen-off condition again established the verified
stock USB foundation: Android reached `ncm,adb`, created `usb0`, retained ADB,
and Windows enumerated a healthy Microsoft inbox `UsbNcm` adapter with problem
code 0. The least-invasive stock runtime address request available to the shell,
however, returned exit 1 and did not assign an IPv4 address to `usb0`:

```text
adb shell ndc interface setcfg usb0 10.77.77.2 30 up
```

No diagnostic text was returned. The controller independently confirmed that
`usb0` still had no IPv4 address. Under the authorized boundary, the next ways
to force an address would require a different privileged/system owner or a
prohibited escalation. The gate therefore stopped before assigning the Windows
peer address, before ping, and before TCP. No root, custom userspace or policy
change was attempted.

Historical classifications are unchanged: 2C-1 is **VERIFIED**, 2C-1E1 is
**DENIED**, 2C-1E1A is **VERIFIED**, 2C-1E1B and 2C-1E1C are **PARTIAL**, and
2C-1E1D is **VERIFIED**. This E2 result does not weaken the verified NCM
enumeration or bounded screen-off stability results.

## Baseline and prepared proof

The live accepted run began with:

- `sys.usb.config=mtp,adb` and `sys.usb.state=mtp,adb`;
- host ADB state `device` and phone `adbd=running`;
- phone wakefulness `Dozing` and built-in display state `OFF`;
- no phone `usb0` and no active Windows NCM device;
- no ADB forward or reverse entry; and
- no active host route in `10.0.0.0/8`.

The non-conflicting ephemeral design selected `10.77.77.0/30`, with the planned
Windows peer `10.77.77.1` and Android peer `10.77.77.2`. It specified no gateway,
DNS, DHCP, NAT, bridge, Internet sharing or persistent route.

A narrow non-video test path was prepared and validated offline. The Android
receiver has an explicit NCM diagnostic mode that listens on ordinary userspace
TCP address `0.0.0.0:48231`, forces ACK-only operation and does not request
`FLAG_KEEP_SCREEN_ON`. The Windows probe uses the existing `ByteStream`,
nonblocking TCP and 48-byte SWDP framing. It is bounded to two connections, eight
deterministic HEARTBEAT request/echo pairs per connection and a clean DRAIN/ACK
on each connection. That would exercise HELLO/CAPABILITIES, session identity,
sequence ordering, exact eight-byte payload integrity, bidirectional messages,
clean close and one reconnect without H.264. Protocol CRC remains coupled to the
FRAME/H.264 path and was deliberately not exercised.

The updated Android APK built, signed and verified locally. The EWDK build of the
Windows probe passed `/W4 /WX`, and the existing 58,235 deterministic
protocol/parser/queue/resync regression checks passed. These are preparation
results, not live NCM data-plane evidence.

The updated SweetDisplay development APK was installed with the owner's existing
ordinary `adb install` authorization. It remains installed after the run, while
its bounded activity/process was force-stopped during cleanup. This project APK
update is the only persistent phone-content change in E2; it is not a persistent
USB, network or security configuration change.

## Live NCM and address result

Exactly one stock request was used in the accepted run:

```text
adb shell svc usb setFunctions ncm
```

Independent observation established:

- Android config and state `ncm,adb`;
- `adbd=running` and host ADB state `device`;
- phone `usb0` present;
- Windows `UsbNcm Host Device` present;
- service `UsbNcm`, provider Microsoft, status OK and problem code 0; and
- NCM network interface index available to the controller.

The Windows adapter initially reported media-disconnected state, which is
expected before peer Layer-3 setup and is not a PnP health failure. No driver was
installed or updated.

The subsequent stock `ndc interface setcfg` command returned exit 1 with empty
output. A read-only `ip -o -4 addr show dev usb0` check remained empty. This is
the exact stop reason. The controller issued no `New-NetIPAddress`, and neither
`10.77.77.1/30` nor its connected route was ever created on Windows.

Because the peer addresses did not exist:

- Windows-to-Android and Android-to-Windows ping were not run;
- no TCP connection was attempted;
- Windows-to-Android and Android-to-Windows application payload are unproven;
- ordering, framing and reconnect are validated only offline, not over NCM;
- total live application test data was zero bytes; and
- no performance or latency conclusion is made.

ADB forward and reverse lists were empty before the request, at the stop, and
after recovery. ADB remained only a control, observation and recovery channel;
it carried no tested application payload.

## Recovery and final state

The bounded Android test component was stopped. The stock `ndc clearaddrs usb0`
cleanup path returned successfully even though no test IPv4 address had been
created. No Windows test address or route existed to remove. The normal stock
rollback request then restored `mtp,adb`.

Final independent checks established:

- `sys.usb.config=mtp,adb` and `sys.usb.state=mtp,adb`;
- ADB state `device` and `adbd=running`;
- phone `usb0` absent;
- Windows NCM inactive;
- healthy Windows ADB and MTP, both problem code 0;
- no `10.77.77.1` address and no `10.77.77.0/30` route;
- empty ADB forward and reverse lists;
- unchanged default routes, DNS configuration and active network profiles;
- unchanged Windows driver-package inventory; and
- unchanged persistent system/vendor USB properties.

No cleanup or rollback error occurred. No firewall rule, profile customization,
gateway, DNS, DHCP, NAT, bridge or Internet-sharing state was introduced.
The updated development APK remains installed as stated above.

## Preserved preparatory attempts

Two earlier private controller attempts remain preserved rather than overwritten.
The first stopped at a PowerShell empty-collection check before any NCM request.
The second reached `ncm,adb` but stopped before IP because an overly strict
`NetEnabled` observer filter did not accept the unaddressed NCM adapter. It made
no IP or TCP mutation and rolled back cleanly. The accepted third run removed
that observer assumption, positively captured the healthy NCM adapter, and then
reached the genuine stock Android address-ownership blocker described above.

## Scope, safety and remaining unknowns

No root, `su`, `adb root`, remount, persistent property, ConfigFS/UDC, manual
gadget, SELinux, service/package disabling, input injection, Fastboot,
partition/AVB, Magisk, recovery, kernel, boot-image, Windows driver, firewall,
ICS/NAT/bridge, default-route, DNS, persistent network, ADB tunnel, iperf, video,
H.264, display streaming, HID, FunctionFS or camera operation occurred. No
commit or push occurred.

Remaining unknowns are the exact silent netd denial reason, whether an existing
legitimate stock system owner exposes a nonpersistent `usb0` address API within
the authorized boundary, peer Layer-3 reachability, real TCP over NCM, live SWDP
framing/integrity/reconnect over NCM, negotiated USB speed and all performance
properties.

The recommended next bounded phase is **DEVICE PHASE 2C-1E2A — STOCK USB0
ADDRESS-OWNERSHIP DIAGNOSIS**: read-only framework/netd policy and exact-ROM
analysis to identify whether a legitimate nonpersistent system-owned address
path exists. It must not escalate to root or begin E3. E3 media transport is not
recommended until E2 can pass.

## Repository closeout

`git diff --check` passed with only the repository's normal line-ending notices.
The existing publication checker passed for 223 publishable working-tree files,
the index and 125 reachable history blobs. Targeted scans of E2's public code and
documents found zero personal filesystem paths, MAC addresses, ADB-key markers,
PnP instance identifiers or connected-device serials. Raw controller output,
logs and machine/device identity remain below the ignored private evidence tree.

Public E2 changes are this result, the justified STATUS/ROADMAP/TEST_LOG/USB plan
updates, the narrow Windows NCM protocol probe and its build script, the generic
IPv4 TCP endpoint support, and the Android receiver's explicit NCM diagnostic
bind mode. The ignored private controller and evidence are not publication
content. No commit or push occurred.
