# Stock-kernel USB gadget plan

DEVICE PHASE 2B performed analysis only. It did not write ConfigFS, change USB
functions, create HID, unbind the UDC, disable ADB or test enumeration.

## Current verified state

| Area | Stock runtime evidence | Consequence |
|---|---|---|
| Controller | Qualcomm DWC3 controller reported by boot property/sysfs | Device-mode foundation exists |
| Composition | `sys.usb.config` and state are `mtp,adb`; USB service reports MTP | ADB currently coexists with MTP and must be preserved during development |
| ConfigFS | mounted at `/config`; `/config/usb_gadget` denied to shell SELinux domain | Ordinary APK and shell cannot safely compose a new gadget |
| FunctionFS | ADB plus vendor diagnostic FunctionFS mounts are present | Kernel/userspace FunctionFS machinery is active, but no SweetDisplay endpoint exists |
| NCM | running config enables `USB_F_NCM` and `USB_CONFIGFS_NCM` | Candidate stock-kernel IP transport; not active or enumerated |
| HID | running config enables `USB_F_HID` and `USB_CONFIGFS_F_HID` | Kernel function exists; descriptors, `/dev/hidg*`, policy and host enumeration untested |
| Other network/UVC | ECM, RNDIS, EEM and ConfigFS UVC disabled | Do not design Phase 2C around these absent functions |

Kernel support does not grant an app permission to create a function. Current
ConfigFS denial and the absence of a live HID node mean USB HID is
SUPPORTED-BY-INVENTORY / REQUIRES-EXPERIMENT, not VERIFIED.

## Recommended staged transport

### Phase 2C-0: no gadget change — VERIFIED

Use ADB port forwarding/reverse with the existing TCP ByteStream implementation.
Run the Android receiver as an ordinary APK and preserve protocol 1.0 bytes,
deadlines, partial reads/writes, exact ACK, reconnect and IDR recovery. This proves
the real PC-to-phone display path over the physical USB cable without changing
the gadget. Touch should initially return over the same authenticated development
tunnel under a newly negotiated protocol profile, not as HID.

This prototype is now verified. The real path used ephemeral host-side
`adb forward tcp:48231 tcp:48231`, the existing `mtp,adb` composition and an
ordinary receiver APK. Real Display 3 content reached Qualcomm hardware decode
and the AMOLED; a 600-second conservative run and controlled receiver reconnect
completed with bounded accounting and operator visual confirmation. No gadget
operation occurred. See [DEVICE PHASE 2C-0 results](DEVICE_PHASE2C0_RESULTS.md).

### Phase 2C-1: architecture VERIFIED; E1 DENIED; E1A VERIFIED; E1B PARTIAL

Read-only inspection has established that this exact ROM pre-creates
`functions/ncm.0` and contains complete `ncm` and `ncm,adb` init branches. The
latter links both NCM and `ffs.adb`, then binds the existing DWC3 controller.
Windows 11 has an inbox NCM class driver. Live selection, enumeration, IP setup
and throughput were untested at this gate; see
[the Phase 2C-1 architecture gate](DEVICE_PHASE2C1_ARCHITECTURE.md).

The separately authorized 2C-1E1 experiment issued the high-level stock
`svc usb setFunctions ncm` request once. It returned255 and never changed the
phone from `mtp,adb`; no phone NCM interface or Windows NCM device appeared.
Direct USB-property,ConfigFS and UDC paths were outside scope and were not used.
Rollback through the same framework path passed, with unchanged persistent USB
properties and network-driver inventory. The result is **DENIED** at the
authorized privilege boundary, not a descriptor-enumeration failure; see
[the Phase 2C-1E1 result](DEVICE_PHASE2C1E1_RESULTS.md).

The subsequent read-only 2C-1E1A diagnosis refines the causal interpretation
without changing that historical classification. Exact framework bytecode and
retained runtime events show the request actually reached the no-Gadget-HAL
`UsbHandlerLegacy` path, briefly selected `ncm,adb`, and reconnected ADB. A later
framework request selected `mtp,adb`; it is strongly localized to an interaction
in Xiaomi's automatically opened USB details activity, though the exact clicked
preference is UNKNOWN. Thus an existing legitimate stock Level-2 path is
accessible and Gadget HAL absence is not the blocker. The brief Windows state
was not captured, so NCM enumeration remains UNKNOWN. See
[the Phase 2C-1E1A diagnosis](DEVICE_PHASE2C1E1A_DIAGNOSIS.md).

The controlled 2C-1E1B replay is **PARTIAL**. It proved real stock `ncm,adb`,a
phone `usb0` interface,healthy Microsoft inbox `UsbNcm`,concurrent ADB and MTP
removal. It did not prove a stable hands-off hold: Xiaomi's automatically opened
USB details activity received a recorded DOWN/UP and requested `mtp,adb` within
seconds. The input source remains UNKNOWN. Stock rollback restored healthy
`mtp,adb` with unchanged persistent USB properties,driver inventory and active
network-profile state. See
[the Phase 2C-1E1B result](DEVICE_PHASE2C1E1B_RESULTS.md).

The bounded 2C-1E1C isolation run is also **PARTIAL**. It reproduced all live
NCM+ADB enumeration conditions,but the120-second hold reverted within seconds.
Passive kernel evidence localizes the event to a103ms DOWN/UP from the physical
touchscreen path; Xiaomi USB details requested `mtp,adb` about99ms later. This
excludes the observed mouse,button,accessibility and virtual-input paths,but it
does not distinguish real contact from a touch-controller/electrical ghost
event. The autonomous return and final health checks passed. See
[the Phase 2C-1E1C result](DEVICE_PHASE2C1E1C_RESULTS.md).

