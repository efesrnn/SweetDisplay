# DEVICE PHASE 2C-1E1 — stock NCM+ADB enumeration-only

## Result

**DEVICE PHASE 2C-1E1: DENIED**

The exact stock, system-owned framework control path did not select NCM at the
authorized privilege boundary. The live request was attempted once, the phone
never left `mtp,adb`, Windows never enumerated an NCM interface, and the required
rollback/final-state verification passed. This result does not change the
separate **DEVICE PHASE 2C-1 architecture gate: VERIFIED** classification.

## Scope and recovery gate

The owner was present and had authorized this bounded experiment. Before the
request, the connected phone was physically available, Android was awake, ADB
reported `device`, `adbd` was running, and both `sys.usb.config` and
`sys.usb.state` were exactly `mtp,adb`. There was no active ADB forwarding or
phone/USB test. An unrelated Windows-side `SweetDisplayDevice` simulator was
running but did not use the phone or USB transport.

The primary rollback was established as the same stock framework path used for
the experiment: `svc usb setFunctions mtp`. With USB debugging already enabled,
the framework retains/adds ADB and the expected converged state is `mtp,adb`.
A normal stock reboot was reserved only as a fallback if ADB and the control path
were lost; it was not needed.

The baseline Windows state contained a healthy inbox `WINUSB` ADB interface and
a healthy `WUDFWpdMtp` MTP device, both with problem code zero. No NCM PnP device
or NCM network adapter was active. The 2C-1 read-only gate had already established
the presence of the Windows inbox `UsbNcm` packages; no package was reinstalled.

## Authorized request and observation

At `2026-09-23T00:17:27.560+03:00`, the single allowed live request was issued:

```text
svc usb setFunctions ncm
```

This is Android's stock framework-owned USB control command. It does not write
ConfigFS directly or manually reproduce the vendor init branch. The command
returned exit code 255. MIUI also emitted its previously observed missing
`theme_compatibility.xml` diagnostic. More importantly, throughout the bounded
observation:

- `sys.usb.config` remained `mtp,adb`;
- `sys.usb.state` remained `mtp,adb`;
- `adbd` remained `running` and ADB remained `device`;
- no ADB disconnect/reconnect was observed;
- no NCM-specific phone network interface appeared;
- no Windows NCM PnP device or network adapter appeared;
- no Windows NCM driver/service bound; and
- the framework USB-speed query remained `-1`, so negotiated speed is UNKNOWN.

The runtime continues to report Gadget HAL `unknown`, while the stock `svc usb`
help advertises NCM only when Gadget HAL 1.2 is supported. The observed request
did not result in entry to the ROM's dormant `ncm,adb` composition through this
authorized high-level path. The exact internal rejection point was not exposed;
the HAL/help mismatch is corroborating context, not a stronger causal claim.
Selecting the vendor init branch by writing USB
properties, ConfigFS or the UDC directly would cross the explicit prohibition
and was not attempted. No level-3 fallback was attempted.

No IP address, DHCP operation, route, metric, network profile, firewall setting,
ping, TCP socket, SweetDisplay traffic or throughput test was performed. Windows
was not asked to install, update or replace a driver.

## Mandatory rollback and final state

Rollback began at `2026-09-23T00:19:13.296+03:00` through the same stock path:

```text
svc usb setFunctions mtp
```

It returned success. By `2026-09-23T00:19:16.372+03:00`, all rollback checks had
passed:

- `sys.usb.config = mtp,adb`;
- `sys.usb.state = mtp,adb`;
- `init.svc.adbd = running`;
- host ADB state = `device`;
- no active/healthy NCM PnP device remained;
- no NCM network adapter was active;
- the ADB `WINUSB` and MTP `WUDFWpdMtp` devices were healthy with problem code
  zero;
- `persist.sys.usb.config` was unchanged;
- `persist.vendor.usb.config` was unchanged; and
- the third-party Windows network-driver inventory was byte-for-byte unchanged.

The reboot fallback was not used. No persistent phone USB, Windows network or
security configuration was introduced.

## Classification and remaining unknowns

The precise gate result is **DENIED**, rather than FAILED: the legitimate stock
system-owned path available to the authorized shell could not select `ncm,adb`,
and every lower-level route that could force the dormant init branch was outside
the approved privilege boundary. Because no transition occurred, the experiment
cannot answer whether the exact NCM descriptors would enumerate with inbox
`UsbNcm`, whether ADB would coexist after a real NCM transition, or what USB speed
would be negotiated.

This result does not verify IP transport, TCP over NCM, SweetDisplay over NCM,
NCM throughput/latency, production native USB, HID, a SweetDisplay FunctionFS
interface or camera behavior. DEVICE PHASE 2C-1E2 was not begun.

## Safety and evidence handling

No root, `su`, `adb root`, remount, SELinux change, direct ConfigFS write,
FunctionFS creation, manual NCM/HID creation, UDC bind/unbind, descriptor edit,
service stop/start, ADB-server restart, Fastboot, bootloader, partition, AVB,
kernel, driver-installation, firewall or network-configuration operation was
performed. The only phone-state requests were the authorized stock framework
selection attempt and its mandatory stock-framework rollback. Nothing was
flashed and no persistent phone change was made.

Public documentation contains no device serial, USB instance identity, MAC
address or machine-specific unique identifier. Sanitized run evidence is stored
only below the ignored private evidence tree. Raw device evidence and proprietary
firmware/images/archives remain private and ignored.
