# DEVICE PHASE 2C-1E2A — stock `usb0` address-ownership diagnosis

Date: 2026-09-24. Result: **VERIFIED** as a read-only ownership diagnosis.

DEVICE PHASE 2C-1E2 remains **BLOCKED**. This phase did not activate NCM,
assign an address, enable tethering, run TCP, or change USB/network state. It
identifies the stock owner and the accessibility boundary; it does not
invalidate the E2 software prepared and verified offline.

## Direct answer

Android already has components authorized to configure an interface: the
NetworkStack/Tethering implementation decides downstream addressing and asks
`netd` to apply interface configuration and routes. `system_server` also exposes
an internal network-management path to `netd`. On this exact ROM, however,
selecting the dormant `ncm,adb` gadget composition does not start a Tethering
downstream. The runtime Tethering configuration selects RNDIS for USB tethering,
has no NCM-specific interface regex, and did not claim or address `usb0` in E2.

The ordinary SweetDisplay APK cannot assign an address to an arbitrary Linux
interface with a public Android API. The ADB shell cannot use direct `ndc`
because the direct `netd` operation requires `NETWORK_STACK` or
`MAINLINE_NETWORK_STACK`; neither is granted to the shell. Although the shell
has several other privileged diagnostic permissions on this development build,
none satisfies that direct Binder check or exposes a stock NCM-addressing CLI.

## Why the E2 `ndc` request failed

The Android 13 path for the exact command form is:

```text
ADB shell (UID 2000, u:r:shell:s0)
  -> /system/bin/ndc interface setcfg
  -> INetd.interfaceSetCfg(InterfaceConfigurationParcel)
  -> NetdNativeService Binder permission check
  -> InterfaceController/kernel interface configuration, only if authorized
```

Pinned Android 13 `ndc` source shows that `interface setcfg` parses the address,
prefix and `up` flag, constructs `InterfaceConfigurationParcel`, calls
`INetd.interfaceSetCfg`, and reports a failed Binder status. Android 13
`NetdNativeService::interfaceSetCfg` is guarded by `NETWORK_STACK` or
`MAINLINE_NETWORK_STACK` before it calls `InterfaceController::setCfg`.

Exact runtime evidence establishes:

- the shell was UID 2000 in `u:r:shell:s0`;
- all effective, permitted, inheritable and ambient Linux capabilities were
  zero (its bounded set was `0xc0`, which does not include `CAP_NET_ADMIN`);
- `NETWORK_STACK` is signature-only and `MAINLINE_NETWORK_STACK` is a
  signature permission owned by the NetworkStack package;
- neither required permission is granted to `com.android.shell`; and
- the NetworkStack shared UID 1073 holds `MAINLINE_NETWORK_STACK`, while the
  native `netd` process owns the capabilities needed to apply network state.

Therefore a direct shell `ndc interface setcfg` call cannot pass this ROM's
`netd` Binder permission boundary. E2 proved that `usb0` existed and that no
IPv4 address appeared, so the failure was before successful kernel address
configuration. Exit code 1 by itself does **not** identify a layer. E2 preserved
no Binder exception text and returned no diagnostic output; the exact returned
status/message remains **UNKNOWN**. The combined source and runtime permission
evidence localizes the effective rejection to the direct `netd` authorization
boundary, not to a demonstrated kernel/NCM failure. The command was not repeated.

Primary source references:

