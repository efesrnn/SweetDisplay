# SWEETDISPLAY FINAL-USB

Current result: **PARTIAL; Candidate 4 READY FOR LIVE VALIDATION.** Candidate 1
proved stock NCM-function creation but stopped at an unnecessary sysfs check.
Candidate 2 then proved custom-environment NCM enumeration and Microsoft inbox
`UsbNcm`; its endpoint stopped before address ioctls because enforcing SELinux
denied UDP-socket creation. Both attempts rebooted normally to healthy stock.
Candidate 3 reached the exact address ioctl but Android SELinux's separate ioctl
extended-permission filter denied command `0x8916`. Candidate 4 adds only the two
mutating interface ioctl command numbers required by the implementation.
FINAL-USB is not VERIFIED.

FINAL-BOOT BRINGUP remains VERIFIED. Historical stock-MIUI USB results remain
unchanged: ConfigFS/NCM/DWC3 and Microsoft inbox `UsbNcm` enumeration were
already proven, while stock shell address assignment was blocked by Android's
network ownership boundary. FINAL-USB tests whether the dedicated temporary
SweetDisplay userspace can own that configuration directly.

## Minimum custom-environment closure

Every candidate retains the exact pinned stock kernel, embedded DTB and Android
first-stage init/ueventd closure used by V11. Candidates 1 and 2 retain the
exact compiled stock recovery policy. Candidates 3 and 4 use reviewed
stock-derived policies that remain enforcing. Candidate 3 adds the proven
native TCP-server boundary; Candidate 4 adds only exact ioctl extended
permissions for address and netmask assignment. Its 75-entry ramdisk adds only
an empty `/config` mountpoint and one static native `sweetdisplay-usbd`
executable to the stable V11 display/touch diagnostic closure.

After stock coldboot completion, init performs the minimum volatile gadget
setup:

1. mount ConfigFS at `/config`;
2. create one gadget using the exact stock NCM-only identity `05C6:A4A1`;
3. create one `ncm.0` function and one configuration;
4. use the stock Microsoft OS-descriptor settings and link only `ncm.0`;
5. bind the existing `a600000.dwc3` UDC;
6. start the diagnostic and native USB endpoint in the existing recovery
   SELinux domain.

The gadget contains no ADB, MTP, RNDIS, FunctionFS, HID, camera or audio
function and publishes no device serial string. The kernel NCM function owns
its volatile link-layer addresses; they are neither persisted nor published.
Init grants the endpoint only `CAP_NET_ADMIN`, required for its three mutating
`usb0` interface ioctls. Candidates 3 and 4 deliberately use an IPv4 TCP socket for
those ioctls because the stock policy already grants the recovery domain TCP
socket creation/ioctl. It adds only recovery-domain TCP read/write/getattr,
bind/listen/accept/setopt/shutdown plus ordinary IPv4 node/port bind. It adds no
UDP permission, domain transition or permissive rule. Candidate 3 proved that
the ordinary `ioctl` class permission is additionally constrained by Android's
xperm filter. Candidate 4 adds only `0x8916` (`SIOCSIFADDR`) and `0x891c`
(`SIOCSIFNETMASK`); stock already grants recovery `0x8913`/`0x8914` for reading
and setting interface flags. Any further denial is a bounded blocker;
permissive mode or a broad policy change is not a fallback.

## Point-to-point data plane

The native endpoint waits for `usb0`, assigns only `10.77.77.2/30`, marks that
interface up and listens on `10.77.77.2:48231`. It creates no default route,
gateway, DNS, DHCP, NAT, bridge or Internet-sharing state.

The elevated Windows controller discovers exactly one healthy adapter bound to
Microsoft `UsbNcm`, then adds `10.77.77.1/30` to that adapter in `ActiveStore`.
It rejects a pre-existing route conflict, default route, DNS server, driver
inventory change or unhealthy PnP state. It never installs or updates a driver,
changes firewall/profile settings, or uses ADB forwarding. The address is
removed explicitly after the protocol probe; adapter removal and reboot are a
second cleanup boundary.

TCP itself proves bidirectional IPv4 transfer. No ICMP test is required. The
existing Windows `NcmProtocolProbe` opens two sequential native TCP sessions,
thereby exercising one bounded disconnect/reconnect. Each session performs the
existing 48-byte SWDP framing, HELLO, CAPABILITIES, eight integrity-checked
HEARTBEAT round trips and a DRAIN request/acknowledgement. Video, frame CRC,
media decode and touch transport are deliberately outside FINAL-USB.

## On-device observability and rollback

