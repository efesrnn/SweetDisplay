# DEVICE PHASE 2C-1E1A — stock NCM control-path diagnosis

## Result

**DEVICE PHASE 2C-1E1A: VERIFIED**

Read-only reconstruction localized the stock ownership chain and the event that
overwrote the short-lived NCM request. The existing **DEVICE PHASE 2C-1
architecture gate: VERIFIED** and historical **DEVICE PHASE 2C-1E1: DENIED**
classifications are preserved. This diagnosis does not verify Windows NCM
enumeration, IP transport, TCP over NCM or SweetDisplay over NCM.

The previous gate's host-side polling did not observe NCM. Retained Android
runtime records now show that the request nevertheless reached the legacy USB
controller, selected `ncm,adb`, disconnected/reconfigured the gadget and allowed
ADB to reconnect. A later, separate framework request selected `mtp,adb` again.
That second request is strongly localized to an interaction in Xiaomi's USB
details activity; the exact clicked preference is not recorded and remains
**UNKNOWN**.

## Exact reconstructed control path

The exact shipping `/system/bin/svc` script starts
`com.android.commands.svc.Svc` from `/system/framework/svc.jar`. Read-only
inspection of that exact JAR shows this path:

```text
adb shell
  -> /system/bin/svc usb setFunctions ncm
  -> UsbCommand.run()
  -> UsbManager.usbFunctionsFromString("ncm") = FUNCTION_NCM (1 << 10)
  -> IUsbManager.setCurrentFunctions(1024)
  -> system_server / UsbService (MANAGE_USB permission check)
  -> UsbDeviceManager
  -> UsbHandlerLegacy (no USB Gadget HAL service exists)
  -> add ADB because USB debugging is enabled
  -> sys.usb.config = none, then ncm,adb
  -> vendor init property trigger in init.qcom.usb.rc
  -> ConfigFS NCM + FunctionFS ADB links
  -> DWC3 UDC bind
  -> sys.usb.state = ncm,adb
```