- [Android 13 `ndc` dispatcher](https://android.googlesource.com/platform/system/netd/+/refs/tags/android-13.0.0_r76/server/NdcDispatcher.cpp)
- [Android 13 `NetdNativeService`](https://android.googlesource.com/platform/system/netd/+/refs/tags/android-13.0.0_r76/server/NetdNativeService.cpp)

## Stock ownership map

| Component | Stock responsibility | Can SweetDisplay legitimately invoke it? |
|---|---|---|
| `netd` | Applies interface configuration, addresses and routes after Binder permission enforcement | Not directly from the ordinary APK or current `ndc` shell caller |
| `NetworkManagementService` in `system_server` | Internal framework broker to `netd`; enforces network-stack/internal permissions | Internal/system API; no ordinary-app API and no bounded stock CLI for this NCM request |
| ConnectivityService | Tracks provisioned Android `Network` objects and routes app sockets; coordinates connectivity services | Public APIs can request/use a network, not assign IPv4 to arbitrary `usb0` |
| NetworkStack/Tethering | Owns downstream address selection, interface-up configuration, connected route, DHCP and optional upstream/NAT behavior | Privileged/module path; this ROM exposes no ordinary-app NCM request |
| Ethernet service | Manages interfaces admitted to Android Ethernet management | System API/signature permission; `usb0` was not shown to be an Ethernet-managed interface |
| UsbDeviceManager/vendor init | Selects gadget functions and realizes stock `ncm,adb` | It creates the USB function/interface but does not by that act assign Layer-3 state |
| Xiaomi USB UI/services | Select USB modes through the framework path | No registered vendor API capable of NCM IPv4 assignment was found; such a path remains UNKNOWN |

`netd` is the mechanism that finally applies an address, but NetworkStack/
Tethering or `system_server` is normally the policy owner that asks it to do so.
Having technical privilege is distinct from exposing an authorized product API.

Runtime process boundaries were consistent with that ownership:

| Process | UID / SELinux domain | Effective capability mask |
|---|---|---:|
| `netd` | 0 / `u:r:netd:s0` | `0x74ef` |
| `system_server` | 1000 / `u:r:system_server:s0` | `0x1806897c20` |
| NetworkStack/Tethering process | 1073 / `u:r:network_stack:s0` | `0x3c00` |
| ADB shell | 2000 / `u:r:shell:s0` | `0x0` |

SELinux was Enforcing. Linux capabilities explain which trusted process can
perform low-level networking, while the decisive `ndc` boundary is the Android
Binder permission check. No SELinux denial was provoked or policy changed.

## NCM composition versus stock tethering

The exact runtime `dumpsys tethering` configuration reported:

```text
tetherableUsbRegexs: [usb\d, rndis\d]
tetherableNcmRegexs: []
mUsbTetheringFunction: RNDIS
Upstream wanted: false
Current upstream interface(s): null
```

The Tethering process did observe historical USB broadcasts with `ncm:true`, but
E2's composition-only request produced an unaddressed `usb0`. This establishes
that `svc usb setFunctions ncm` selects the gadget composition; it does not
request an IpServer/Tethering downstream.

The generic USB regex could textually match `usb0`, and AOSP Tethering/IpServer
has code capable of assigning a downstream address, prefix and connected route.
That does not create an accessible NCM path on this build: normal USB tethering
is configured to request RNDIS, the NCM-specific regex is empty, and no active
tethering state claimed `usb0`. Starting conventional USB tethering would be a
different experiment, would change the USB composition to RNDIS and could add
DHCP/NAT/upstream behavior; it is not a way to address the already-selected
dormant NCM composition under E2's constraints.

For comparison, stock RNDIS tethering is owned by NetworkStack/Tethering. Its
IpServer selects a downstream prefix, asks the privileged interface controller/
`netd` to bring the interface up, adds a connected route, and manages DHCP and,
when tethered, forwarding/NAT. The kernel and stock services could technically
perform the same primitives for NCM, but this ROM does not configure or expose
that NCM-specific request path.

Relevant AOSP sources:

- [Android 13 Tethering configuration](https://android.googlesource.com/platform/packages/modules/Connectivity/+/refs/heads/android13-mainline-tethering-release/Tethering/src/com/android/networkstack/tethering/TetheringConfiguration.java)
- [Android 13 downstream IpServer](https://android.googlesource.com/platform/packages/modules/Connectivity/+/refs/heads/android13-mainline-tethering-release/Tethering/src/com/android/networkstack/tethering/IpServer.java)

## Ordinary application and service/API candidates

The installed SweetDisplay receiver is an ordinary UID and requests only
`android.permission.INTERNET`. It cannot obtain signature permissions by user
consent.

| API/path | Actual relevance to `usb0` address assignment | Accessibility |
|---|---|---|
| `ConnectivityManager`, `NetworkRequest`, `NetworkCallback` | Discovers/requests a network after a system owner provisions it | Public, but insufficient |
| Bind process/socket to `Network` | Selects an existing provisioned network | Public, but insufficient |
| `TetheringManager` | Can ask the stock stack to create a downstream; static IPv4 requires privileged tethering authority | System/module API; not an ordinary-app NCM interface, and this ROM selects RNDIS |
| Local-only tethering | Stock stack can create a local-only downstream | No public ordinary-app USB/NCM request matching this ROM; still privileged ownership |
| `EthernetManager.updateConfiguration` | Can configure Ethernet-managed interfaces | System API with `NETWORK_STACK`/`MAINLINE_NETWORK_STACK`/`MANAGE_ETHERNET_NETWORKS`; applicability to this `usb0` is unproven |
| IpClient / NetworkStack | Performs IP provisioning under system ownership | Internal/module API, not an application API |
| VPN APIs | Create/manage a TUN-based VPN | Not applicable to assigning an address to gadget `usb0` |
| Direct `ndc` | Calls `netd` directly | Rejected for this shell caller; direct privileged mutation is outside the authorized product path |

Registered services included `connectivity`, `ethernet`, `netd`,
`network_management`, `network_stack`, `tethering` and `usb`. The Tethering and
NetworkStack service endpoints are protected by mainline/network-stack
permissions. `cmd connectivity help` exposed only airplane-mode operations on
this build; Ethernet reported no shell command implementation, and no stock
NCM-addressing command was present. No state-changing Binder method was called.

The shell package does hold `CONNECTIVITY_INTERNAL`, `TETHER_PRIVILEGED`,
`MANAGE_USB` and `WRITE_SETTINGS` on this development build. That does not make
the failed direct `ndc` call pass its distinct network-stack permission gate,
does not give the ordinary APK those permissions, and does not expose a stock
NCM-specific request. A handcrafted `service call`, reflection payload or custom
shell program would be direct privileged shell mutation/custom userspace, not the
existing legitimate product interface requested by this gate.

## Candidate decision table

| Candidate | Classification | Required boundary / reason |
|---|---|---|
| Direct shell `ndc` | **NOT ACCESSIBLE** | `NETWORK_STACK` or `MAINLINE_NETWORK_STACK`; shell has neither |
| `system_server` network-management path | **SUPPORTED** internally, **NOT ACCESSIBLE** to the ordinary APK | System/internal Binder authority; no public `usb0` assignment API |
| Android Tethering stack | **SUPPORTED** for stock downstream ownership, **NOT ACCESSIBLE** for dormant NCM here | Privileged/module API; exact ROM selects RNDIS and has no NCM regex |
| Ordinary SweetDisplay APK | **NOT ACCESSIBLE** | `INTERNET` permits sockets, not interface/address administration |
| Privileged/system APK or service | **POSSIBLE**, not implemented or proven | Platform/signature permission plus a narrowly designed system integration |
| Xiaomi/vendor service | **UNKNOWN** | No registered NCM IPv4 service/API was found |
| Temporary custom userspace | **POSSIBLE** in a separately authorized Level-3 environment | Needs `CAP_NET_ADMIN`/matching SELinux and an independent recovery plan |
| Root/direct network configuration | **SUPPORTED** technically, prohibited | Root or equivalent privileged mutation; unnecessary as an architectural requirement |

## Product work preserved

E2's generic IPv4 `TcpStream`, `NcmProtocolProbe`, Android NCM diagnostic bind,
HELLO/CAPABILITIES exchange, eight HEARTBEAT echoes per session, ordering,
DRAIN/ACK and reconnect design remain **OFFLINE VERIFIED / LIVE NCM UNVERIFIED**.
The Android APK build/signature check, EWDK `/W4 /WX` probe build and 58,235
protocol regressions remain valid. The address-ownership blocker does not remove
or weaken those results.

Root is not intrinsically required: the stock NetworkStack, `system_server` and
`netd` already possess the required authority. A custom kernel is not required:
the stock kernel has NCM and previously enumerated it. What is absent is an
authorized, existing Level-2 API that the ordinary SweetDisplay APK/current
bounded shell workflow can use to request NCM `usb0` addressing.

## Next architecture gate

The alternatives are: introduce a narrowly scoped platform-privileged/system
integration; use a controlled temporary Level-3 userspace with the existing
stock kernel; change native transport strategy; or retain the verified ADB
transport for the current prototype. A platform integration would require
signature/system trust not presently available on unchanged stock MIUI, while a
different native gadget still needs privileged ownership. Retaining ADB is the
lowest-risk product baseline but does not answer native-NCM viability.

The one recommended next architecture gate is **DEVICE PHASE 2C-1L3G —
EPHEMERAL LEVEL-3 AUTHORIZATION AND RECOVERY GATE**. It should decide, offline
and without booting anything, whether to authorize a RAM-only temporary
userspace using the existing stock kernel, exactly which capabilities/SELinux
domain it needs, and how stock boot is recovered. It must not build, boot or
flash during that gate. E2B and E3 must remain stopped unless a later explicit
authorization establishes this boundary.

## Safety and hygiene

The phone began and ended `mtp,adb`; ADB was healthy. SweetDisplay did not need
to be running. This phase performed only read-only ADB queries. It did not
activate NCM/RNDIS/tethering, assign or clear an address, change a route, use
DHCP, ping, open TCP, create an ADB tunnel, stream data, restart a service, or
modify USB, firewall, security, package, system/vendor, boot or partition state.

Raw command output remains private/ignored. Public documentation contains no
device serial, MAC address, USB instance identifier, ADB key, personal path or
other unique machine/device identity. `git diff --check` passed with only normal
line-ending notices. The publication checker passed for 224 publishable files,
the index and 125 reachable history blobs; targeted changed-document identity
scans returned zero matches. No commit or push occurred.

**B. STOCK OWNER EXISTS BUT NO ACCESSIBLE PATH EXISTS**
