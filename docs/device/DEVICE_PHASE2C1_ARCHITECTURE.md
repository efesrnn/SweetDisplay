# DEVICE PHASE 2C-1 — native USB transport architecture gate

Date: 2026-09-23. Result: **VERIFIED** as a read-only architecture gate.

No native USB function was enabled and no transport implementation was added.
The phone remained on its stock `mtp,adb` composition throughout the audit. The
selected direction for the first separately authorized native-USB prototype is
the stock ROM's existing **CDC-NCM plus ADB composition**, carrying the unchanged
SweetDisplay byte stream over TCP. A dedicated FunctionFS/WinUSB interface is a
later fallback or optimization path. HID remains an optional later touch-only
function; it is not the video transport.

This decision preserves the existing classifications: DEVICE PHASE 1 VERIFIED,
stock-image gate VERIFIED, DEVICE PHASE 2A PARTIAL, DEVICE PHASE 2C-0 VERIFIED,
DEVICE PHASE 2C-T PARTIAL, DEVICE PHASE 2C-T1 VERIFIED, DEVICE PHASE 2C-PERF
VERIFIED, DEVICE PHASE 2C-PERF1 PARTIAL, DEVICE PHASE 2C-PERF2
VERIFIED/TRIGGERED and DEVICE PHASE 2C-PERF3 VERIFIED.

## Exact stock baseline

The connected device was read through the already authorized ADB connection.
Only public model/build facts are recorded here; no serial, USB instance path,
session value or other unique device identifier is published.

| Item | Exact observed state | Consequence |
|---|---|---|
| Device/build | `sweet`, M2101K6G, Android 13/API 33, MIUI `V14.0.2.0.TKFTRXM` | Findings apply to this exact stock build |
| Kernel | `4.14.190-perf-g6d6db67fd446`, SELinux Enforcing | Same immutable stock kernel as earlier gates |
| Controller | `a600000.dwc3`; DWC3 dual-role and MSM glue built in | Device-mode controller exists |
| Live composition | `sys.usb.config=mtp,adb`, `sys.usb.state=mtp,adb`, `sys.usb.configfs=1`; `adbd` running | Development recovery path is currently present |
| Framework view | MTP current; connected/configured; kernel state `CONFIGURED` | Android framework and init own the live composition |
| ConfigFS | mounted at `/config`; shell is denied `/config/usb_gadget` | Level 0/1 code cannot compose the gadget directly |
| FunctionFS | ADB plus Qualcomm diagnostic FunctionFS mounts are active | FunctionFS machinery works, but no SweetDisplay instance exists |
| HID node | no `/dev/hidg0` | HID is compiled but inactive |
| Network state | no USB network interface in `/proc/net/dev` | NCM is not active |

The runtime config has `CONFIG_USB_GADGET`, `CONFIG_CONFIGFS_FS`,
`CONFIG_USB_CONFIGFS`, `CONFIG_USB_CONFIGFS_F_FS`, `CONFIG_USB_F_NCM`,
`CONFIG_USB_CONFIGFS_NCM`, `CONFIG_USB_F_HID` and
`CONFIG_USB_CONFIGFS_F_HID` enabled. ConfigFS ACM, ECM, RNDIS, EEM and UVC are
disabled in the exact running config. These are compile-time facts, not claims
that an ordinary application may activate a function.

## Ownership chain

The exact stock chain is:

1. Android `UsbDeviceManager` owns the user-visible function choice and reports
   MTP as current; ADB is added by the development state.
2. This build exposes `android.hardware.usb@1.0::IUsb/default` in the
   `hal_usb_default` SELinux domain. It is the USB port/role HAL, not an exposed
   USB Gadget HAL service.
3. Read-only `svc usb` queries report USB HAL V1.0, Gadget HAL `unknown` and USB
   speed `-1`/unknown. AOSP documents that Android 12 added Gadget HAL V1.2 NCM
   and speed APIs; this Android 13 vendor build still uses the older
   device-specific init-property path for gadget composition.
4. `sys.usb.config`, `sys.usb.state` and `sys.usb.configfs` are labelled
   `usb_control_prop`. Vendor `init.qcom.usb.rc` reacts to those properties,
   links pre-created ConfigFS functions, binds the gadget to
   `${sys.usb.controller}`, then publishes `sys.usb.state`.
5. The same init file creates `functions/ncm.0` at boot and contains complete
   `ncm` and `ncm,adb` branches. The `ncm,adb` branch retains `ffs.adb`, links
   NCM first and ADB second, binds the DWC3 controller and reports `ncm,adb`.
   This is exact stock-userspace support, not an inferred generic-kernel path.