The stable V11 double-buffer/page-flip UI is retained. A second panel reports
USB, static-IP, TCP, session, heartbeat, drain and SWDP state while preserving
DISPLAY, physical-touch coordinates, DWC3 and SELINUX ENFORCING indicators.
The native endpoint writes only a volatile status file in `/tmp`; it does not
write a persistent partition.

The diagnostic still exits after 180 seconds. Init owns the normal reboot, and
the independent 240-second watchdog remains. Reboot discards the whole custom
ramdisk, gadget and device address. The Windows controller waits for NCM
removal, healthy stock ADB/boot completion, `mtp,adb` config/state and enforcing
SELinux, then verifies no temporary address/route, NCM device or driver change
remains. A failed live check does not trigger a second boot automatically.

## Candidate 1 offline artifact

- Image: 25,862,144 bytes; SHA-256
  `d88e37141185d4c1d9942b59087b47d3f4f36987b3207f37289952a5c594caac`.
- Ramdisk SHA-256:
  `8df3202f81a84680192971a8ce06692e29387a9dee92e5ebb7876e3283ad4d7f`.
- Diagnostic: 440,424-byte static ARM64 ELF; SHA-256
  `a4139cee026d537ff7392ec73155af8bb1e00007d506fef3a243a019c107d601`.
- USB endpoint: 431,816-byte static ARM64 ELF; SHA-256
  `c069c7a22f098a925b10912f90bd223c1d105031f0635756927d32c2fe689e5e`.
- Exact stock kernel SHA-256:
  `764b10ccaf74d2fb0a4569d1254bfc21251a3351afe296d149f639c3d65c7bfc`.
- Exact embedded DTB SHA-256:
  `8f1b614d87eb9c6835ecf56cfe6f7a338855a5317dad1eab6391222e04804d10`.
- Two clean builds are byte-identical. Boot v2/4096-page structure, re-unpack,
  exact kernel/DTB, no-AVB-footer, executable closure, forbidden persistent
  path and sensitive-marker checks pass.
- Eighteen page-flip event cases pass; both device binaries are static ELFs.
  The existing Windows SWDP NCM probe rebuilds cleanly with the pinned EWDK.
- PowerShell parser checks pass for build and live-control scripts.
- Repository whitespace and publication hygiene pass for 276 publishable
  working-tree files, the index and 125 reachable history blobs. Generated
  images/binaries, the iteration ledger and future raw evidence remain ignored;
  no unique device/USB identity, MAC or personal path is published.

## Candidate 1 live result

The fresh owner gate and exact preflight passed one `sweet`, unlocked target and
the exact candidate hash. Exactly one RAM-only command completed with Sending
OKAY in 0.627 seconds, Booting OKAY in 0.147 seconds and 0.786 seconds total.
The owner observed the SweetDisplay UI reporting `USB: ERROR`.

Post-return `SYSTEM_LAST_KMSG` supplies the exact boundary:

- NCM function creation ran at 4.584 seconds and the kernel generated volatile
  self/host Ethernet addresses;
- the endpoint then repeatedly received enforcing denial
  `recovery -> sysfs_net:dir search` while checking `/sys/class/net/usb0`;
- the source waits for that access check before calling any address ioctl, so
  address, link-up, listener and SWDP code were not reached;
- no other new endpoint-domain denial was observed;
- normal timer-path reboot occurred at 184.582 seconds.

The independent Windows controller stopped after boot on a malformed
`Where-Object` filter before it could retain NCM enumeration evidence or add the
project address. Thus Windows NCM enumeration in this attempt is UNKNOWN, not a
failure claim. `10.77.77.1/30` and its route were absent afterward, no NCM device
remained, and the Windows driver-package inventory matched baseline. Unrelated
host default-route/DNS/profile snapshots differed after return because the
ordinary host adapters had disconnected; the controller never reached a
network mutation, so causation is not assigned, but exact global equality is
not claimed for Candidate 1.

Stock return is healthy: ADB `device`, boot completion 1, `mtp,adb` config/state,
SELinux Enforcing and V14.0.2.0.TKFTRXM. No flash, partition write, persistent
device state or project network address occurred.

## Candidate 2 offline result

Candidate 2 removes the denied and unnecessary sysfs existence check. It polls
the same bounded 30-second window by attempting the `usb0` configuration ioctl
directly; absence remains a normal retry condition. It labels each address,
netmask, flag-read and link-up stage so any next enforcing denial is exact. It
does not change policy, capability, ConfigFS topology, addressing or protocol.

The Windows observer replaces every malformed property-filter shorthand with
an explicit script block. Those filters execute successfully against the
current safe no-NCM/no-project-route state. No acceptance criterion is weakened.