The final bounded idle-stability gate,2C-1E1D,is **VERIFIED**. With the owner
placing the phone in its locked/noninteractive screen-off state,the stock
`ncm,adb` composition,phone `usb0`,Windows inbox `UsbNcm` and ADB remained healthy
for121.364s. Passive kernel observation recorded no input event,Xiaomi USB
details did not launch,and no autonomous MTP request occurred. The USB
notification briefly activated the normal low-power AOD/DOZE substate without
an interactive wake. Stock rollback restored healthy `mtp,adb` with no persistent
change. See [the Phase 2C-1E1D result](DEVICE_PHASE2C1E1D_RESULTS.md).

The separately authorized 2C-1E2 data-plane gate is **BLOCKED**. The verified
screen-off NCM foundation reproduced,including `usb0`,healthy Microsoft inbox
`UsbNcm` and concurrent ADB. The stock shell `ndc interface setcfg` request for
the selected temporary `/30` peer returned1 and created no Android IPv4 address.
The gate stopped before Windows addressing,ping or TCP. Stock rollback and all
network/USB cleanup checks passed. See
[the Phase 2C-1E2 result](DEVICE_PHASE2C1E2_RESULTS.md).

The read-only 2C-1E2A ownership diagnosis is **VERIFIED** without relabeling E2.
NetworkStack/Tethering plus `netd` are the stock address owner. Direct `ndc`
requires `NETWORK_STACK` or `MAINLINE_NETWORK_STACK`,which the shell lacks,and
the ordinary receiver cannot administer an interface. This ROM's normal USB
tethering selects RNDIS and reports an empty NCM regex;composition-only
`ncm,adb` therefore has no exposed stock addressing request. See
[the Phase 2C-1E2A diagnosis](DEVICE_PHASE2C1E2A_DIAGNOSIS.md).

The later authoritative [FINAL-ARCH PHASE 1](../final-architecture/SWEETDISPLAY_FINAL_ARCH_PHASE1.md)
changes the product context without changing that historical result. MIUI/APK
is validation-only. The selected final foundation uses the exact stock kernel
with custom minimal userspace that can own ConfigFS and isolated `usb0`
addressing directly. Initial product transport remains NCM/TCP because it reuses
the verified Windows inbox binding and SWDP/TCP implementation. ADB may be kept
only in a development composition;normal product use must not require it.

Do not carry SweetDisplay over NCM unless a later, separately authorized design
establishes a legitimate system-owned way to select the composition. NCM packet
boundaries would still have no protocol-framing meaning. Do not add firewall or
persistent network configuration silently.

### Later privileged composite experiment

A dedicated `ffs.sweetdisplay` transport plus HID absolute-pointer function would
need all of the following before any live attempt:

- a legitimate USB identity and descriptor plan;
- explicit endpoint directions, packet sizes and OS descriptors;
- bounded FunctionFS daemon ownership, cancellation and disconnect handling;
- a HID report descriptor with negotiated logical/physical ranges;
- init/vendor USB-service integration that retains ADB;
- SELinux types and least-privilege allow rules for only the intended endpoints;
- a host rollback/recovery procedure and independent confirmation that ADB returns;
- protocol authentication/trust design beyond today's local prototype boundary.

Those requirements suggest privileged/system integration (level D/E), not a
custom kernel. They are not permission to root, remount, patch vendor partitions
or change SELinux in Phase 2C.

## Proposed HID report

Do not freeze a descriptor until TOUCH negotiation exists. The likely report is
an absolute pointer/digitizer with contact ID, tip/in-range bits, X 0..1079, Y
0..2399 and optional pressure. Rotation must be applied exactly once. Android
MotionEvent time is device-monotonic; HID reports themselves do not solve host
clock correlation.

The protocol should carry touch semantically before translating to HID, so the
device receiver can reject out-of-range coordinates, invalid contact transitions,
duplicate IDs and stale-session events. A disconnect must release every active
contact to prevent a stuck pointer.

## Phase 2C stop conditions

Stop immediately if the proposed operation would remove ADB without an already
tested recovery path, requires an unapproved reboot/root/remount/SELinux change,
touches a partition, exposes an unlicensed USB identity, or cannot restore the
exact prior composition. Phase 2C-0 performed no gadget operation. The Phase
2C-1 architecture gate remains VERIFIED. The 2C-1E1 system-owned NCM selection
gate remains DENIED,while its read-only 2C-1E1A control-path diagnosis is VERIFIED.
2C-1E1B proves Windows inbox NCM enumeration but remains PARTIAL because the
composition did not hold for its full hands-off window. 2C-1E1C remains PARTIAL:
it localizes the recurring input to the physical touchscreen kernel path,but the
real-contact-versus-ghost cause remains unresolved. 2C-1E1D verifies a bounded
121.364s locked/noninteractive idle NCM+ADB hold and healthy stock rollback.
2C-1E2 remains BLOCKED before Layer3 because the authorized stock shell path
could not assign a temporary `usb0` address; no Windows test address,ping or TCP
payload occurred. Read-only 2C-1E2A verifies the stock owner but finds no
accessible current ordinary-app/shell NCM address path. FINAL-ARCH 1 supersedes
the former next-action recommendation:current work proceeds to host-only
FINAL-BOOT PREP 1,then a separately authorized temporary boot. Dedicated gadget
work still requires FINAL-USB authorization.
