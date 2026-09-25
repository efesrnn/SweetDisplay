# DEVICE PHASE 2C-1E1C — USB UI input-source isolation and stable NCM hold

## Result

**DEVICE PHASE 2C-1E1C: PARTIAL**

The single authorized stock request again reached `ncm,adb`. Android reported
matching config/state and created `usb0`; Windows enumerated a healthy Microsoft
inbox `UsbNcm` device with problem code 0; ADB returned concurrently and MTP was
absent. All hold-start conditions were therefore independently established.

The composition did not remain stable for the required 120-second hands-off
hold. Xiaomi Settings automatically opened its USB details activity. A passive
kernel input observer then recorded a 103 ms DOWN/UP sequence from the physical
touchscreen event device. Android's input-interaction record aligned with that
sequence, and `UsbDeviceManager` requested `mtp,adb` approximately 99 ms later.
The phone was stationary and the owner had confirmed hands-off operation.

This localizes the event to classification **A: physical touchscreen input path**,
not a hardware button, mouse, accessibility service or observed virtual-input
device. The evidence cannot distinguish a real conductive contact from a
touch-controller/electrical ghost event, so that narrower cause remains
**UNKNOWN**. The useful localization plus healthy autonomous return/final state
meets the defined PARTIAL result. It does not meet the stability requirement for
VERIFIED.

Historical classifications are unchanged:

- DEVICE PHASE 2C-1 architecture gate: **VERIFIED**;
- DEVICE PHASE 2C-1E1: **DENIED**;
- DEVICE PHASE 2C-1E1A: **VERIFIED**; and
- DEVICE PHASE 2C-1E1B: **PARTIAL**.

## Baseline and observers

Immediately before the request, the phone reported
`sys.usb.config=mtp,adb`, `sys.usb.state=mtp,adb`, `adbd=running` and host ADB
state `device`. MIUI Launcher was foreground; the USB details screen and
SweetDisplay were not foreground. No phone USB-network interface, ADB
forward/reverse mapping or active SweetDisplay transport was present. Windows
had healthy problem-code-0 ADB and MTP and no active NCM PnP device.

Before mutation, the controller armed private timestamped Android logcat and
passive `getevent` observers, plus independent phone config/state/interface,
activity/window/power and Windows PnP/network-adapter sampling. Raw records stay
under the ignored private evidence tree.

The sanitized input inventory contained eight registered input devices: one
touchscreen, no mouse, two key/button-class devices and one virtual/uinput-class
device. Accessibility was disabled. Passive observation did not grab, seize,
reconfigure or inject input.

## Request and correlated timeline

Exactly one NCM request was issued:

```text
adb shell svc usb setFunctions ncm
```

The exact host process exit code is **UNKNOWN** because a local PowerShell
post-processing expression failed after the live evidence and final-state files
had already been captured. The request stderr retained MIUI's previously known
missing-theme-resource diagnostic. Exit status is not used as the transition
criterion; independent state proves the request succeeded.

First independent phone confirmation of `ncm,adb`, running ADB and `usb0` was at
`2026-09-23T13:19:44.101+03:00`. Healthy Windows NCM plus ADB was first sampled
at `2026-09-23T13:19:45.305+03:00`, starting the stability hold. The device-side
event ordering was:

1. `UsbDeviceManager` requested `ncm,adb`.
2. Approximately 0.80 seconds later Xiaomi Settings automatically started and
   focused `Settings$UsbDetailsActivity`.
3. Approximately 4.21 seconds after the NCM request, the physical touchscreen
   kernel device emitted DOWN and then UP over about 103 ms.
4. Android recorded input interaction with the focused USB details activity.
5. Approximately 99 ms after that interaction record, and while the kernel
   touch sequence was still completing, `UsbDeviceManager` requested `mtp,adb`.
6. NCM disappeared and healthy MTP+ADB returned.

The controller detected the reversion about 3.05 seconds after all independent
hold-start conditions had been established. Device logs place the NCM-to-MTP
request interval at about 4.31 seconds. The required 120-second hold therefore
failed and was stopped without a second NCM request.

## Windows and phone result

The temporary live Windows binding was:

| Field | Observed value |
|---|---|
| Device description | `UsbNcm Host Device` |
| Service | `UsbNcm` |
| Provider | Microsoft |
| INF | `usbncm.inf` |
| Driver version | `10.0.26100.9444` |
| Device health | Healthy |
| Problem code | 0 |

No third-party/manual driver was requested, installed or updated. The complete
Windows driver-package inventory was byte-identical before and after the run.
Android `usb0` appeared while both config and state were `ncm,adb`; ADB returned
in that composition. Negotiated USB speed is **UNKNOWN** because the volatile
speed result was lost with the post-processing fault and no later privileged or
composition-changing operation was performed to recreate it.

The controller's summary-format defect was corrected in the ignored private
controller and its PowerShell syntax was revalidated. It occurred only after
the live observation, autonomous return, final phone/Windows capture and driver
inventory capture. It did not cause or authorize another NCM request.

## Rollback and final state

The touchscreen-driven Xiaomi Settings path had already returned the phone to
`mtp,adb`. In accordance with the experiment instructions, no redundant
`svc usb setFunctions mtp` command was issued. The normal controller and its
error-safety trap both used the `confirm-existing-mtp` path; the trap recorded
that no recovery command was needed and convergence was complete. Emergency
reboot was not used.

Final verification established:

- `sys.usb.config=mtp,adb` and `sys.usb.state=mtp,adb`;
- `adbd=running` and host ADB state `device`;
- no phone `usb0` interface;
- no active Windows NCM PnP device;
- healthy Windows ADB and MTP, both problem code 0;
- unchanged Windows driver-package inventory; and
- unchanged active Windows network-profile state.

No persistent USB-property operation was issued. The final persistent USB
values remain the previously established development-device baseline. No
lasting USB, network, driver, security or accessibility change was introduced.

## Scope, safety and remaining unknowns

No IP address, DHCP operation, route, metric, network-profile or firewall
change, ping, TCP socket, iperf, SweetDisplay payload or throughput test was
performed. No root, `su`, `adb root`, remount, `setprop`, direct ConfigFS/UDC,
manual gadget/function, FunctionFS, HID, SELinux, package/service, input
injection, accessibility, Fastboot, partition/AVB, kernel/userspace, Windows
driver or security operation occurred. No ADB-server restart, commit or push
occurred.

Remaining unknowns are:

- whether the physical-touchscreen-path event was real conductive contact or a
  controller/electrical ghost event;
- why it recurs shortly after this USB-composition transition;
- long-term/production NCM stability in the absence of that event;
- exact NCM request-process exit code for this run; and
- negotiated USB speed.

This result does not verify IP transport, TCP over NCM, SweetDisplay over NCM,
NCM performance, HID, FunctionFS or camera behavior. No later phase was begun.

## Publication closeout

`git diff --check` passed with only the repository's normal line-ending notices.
The publication checker passed for 219 publishable working-tree files, the index
and 125 reachable history blobs. Targeted scans of the updated public documents
found zero connected-device serial, personal-path, MAC-address, USB-instance or
ADB-key matches. The private controller and complete raw run remain ignored.