- Image: 25,862,144 bytes; SHA-256
  `57b6e8e3f3577297563874b7ef9540fcedb67633bd39b230a9044b59bf0e5308`.
- Ramdisk SHA-256:
  `d770434b9df4787d59d311922e053817aeb27300a3abd4d986aa2a9a09d31fec`.
- Diagnostic SHA-256:
  `2b9eb1e8f91333b7ec06bbb1177738d149a4a4c74e626f560b77ae97491f1cbd`.
- USB endpoint SHA-256:
  `3d38e5b1370f6ae8451f763745aa81d52dfd4bd67422d8032ff731ababba2dd1`.
- Exact stock kernel and DTB hashes remain unchanged.
- Two Candidate 2 builds are byte-identical; all Candidate 1 diagnostic/daemon
  source-regression hashes still match their live artifact exactly.
- Static ELF, event tests, boot structure/re-unpack, 75-entry closure and
  no-persistence/sensitive-marker checks pass.

## Candidate 2 live result

One fresh owner-authorized RAM-only attempt used the exact Candidate 2 hash.
Fastboot reported Sending OKAY in 0.623 seconds, Booting OKAY in 0.147 seconds
and 0.779 seconds total. The owner observed `USB: ACTIVE` followed by
`USB: ERROR`.

This attempt materially advances the gate:

- the custom environment created NCM and Windows enumerated one healthy adapter
  using Microsoft inbox `UsbNcm`, with problem code 0;
- the Windows controller added its ActiveStore project address, but its fixed
  500 ms snapshot required exactly one immediately visible connected route and
  stopped before TCP; the address was removed in `finally`;
- the kernel record independently shows `ncm.0` creation at 4.724 seconds and
  then the exact enforcing denial `recovery -> self:udp_socket create` at the
  labelled `FU2_SOCKET` stage, before any address ioctl;
- the custom environment therefore did not obtain 10.77.77.2/30 and no native
  TCP/SWDP session occurred;
- normal timer-path reboot occurred at 184.672 seconds.

The controller sampled stock USB during its transient `adb`-only convergence
and therefore reported a cleanup failure that was not the final state. A later
bounded read-only check established ADB `device`, boot completion 1, `mtp,adb`
config/state, SELinux Enforcing and V14.0.2.0.TKFTRXM. Windows had no NCM device,
project address or project route; its default route, DNS, profiles and driver
inventory matched baseline. No persistent change occurred.

## Candidate 3 offline result

Candidate 3 does not add the denied UDP permission. It uses the already allowed
TCP socket class for the interface ioctls and adds only the server operations
subsequently required by the same socket. The policy is compiled at version 30
with expansion disabled. Binary disassembly proves the exact expected allow
delta, zero non-allow runtime-rule delta and no recovery UDP permission. Forty
unused generated attribute memberships are compiler-pruned; none is referenced
by any runtime rule.

The Windows controller now waits up to ten seconds for ActiveStore address/route
convergence, accepts duplicate store views of the same project route, rejects
any unexpected NCM route/default/DNS state, and waits for complete
`boot_completed=1` + `mtp,adb` + Enforcing convergence before classifying stock
return. These changes remove premature sampling without weakening cleanup.

- Image: 25,858,048 bytes; SHA-256
  `62dade6576d8c2338fb080b94d3a3ee6c43f1973e56950522bee6719b0310a41`.
- Ramdisk SHA-256:
  `54ae6081b399f65f85951b900b057e9fd0d1c46bf3128331d911ee1b10dca4c9`.
- Enforcing policy: 908,849 bytes; SHA-256
  `320e3cfc471a214e33964529206812371df1cdeaeeb975dc9af2319525303cb8`.
- Diagnostic SHA-256:
  `b8a3a57f96ba6749b97971b862804fe37a64b44dae5d0f7e8e01abb70936da89`.
- USB endpoint SHA-256:
  `a3f1da36bea2be4566e568680f23ad3edce5d988a2ecac298ca824c37185d939`.
- Exact stock kernel and embedded DTB hashes remain unchanged.
- Repeated builds reproduce the exact image, ramdisk, policy and ELF hashes.
  Candidate 1 binary hashes also remain exact. Static ELF, 18 event cases,
  boot-v2 re-unpack, 75-entry closure, no-AVB-footer, forbidden-path and
  sensitive-marker checks pass.
- `git diff --check` passes. Publication hygiene passes for 278 publishable
  working-tree files, the index and 125 reachable history blobs. Generated
  images/policies/binaries and raw evidence remain ignored/private; no unique
  device/USB identity, MAC or personal path is published.

