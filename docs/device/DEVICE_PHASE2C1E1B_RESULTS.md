# DEVICE PHASE 2C-1E1B — controlled hands-off stock NCM+ADB enumeration replay

## Result

**DEVICE PHASE 2C-1E1B: PARTIAL**

The exact stock Level-2 path reproducibly reached `ncm,adb`. Android created a
phone-side `usb0` interface, Windows enumerated a healthy CDC-NCM adapter using
the Microsoft inbox `UsbNcm` service, MTP disappeared, and ADB returned while
NCM was active. The composition did not remain stable for the full 30-second
hands-off window: an input DOWN/UP sequence was recorded in Xiaomi's automatically
opened USB details activity and was immediately followed by a new framework
request for `mtp,adb`.

The owner had explicitly confirmed hands-off operation and no deliberate phone
interaction was requested or performed as part of the procedure. The origin of
the recorded input event—physical, unintended/ghost, or another input source—is
**UNKNOWN**. Because the stability and no-second-actor criteria are not met,
E1B cannot be VERIFIED. Stock rollback and all final health checks passed.

The historical classifications remain unchanged:

- DEVICE PHASE 2C-1 architecture gate: **VERIFIED**;
- DEVICE PHASE 2C-1E1: **DENIED**; and
- DEVICE PHASE 2C-1E1A: **VERIFIED**.

## Baseline and recovery gate

Before mutation, the phone reported `sys.usb.config=mtp,adb`,
`sys.usb.state=mtp,adb`, `init.svc.adbd=running` and host ADB state `device`.
No phone USB-network interface was present. There was no ADB forward/reverse
mapping and no established SweetDisplay TCP session.

Windows had healthy problem-code-0 ADB and MTP devices. No active `UsbNcm`
device or NCM network adapter was present. The inbox NCM package was not
reinstalled or updated. Primary rollback through the same stock framework path
was prepared; normal stock reboot remained emergency-only and was not needed.

All Android, PnP and network-adapter observers were initialized before the live
request. Raw evidence is retained only below the ignored private evidence tree.

## Request and independent timeline

Exactly one authorized NCM request was issued:

```text
adb shell svc usb setFunctions ncm
```

The request process exited 255 when its own ADB transport was interrupted. As
established by E1A, this exit is transport evidence and not a framework rejection.

The independent observation sequence was:

1. Android framework logged `Setting USB config to ncm,adb`.
2. The phone independently reported both config and state as `ncm,adb`.
3. The new phone interface `usb0` appeared.
4. MTP disappeared from Windows.
5. Windows enumerated a healthy NCM network device and healthy ADB concurrently.
6. Xiaomi Settings automatically opened `Settings$UsbDetailsActivity`.
7. The activity received an input DOWN followed by UP.
8. Approximately 130 ms after the UP event, Android framework logged
   `Setting USB config to mtp,adb` and the activity closed by app request.
9. The phone and Windows returned to MTP+ADB before the 30-second observation
   window ended.

Host polling observed the NCM device in two consecutive Windows samples. Android
polling captured one fully converged `ncm,adb` sample before the second transition
temporarily removed ADB. The available evidence places the stable NCM interval at
only a few seconds; it did not satisfy the full hands-off window.

## Windows enumeration result

The live NCM binding was:

| Field | Observed value |
|---|---|
| Device description | `UsbNcm Host Device` |
| Service | `UsbNcm` |
| Provider/manufacturer | Microsoft |
| INF | `usbncm.inf` |
| Driver version | `10.0.26100.9444` |
| Signed | Inbox Microsoft package |
| Device health | Healthy |
| Problem code | 0 |

No manual or third-party driver was requested or installed. The driver-package
inventory was identical before and after the experiment. Windows retained the
normal inactive PnP history record for the briefly enumerated device, but no NCM
adapter remained enabled or active after rollback.

During `ncm,adb`, Windows ADB was healthy and MTP was absent. After return to
`mtp,adb`, NCM ceased to be active and the original healthy ADB/MTP state returned.

## Phone result and USB speed

The phone-side NCM interface name was `usb0`; no MAC address is published. ADB
returned while Android still reported `ncm,adb`. Framework USB-speed reporting
returned `-1`, so negotiated speed remains **UNKNOWN**.

No address was assigned manually. No DHCP request, route, metric, firewall,
network profile, ping, TCP socket, iperf, SweetDisplay payload or throughput
operation was performed. Any automatic operating-system link-local behavior was
observation-only.

## Rollback and final state

The mandatory primary rollback command was issued once through the same stock
system-owned path:

```text
adb shell svc usb setFunctions mtp
```

It returned 0 and converged to:

- `sys.usb.config=mtp,adb`;
- `sys.usb.state=mtp,adb`;
- `adbd=running`;
- host ADB state `device`;
- no active Windows NCM adapter;
- healthy Windows ADB and MTP, problem code 0; and
- no active-profile or enabled-adapter change from baseline.

`persist.sys.usb.config` and `persist.vendor.usb.config` were unchanged. The
Windows driver-package inventory was unchanged. The emergency reboot fallback
was not used.

## Classification and remaining unknowns

E1B is **PARTIAL**, matching the defined case where transition/enumeration occurs,
a non-safety acceptance condition remains incomplete, and rollback succeeds.
The following success criteria were established: stock transition, Android NCM
state, phone NCM interface, Windows inbox NCM binding, healthy problem code,
ADB coexistence, MTP disappearance, no prohibited privilege, and complete stock
rollback.

The following criteria were not established:

- `ncm,adb` stability for the full 30-second hands-off window;
- absence of a second UI/input-driven actor; and
- negotiated USB speed.

The source of the recorded DOWN/UP event remains UNKNOWN. A later experiment
must not be inferred or executed from this result without separate authorization.
No IP connectivity, TCP transport, throughput, latency, video, HID, dedicated
FunctionFS interface or camera behavior is verified here.

## Safety and evidence closeout

No root, `su`, `adb root`, remount, direct `setprop`, ConfigFS/FunctionFS write,
manual NCM/HID creation, UDC bind/unbind, descriptor change, SELinux change,
service restart, ADB-server restart, Fastboot, partition/AVB operation, custom
kernel/userspace, Windows driver install/update, certificate/security change or
manual network change occurred. The only composition operations were the single
authorized stock NCM request and mandatory stock rollback. No commit or push was
performed.
