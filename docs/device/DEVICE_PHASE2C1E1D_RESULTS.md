# DEVICE PHASE 2C-1E1D — screen-off NCM stability gate

## Result

**DEVICE PHASE 2C-1E1D: VERIFIED**

The owner placed the phone stationary on a dry nonconductive surface and used
the physical power button to enter the locked, noninteractive screen-off power
regime before the live request. The single authorized stock request reached
`ncm,adb`; Android created `usb0`; Windows bound healthy Microsoft inbox
`UsbNcm`; ADB returned; and all four hold-start conditions remained established
for **121.364 seconds**. There was no autonomous `mtp,adb` request and no
touchscreen DOWN/UP event. Controlled stock rollback succeeded.

This is bounded idle-composition stability only. It does not establish IP, TCP,
payload, throughput, latency, video or production NCM behavior. Historical
classifications remain unchanged:

- DEVICE PHASE 2C-1: **VERIFIED**;
- DEVICE PHASE 2C-1E1: **DENIED**;
- DEVICE PHASE 2C-1E1A: **VERIFIED**;
- DEVICE PHASE 2C-1E1B: **PARTIAL**; and
- DEVICE PHASE 2C-1E1C: **PARTIAL**.

## Baseline and display-off gate

Immediately before mutation, two independent read-only checks established:

- `sys.usb.config=mtp,adb` and `sys.usb.state=mtp,adb`;
- host ADB state `device` and `adbd=running`;
- no phone `usb0` interface and no active Windows NCM device;
- healthy problem-code-0 Windows ADB and MTP;
- Android `mWakefulness=Dozing`;
- built-in display state `OFF`;
- window-policy screen state `SCREEN_STATE_OFF`; and
- window-policy interactive state `INTERACTIVE_STATE_SLEEP`.

The second display check occurred after all passive observers were armed and
immediately before the NCM request. It still reported `Dozing` plus physical
display `OFF`. No POWER/keyevent or other input was injected.

## Observers and request

Private, timestamped observers covered Android USB composition, ADB reconnect,
phone interfaces, framework/init and UI logs, passive kernel input events,
window/power state and Windows PnP/network-adapter state. Raw logs and device
identity remain below the ignored private evidence tree.

Exactly one NCM request was issued:

```text
adb shell svc usb setFunctions ncm
```

The host request exited 255 when its ADB transport was interrupted by expected
USB re-enumeration. Independent state, rather than this exit code, established
success. First phone confirmation of `ncm,adb`, running ADB and `usb0` occurred
at `2026-09-23T15:24:05.463+03:00`. Healthy Windows NCM plus ADB was established
at `2026-09-23T15:24:06.744+03:00`, starting the hold timer.

## Screen, input and Xiaomi UI behavior

The phone never entered interactive `Awake` state during the bounded hold. Its
focus remained the locked Always-On Display surface and no resumed activity was
reported. The USB state broadcast reached Xiaomi's `UsbModeChooserReceiver`, but
the bounded logs contain no launch, focus or resume of
`Settings$UsbDetailsActivity`.

The USB notification caused the built-in display to enter low-power AOD/`DOZE`
for approximately nine seconds before returning to literal `OFF`. Three readable
samples reported `DOZE`; the following 55 readable samples and the pre-rollback
sample reported `OFF`. This is recorded explicitly: the device stayed locked,
noninteractive and in Android's screen-off/dozing power regime, but the panel was
not literally `OFF` at every instant because AOD briefly illuminated. It never
became normal interactive `ON`.

Both passive kernel-input observer segments recorded **zero event lines**:
zero touchscreen DOWN, zero touchscreen UP and zero hardware-key event. Thus the
physical-touchscreen-path event seen in E1C did not recur. This result does not
prove whether the earlier E1C event was real contact or ghost/electrical input.

## Stable NCM hold

The acceptance timer ran from `2026-09-23T15:24:06.744+03:00` through
`2026-09-23T15:26:08.108+03:00`, totaling **121.364 seconds**. Throughout the
hold:

- Android samples remained `ncm,adb` with `usb0`;
- ADB remained available in the NCM composition;
- Windows supplied 57 consecutive healthy `UsbNcm` plus ADB samples;
- Windows recorded no NCM problem-code sample;
- MTP remained absent while NCM was active;
- no `mtp,adb` reversion request occurred; and
- no phone interaction, cable movement or second NCM request occurred.

The Windows binding remained `UsbNcm Host Device`, service `UsbNcm`, Microsoft
provider, inbox `usbncm.inf`, version `10.0.26100.9444`, healthy/problem code 0.
No driver was installed or updated. Framework speed reporting returned `-1`, so
negotiated USB speed remains **UNKNOWN**.

## Rollback and final state

After the successful hold, the controller issued the required stock rollback:

```text
adb shell svc usb setFunctions mtp
```

The host process exited 255 as its ADB transport was interrupted by the expected
second re-enumeration. Independent rollback observation converged successfully:

- `sys.usb.config=mtp,adb` and `sys.usb.state=mtp,adb`;
- `adbd=running` and host ADB state `device`;
- no phone `usb0` interface;
- no active Windows NCM device;
- healthy Windows ADB and MTP, problem code 0;
- persistent system/vendor USB properties unchanged;
- Windows driver-package inventory unchanged; and
- active Windows network-profile state unchanged.

Emergency reboot was not used. No persistent phone, Windows, network, security,
display, lock, accessibility or input change was introduced.

## Scope and next recommendation

No IP address, DHCP operation, route, metric, network profile, firewall change,
ping, TCP socket, iperf, SweetDisplay traffic or throughput test occurred. No
root, `su`, `adb root`, `setprop`, direct ConfigFS/UDC, SELinux, service/package,
input injection, accessibility, Fastboot, partition/AVB, custom boot/userspace,
kernel, Windows driver, HID, FunctionFS or camera operation occurred. No commit
or push occurred.

The next recommended phase is **DEVICE PHASE 2C-1E2 — EPHEMERAL NCM IP + TCP
DATA-PLANE PROOF**. E1D does not authorize it, and E2 was not begun.

Remaining unknowns are negotiated USB speed, the cause of E1C's physical-
touchscreen-path event, long-duration/production stability and all NCM data-plane
behavior.

## Publication closeout

`git diff --check` passed with only the repository's normal line-ending notices.
The publication checker passed for 220 publishable working-tree files, the index
and 125 reachable history blobs. Targeted scans of the updated public documents
found zero connected-device serial, personal-path, MAC-address, USB-instance or
ADB-key matches. The private controller and complete raw run remain ignored.