The live UDC state/speed attributes and the gadget directory are denied to the
shell. That denial was recorded as **DENIED**; it was not bypassed. Framework
read-only speed reporting is also unavailable on this vendor implementation, so
the negotiated cable speed remains **UNKNOWN**.

## Capability and accessibility matrix

Privilege levels used below are: 0 ordinary application, 1 ADB shell,
2 privileged/system/init/HAL integration, 3 temporary custom userspace using the
existing kernel, and 4 custom kernel.

| Primitive/path | Compiled in | Stock-userspace path | Active now | Lowest plausible level | Classification |
|---|---:|---|---:|---:|---|
| DWC3 gadget + ConfigFS | Yes | Existing vendor init owns it | Yes | 2 | **VERIFIED**, shell access **DENIED** |
| Existing ADB FunctionFS | Yes | `ffs.adb` mount and init composition | Yes | Existing stock service | **VERIFIED** |
| Dedicated vendor FunctionFS bulk | Yes | No `ffs.sweetdisplay` mount, init branch or endpoint policy | No | 2 by new system integration; practically 3 for a nonpersistent prototype | **SUPPORTED / NOT STOCK-ACCESSIBLE** |
| CDC-NCM | Yes | Exact stock `ncm` and `ncm,adb` init branches | No | 2 | **SUPPORTED BY KERNEL AND STOCK INIT; LIVE ENUMERATION UNKNOWN** |
| HID gadget | Yes | No stock HID function instance, descriptor, node or composition branch | No | 2 by new system integration; practically 3 for a nonpersistent prototype | **SUPPORTED / NOT STOCK-ACCESSIBLE** |
| Existing OEM diagnostic functions | Yes/active in part | Qualcomm diag FunctionFS and proprietary functions | Partly | Existing vendor services | **NOT A SWEETDISPLAY INTERFACE** |
| UVC | No ConfigFS UVC in running config | Some generic vendor init text exists, but kernel function is disabled | No | 4 if later required | **UNAVAILABLE IN THIS STOCK KERNEL** |

Level 0 remains sufficient for MediaCodec, Surface presentation, MotionEvent
capture and TCP once an interface already exists. Level 1 can inspect the state,
but ConfigFS and UDC reads are denied and no writable composition route was
proven. The exact NCM init branch establishes a level-2 implementation path.
Level 3 is a fallback if a future nonpersistent environment must own ConfigFS
directly. Level 4 is not required for NCM, FunctionFS bulk or HID on this kernel.

## Candidate evaluation

### A. Direct vendor-specific FunctionFS bulk

The kernel primitive is present and active for ADB/diagnostics, but the stock ROM
has no generic FunctionFS endpoint available to an application. A clean design
would create one vendor-specific interface with a bulk OUT endpoint for
Host-to-phone video/control and a bulk IN endpoint for phone-to-Host telemetry/
touch. The existing 48-byte SweetDisplay header can multiplex all message types;
separate bulk endpoints for video and control are not required for the first
prototype. USB transfer boundaries must remain invisible to the parser.

Windows can use inbox WinUSB for such an interface, but automatic binding requires
the interface to advertise the `WINUSB` compatible ID through Microsoft OS
descriptors and a device-interface GUID, or otherwise requires an INF. FunctionFS
can carry OS descriptors, but no such SweetDisplay descriptor exists in stock.
Endpoint allocation with ADB and later HID is therefore **UNKNOWN** until a real
descriptor/composite design is tested. This route needs new init/SELinux policy
and a dedicated daemon; it is not the smallest first experiment.

### B. CDC-NCM

This is the selected first-native-transport direction. It has three exact-device
advantages:

- the running kernel includes the NCM function;
- the ROM already creates `ncm.0` and defines a composite `ncm,adb` branch; and
- Windows 11 provides an inbox `UsbNcm` driver for CDC subclass 0Dh. This host's
  inbox `usbncm.inf`/`usbncmum.inf` packages are present and match both the class
  identity and `USB\MS_COMP_WINNCM`.

The existing TCP `ByteStream`, handshake, CRC, exact ACK, queue bounds,
reconnect/session rules and touch profile can be reused unchanged. The ordinary
APK can remain the media/touch endpoint. Privileged work is limited to selecting
the composition and configuring an isolated USB network interface. No custom
Windows kernel driver is expected for a compliant enumeration.

Costs are network-interface addressing, firewall/profile behavior and one more
layer of TCP/IP/NCM framing. Stock init proves the USB composition, but it does
not prove IP address assignment, Windows enumeration, ADB survival or throughput.