## Candidate 3 live result

One fresh owner-authorized RAM-only attempt used the exact Candidate 3 hash.
Fastboot reported Sending OKAY in 0.606 seconds, Booting OKAY in 0.146 seconds
and 0.765 seconds total. Windows again enumerated a healthy Microsoft inbox
`UsbNcm` adapter with problem code 0. The owner photo independently shows
DISPLAY OK, USB CONTROLLER DETECTED, SELINUX ENFORCING, BUILD FINAL-USB-3,
USB NCM READY, IP ERROR, TCP WAITING and all protocol counters at zero.

The latest `SYSTEM_LAST_KMSG` segment supplies the exact device boundary:

- at 4.556 seconds, `FU3_ADDR` reached `SIOCSIFADDR` on the TCP socket;
- enforcing SELinux denied `tcp_socket ioctl` command `0x8916` even though the
  ordinary class-level ioctl permission was present;
- 63 identical bounded retries occurred through 31.696 seconds; netmask,
  flag, listener and SWDP stages were never reached;
- normal timer-path reboot occurred at 184.423 seconds;
- the 66,163-character latest segment has SHA-256
  `9ba5f178b046726c45c585ac3e981589e83858209699b93f4ae0cdda0f08cb45`.

Because the phone never raised `usb0`, Windows saw a healthy PnP device but no
connected data-plane route. Its temporary ActiveStore address failed the
address/route convergence gate and was removed before TCP. Final controller
outcome is FAILED with successful cleanup: stock ADB, boot completion,
`mtp,adb`, Enforcing and V14.0.2.0.TKFTRXM returned; NCM, project IP/route and
problem devices were absent; default route, DNS, profiles and driver inventory
matched baseline. No persistent change occurred.

## Candidate 4 offline result

Candidate 4 retains Candidate 3's exact class-level TCP-server permissions and
adds one xperm statement containing only `0x8916` and `0x891c`. The numbers are
verified against the pinned Android NDK Linux socket headers. Compiled policy
version 30 has the exact expected allow and allowxperm deltas, zero other
runtime-rule delta, no UDP permission and no permissive domain.

- Image: 25,858,048 bytes; SHA-256
  `cf0158c10fc083c2fc4e71e75a0fa828fb3b17e0b4d5b7277ce9dcd389fb5938`.
- Ramdisk SHA-256:
  `b6b27cee1c0e724af4ac87513d927ea4c1f6787699e08345ea1d192c0d153757`.
- Enforcing policy: 908,849 bytes; SHA-256
  `83a09282dcfc1aac1424d7413fe02e07429031d49e72597d0d70eaefefd60079`.
- Diagnostic SHA-256:
  `4bb0307fdea553866a058552dda8529a660bcad09d342d7c6a753816d9992e0c`.
- USB endpoint SHA-256:
  `0a48a32dd5f43eb8f20d9fab15c2e4694e49653cfecd1307e18dea981ed75c07`.
- Two builds are byte-identical for image, ramdisk, policy and both ELFs.
  Exact stock kernel/DTB, static ELF, 18 event cases, boot-v2 re-unpack,
  75-entry closure, no-AVB-footer, forbidden-path and sensitive-marker checks
  pass. Candidate 3 policy and ELF hashes remain exact under regression rebuild.
- `git diff --check` and publication hygiene pass for 279 publishable files,
  the index and 125 reachable history blobs. The owner photo, raw kernel data,
  images, policies and binaries remain ignored/private; no unique device/USB
  identity, MAC or personal path is published.

Live-only Candidate 4 facts remain NOT YET TESTED: successful address/netmask/
link-up ioctls, native TCP/SWDP exchange, two sessions and full cleanup.

## Next live gate

The next live attempt may occur only after a fresh owner confirmation that the phone
is physically accessible, battery/thermal state and cable are safe, hardware
recovery controls are available, no unrelated operation is active, and the
phone is in bootloader Fastboot. The controller must match product `sweet`,
unlocked state, exactly one device and Candidate 4 SHA-256
`cf0158c10fc083c2fc4e71e75a0fa828fb3b17e0b4d5b7277ce9dcd389fb5938`
before issuing one `fastboot boot`. Flash, erase, format, partition/AVB writes
and automatic retry remain prohibited.

FINAL-USB becomes VERIFIED only if the full live acceptance set passes,
including stable AMOLED/touch/enforcing, NCM/UsbNcm, deterministic IP, two TCP
sessions with SWDP exchange, controlled reboot, healthy unchanged stock return
and complete Windows cleanup. Stop after VERIFIED; do not begin FINAL-MEDIA.