The exact Xiaomi `UsbCommand` bytecode is materially the same as the matching
[AOSP Android 13 UsbCommand](https://android.googlesource.com/platform/frameworks/base/+/refs/tags/android-13.0.0_r28/cmds/svc/src/com/android/commands/svc/UsbCommand.java):
the client parses the string, calls `IUsbManager.setCurrentFunctions`, catches
only a Binder `RemoteException`, and does not synchronously wait for or return
the eventual USB composition result.

At the framework level, Android 13 defines `ncm` as `FUNCTION_NCM = 1 << 10`,
includes it in the settable function mask and accepts it in
`usbFunctionsFromString`; see the pinned
[UsbManager source](https://android.googlesource.com/platform/frameworks/base/+/refs/tags/android-13.0.0_r28/core/java/android/hardware/usb/UsbManager.java).
The service permission/mask checks are represented by the pinned
[UsbService source](https://android.googlesource.com/platform/frameworks/base/+/refs/tags/android-13.0.0_r28/services/usb/java/com/android/server/usb/UsbService.java).

The generic help text says NCM requires Gadget HAL 1.2, and the HAL-backed
`UsbHandlerHal` path rejects NCM below that version. That is not this phone's
runtime path. The exact runtime reports no Gadget HAL, so
[UsbDeviceManager](https://android.googlesource.com/platform/frameworks/base/+/refs/tags/android-13.0.0_r28/services/usb/java/com/android/server/usb/UsbDeviceManager.java)
selects the legacy property-based handler. Exact Xiaomi bytecode confirms the
legacy handler waits up to three seconds per state transition, adds ADB, writes
`sys.usb.config` non-persistently and restores the previous/default composition
if convergence fails.

## Meaning and limits of exit 255

Exit 255 came from the host `adb shell` process losing its transport while the
phone deliberately disconnected and reconfigured USB. It does not establish a
client parser rejection, permission denial, Gadget HAL rejection, property
failure or init failure. The exact `UsbCommand` has no code that returns the
asynchronous composition outcome.

The retained event order is decisive:

1. `UsbDeviceManager` logged `Setting USB config to ncm,adb`.
2. USB gadget events logged disconnect, reconnect and configured transitions.
3. ADB debugging logged the already-authorized host reconnecting.
4. No `Failed to switch USB config`, `Failsafe 1`, `Failsafe 2` or
   `Unable to set any USB functions` record exists in the retained interval.
5. About four seconds later `UsbDeviceManager` logged a second request:
   `Setting USB config to mtp,adb`.

Therefore the initial NCM path has no proven rejection boundary: it completed
far enough to publish and enumerate a USB configuration on Android. The exact
Windows interfaces during that brief interval were not captured, so Windows NCM
enumeration remains **UNKNOWN**.

## Exact Xiaomi init branch

The exact stock vendor file is `/vendor/etc/init/hw/init.qcom.usb.rc`.
At boot it creates `functions/ncm.0`. Its NCM+ADB branch is triggered by:

```text
sys.usb.config=ncm,adb
sys.usb.configfs=1
sys.usb.ffs.ready=1
```

The first trigger starts `adbd`. When FunctionFS ADB is ready, the branch:

- sets the configuration string to `ncm_adb`;
- removes prior function links;
- selects the branch's Qualcomm vendor/product identity;
- links `functions/ncm.0` as `f1`;
- links `functions/ffs.adb` as `f2`;
- binds `${sys.usb.controller}` to the UDC; and
- publishes `sys.usb.state=${sys.usb.config}`.

The companion `ncm` branch links only NCM and publishes state. General cleanup
passes through `sys.usb.config=none`, unbinds the UDC, removes function links,
stops ADB where applicable and resets the FunctionFS-ready state. No direct
ConfigFS or UDC operation was performed during this phase.

## Property and SELinux ownership

The exact stock property contexts label `sys.usb.config`, `sys.usb.state`,
`sys.usb.configfs` and `sys.usb.controller` as `usb_control_prop`;
`sys.usb.ffs.ready` is `ffs_control_prop`.

The shell is granted `android.permission.MANAGE_USB`, which permits the Binder
request to the USB service. It does not directly own the USB properties. The
pinned Android 13 policy explicitly grants
[`system_server` permission to set `usb_control_prop`](https://android.googlesource.com/platform/system/sepolicy/+/refs/tags/android-13.0.0_r28/private/system_server.te),
while the corresponding Android 13 `shell.te` contains no
`set_prop(shell, usb_control_prop)` grant. Direct shell property writes were not
tested; on this enforcing user build they are expected to be denied, but the
exact write result remains deliberately untested.

The proven operating ownership is:

- shell: authorized Binder client only;
- `system_server`: validates the request and sets the transient USB control
  property through `UsbHandlerLegacy`;
- property service plus init/vendor-init: matches the trigger and performs the
  predefined ConfigFS/UDC actions; and
- `adbd`: supplies the existing FunctionFS ADB endpoint.

Old boot audit records show a separate Qualcomm init-shell domain denied when it
tried to set USB control properties or access ConfigFS. That domain is not the
successful owner and was not used here.

## HAL and service inventory

The phone exposes `android.hardware.usb@1.0::IUsb/default` from
`android.hardware.usb@1.0-service` in the `hal_usb_default` domain. This is the
USB port HAL, and `dumpsys usb` reports port HAL version 1.0.

No HIDL or AIDL `IUsbGadget` service is registered. Runtime framework records
explicitly say `USB GADGET HAL not present in the device`, and
`svc usb getGadgetHalVersion` reports `unknown`. No separate vendor Binder
gadget service exposing an NCM selector was found. The shipping ROM therefore
uses the framework legacy-property path for gadget composition, not a Gadget
HAL path.

## Source of the second MTP request

The retained runtime sequence shows Xiaomi's `UsbModeChooserReceiver` starting
`Settings$UsbDetailsActivity` immediately after USB reached configured state.
An input-interaction record for that activity occurs immediately before the
second framework request for `mtp,adb`.

Read-only inspection of the exact Settings APK shows:

- `UsbModeChooserReceiver` opens the USB details activity when USB is connected,
  configured and unlocked;
- `UsbDetailsFunctionsController.onRadioButtonClicked` maps the selected row and
  calls `UsbBackend.setCurrentFunctions`;
- `UsbBackend.setCurrentFunctions` calls `UsbManager.setCurrentFunctions`;
- the visible map contains MTP, RNDIS, MIDI, PTP and charging; and
- NCM is not a visible preference in that map.

This strongly localizes the overwrite to a user interaction in the stock Xiaomi
USB details UI. The retained data does not identify the exact row touched, so
that final detail is **UNKNOWN**. It is not evidence that Android automatically
rejects NCM.

## Shipping-ROM intent

The NCM branches use a Qualcomm-generic init file that also contains many
diagnostic and networking compositions. Xiaomi Settings exposes RNDIS tethering
but not NCM, and no other stock init trigger or service explicitly requesting
NCM was found. The best supported classification is **inherited generic Qualcomm
gadget composition with network/tethering capability**. A particular shipping
Xiaomi feature, factory tool or intended caller is **UNKNOWN**; the branch's
presence alone does not prove normal product exposure.

Android's standard HAL definition also assigns NCM to the 1.2 gadget function
set; see the pinned
[IUsbGadget 1.2 types](https://android.googlesource.com/platform/hardware/interfaces/+/7735ba5ea97f87329ce1adb65acd4fc58722f7fd/usb/gadget/1.2/types.hal).
That standard definition is architectural context, not evidence that this ROM
ships a Gadget HAL 1.2 service.

## Level-2 decision and next phase

Outcome **A — YES**: an existing legitimate stock Level-2 interface is
accessible. It is the `svc`/`IUsbManager`/`system_server` legacy-property path,
and it reached the predefined stock `ncm,adb` branch without root, direct
ConfigFS/UDC access, persistent property writes, SELinux modification or ROM
modification. A custom kernel remains unnecessary; the running kernel and stock
init already contain the required primitives and composition.

Because Level 2 is accessible, the Level-3/custom-userspace alternatives are not
selected. They remain out of scope and unauthorized.

The one recommended next phase, not executed, is **DEVICE PHASE 2C-1E1B —
controlled hands-off stock NCM+ADB enumeration replay**. It should pre-arm
continuous sanitized Android property/log observation and Windows PnP/network-
adapter observation before the single bounded request, require no interaction
with the automatically opened USB Settings activity, verify ADB and inbox NCM
enumeration, and then restore `mtp,adb` through the same stock path. It must
remain enumeration-only: no IP, DHCP, TCP, throughput or SweetDisplay traffic.

## Safety closeout

The phone was `mtp,adb` with ADB `device` before and after this diagnosis. Only
read-only service/property queries, previously retained runtime evidence and
locally pulled framework artifacts were inspected. No USB composition request,
property write, ConfigFS/FunctionFS/UDC action, service restart, root operation,
SELinux change, Fastboot action, driver/network change or later-phase test was
performed. Raw evidence and proprietary framework artifacts remain under the
ignored private evidence tree. `git diff --check` and the repository publication
checker pass; the updated public documents contain zero matches for the connected
device serial, personal host path, MAC-address pattern or ADB public key.