### C. HID for touch only

Windows provides inbox HID USB/class drivers, and the stock kernel contains the
HID gadget function. A later absolute multi-touch interface is technically
possible without a custom kernel. It requires a Windows Touchscreen top-level
collection plus mandatory Contact ID, X, Y, Tip, Report ID, Contact Count and
Contact Count Maximum semantics. The existing profile-1 lifecycle can be mapped
to those reports, including last-position release and all-contact cleanup.

HID cannot carry the H.264 video plane. Adding it now would create a second
composite-function problem before the native data plane is proven. It remains a
later optional hybrid: NCM or FunctionFS for video/control, HID interrupt IN for
touch. No HID descriptor is frozen by this gate.

### D. Existing OEM/vendor functions

ADB, MTP and Qualcomm diagnostic functions are not a generic application API.
Reusing a diagnostic endpoint would couple SweetDisplay to proprietary ownership,
protocol and SELinux policy and could interfere with recovery/debugging. It is
rejected.

### E. Stock-Android userspace

The ordinary receiver APK remains valid above an already-present TCP interface.
However, the standard `svc usb` help exposes NCM only when Gadget HAL V1.2 is
supported, while this build reports Gadget HAL `unknown`. The ROM's working
candidate path is its vendor init property branch, not a proven ordinary-app or
shell API. A system/init integration is therefore required to select NCM and
configure addressing safely.

### F. Future minimal/custom userspace

The existing kernel already has every primitive needed for NCM, direct
FunctionFS bulk and HID. If the stock system cannot safely expose the NCM branch,
a separately authorized temporary userspace could configure one of those paths
without building a custom kernel. That would be privilege level 3 and must retain
an independent rollback/recovery path. It is a fallback, not the first choice.

## Windows binding and driver assessment

| Path | Windows support | New kernel driver? | Remaining proof |
|---|---|---:|---|
| NCM | Windows 11 inbox `UsbNcm` class driver; matching inbox INF packages present locally | No, if descriptors are compliant | Real `ncm,adb` enumeration and reconnect |
| Vendor bulk | Inbox `winusb.sys`; automatic match needs `WINUSB` OS compatible ID and interface GUID | No, if descriptors are correct | Descriptor, composite and async-I/O prototype |
| HID touch | Inbox `hidusb.sys` + `hidclass.sys` | No, if report descriptor is compliant | Descriptor, multi-contact reports and disconnect release |

WinUSB should use overlapped reads/writes, cancellation and bounded outstanding
transfers. The current one-frame-outstanding protocol does not require a different
wire contract. For a first direct-bulk implementation, one bidirectional bulk
pair is sufficient; HID would add an independent interrupt-IN endpoint. Exact
DWC3 endpoint availability under a FunctionFS+ADB+HID composite remains UNKNOWN.

## Bandwidth and buffering

Raw 2400x1080 BGRA at 56 FPS is approximately **4.645 Gbit/s**, so raw USB 2.0
transport is impossible. The verified encoder targets 30 Mbit/s H.264 and has
produced approximately 25.7–27.3 Mbit/s in real runs.

At 56 video frames/s, SweetDisplay adds 112 bytes per FRAME before the AU and
80 bytes per TELEMETRY acknowledgement. Even assuming 120 touch messages/s,
application framing is only about **0.164 Mbit/s**. A planning allocation of
**33.2 Mbit/s** includes the 30 Mbit/s target, that application traffic and ten
percent headroom for TCP/IP/NCM behavior. This is a budget, not a measurement.

Because negotiated link speed could not be read, acceptance must not assume the
480 Mbit/s USB 2.0 signalling number. For a future High-Speed result, use a
deliberately conservative **120 Mbit/s sustained payload budget**. The planned
33.2 Mbit/s load would consume about 28% of that budget and leave roughly 3.6x
headroom. Full-Speed would be insufficient and is an immediate stop result.

The existing 4 MiB AU maximum, three-pending-AU sender queue, one in-flight FRAME,
250 ms queue-age limit, exact ACK and IDR recovery should remain unchanged for
the first NCM adapter. NCM packets, Ethernet frames, TCP segments and USB
transfers must never become SweetDisplay message boundaries. A future WinUSB
adapter should use a small fixed pool of asynchronous transfers rather than one
unbounded request per AU.

## Remaining UNKNOWN items and blockers

- Whether Android's system-owned control path can select the dormant stock
  `ncm,adb` branch on this shipping build without root, direct ConfigFS writes or
  persistent property changes.
- Whether the exact stock NCM descriptors enumerate against Windows 11 inbox
  `UsbNcm` while ADB remains present.
- Negotiated USB speed; framework reports unknown and the shell cannot read the
  UDC speed attributes.
- NCM interface name, MAC behavior, address assignment and Windows network-
  profile/firewall behavior after real enumeration.
- Measured NCM throughput, jitter, reconnect latency, CPU use and thermal behavior.
- Exact endpoint budget and Microsoft OS descriptor behavior for a new
  FunctionFS+ADB composite.
- Actual HID report descriptor, Windows touchscreen enumeration and release
  behavior over a gadget HID path.
- A production trust/authentication model beyond the dedicated development
  device boundary.

## One selected next experiment — not executed

**Experiment:** `DEVICE PHASE 2C-1E1 — stock NCM+ADB enumeration-only`.

- **Goal:** retire the highest-value uncertainty: whether the exact dormant stock
  `ncm,adb` branch enumerates on this Windows 11 host using the inbox NCM driver
  while preserving ADB.
- **Privilege:** level 2, through the Android system-owned USB control path. If
  that path is unavailable without root, direct ConfigFS access, partition
  modification or a persistent property change, stop and record **DENIED**.
- **Temporary changes:** change only the live composition from `mtp,adb` to
  `ncm,adb`. Do not configure IP, send video, install a driver, add a firewall
  rule, create HID, or edit any persistent property in this experiment.
- **Persistent changes:** none.
- **Rollback:** first snapshot `mtp,adb`, ADB and sanitized Windows PnP state;
  retain an owner-present local recovery path; restore through the same Android
  system-owned control path to `mtp,adb`; verify ADB and the original Windows
  interfaces return. A normal stock reboot is the fallback only if the temporary
  control path loses ADB. No experiment may start without that recovery plan.
- **Risks:** temporary USB disconnect, ADB loss, failed composite enumeration,
  Windows network-adapter creation/profile state, or failure to restore the exact
  composition. Any unexpected prompt, driver request, security change or need
  for direct ConfigFS/UDC manipulation is a stop condition.
- **Success:** Windows enumerates both inbox NCM and ADB without a third-party
  driver; Android reports `ncm,adb`; ADB reconnects; no persistent USB property
  changes; rollback restores `mtp,adb` and removes the active NCM interface.
- **Failure:** NCM or ADB is absent, a non-inbox driver is requested, the state
  does not converge, rollback is incomplete, or any prohibited privilege is
  required. Do not continue to IP or throughput testing after a failure.

Only after this experiment passes should a separate authorization add ephemeral
addressing and a bounded transport-throughput gate. It must not jump directly to
video or HID.

## Custom-kernel decision

**A custom kernel is not required for the first native USB transport prototype.**
The exact running kernel already contains DWC3 gadget, ConfigFS, FunctionFS, NCM
and HID. The first blocker is privileged stock-userspace composition and policy,
not a missing kernel function. A temporary custom userspace is also unnecessary
for the selected first experiment because the exact ROM already defines
`ncm,adb`; it becomes relevant only if the system-owned path cannot expose that
branch safely.

## Sources and safety closeout

Primary design references:

- [AOSP USB HAL implementation guidance](https://source.android.com/docs/core/permissions/usb-hal)
- [Linux ConfigFS gadget documentation](https://docs.kernel.org/usb/gadget_configfs.html)
- [Linux FunctionFS documentation](https://docs.kernel.org/usb/functionfs.html)
- [Linux HID gadget documentation](https://docs.kernel.org/usb/gadget_hid.html)
- [Microsoft inbox USB class drivers](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/supported-usb-classes)
- [Microsoft automatic WinUSB installation](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/automatic-installation-of-winusb)
- [Microsoft Windows touchscreen HID requirements](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/touchscreen-required-hid-top-level-collections)

No ConfigFS write, property write, service start/stop, function creation,
UDC unbind/rebind, ADB restart, USB reset, NCM/HID activation, driver install,
root/su/adb-root, remount, SELinux change, Fastboot action, partition/AVB action,
boot-image action or persistent device setting change occurred. Nothing was
written to the phone. Raw device/machine output, proprietary firmware/images and
unique identifiers remain outside publishable paths. Final `git diff --check`
and publication hygiene pass: 215 publishable paths, the index and 125 reachable
history blobs were checked; only the evidence README is tracked, prohibited
artifact paths are absent and the privately read connected-device identifier has
zero public matches. Hard stop after this gate.
