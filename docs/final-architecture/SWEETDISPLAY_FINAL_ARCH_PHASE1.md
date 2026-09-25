# SWEETDISPLAY FINAL-ARCH PHASE 1 — custom boot/minimal userspace architecture

Date: 2026-09-24. Result: **VERIFIED** as an offline architecture and migration
gate. No build, boot-image creation, phone command, USB/network experiment or
device mutation occurred.

> **MIUI + Android receiver APK is the development/validation platform. It is
> not the final SweetDisplay product architecture.**

## Product decision

The final product is a dedicated appliance. Power-on must lead to a
SweetDisplay-owned boot experience and UI, not the MIUI launcher. The user will
not manually launch an APK and normal product operation will not depend on ADB.

The selected first architecture is:

```text
exact stock Xiaomi kernel + embedded DTB + unchanged external DTBO
  -> custom Android-derived minimal ramdisk/userspace
  -> selected stock vendor firmware/HALs and Android native services only
  -> SweetDisplay native core + EGL/OpenGL ES UI
  -> NCM + TCP SweetDisplay transport
  -> protocol touch initially; USB HID later
```

“Android-derived” here means retaining only the compatible native plumbing that
protects proven hardware acceleration: Android init/ueventd and early-mount
mechanisms as required, Binder service managers, SurfaceFlinger, the exact HWC/
allocator/mapper/gralloc/EGL stack, and the media codec services/vendor codec
components needed by Qualcomm VIDC. It does **not** mean retaining MIUI Launcher,
SystemUI, Xiaomi applications, a manually opened receiver APK, or the complete
Java application framework as the product shell.

This is the shortest evidence-backed path to the final product because it keeps
the exact kernel/DTB and the already proven vendor graphics/codec path while
removing the MIUI UX and ordinary-app privilege boundary. It does not require
ideological removal of every Android-origin component.

## Terminology and option decision

| Environment | Meaning here | Meets SweetDisplay-owned UX? | Decision |
|---|---|---:|---|
| A. Stock MIUI validation platform | Current MIUI, ordinary receiver APK and ADB development transport | No | Retain only for validation/reference |
| B. Stock kernel + custom/minimal userspace | Custom init and native processes; hardware paths may be direct Linux | Yes | Useful diagnostic model, but acceleration dependencies remain too unknown alone |
| C. Stock kernel + Android-derived minimal system | Custom userspace plus selected Android native services and exact vendor HALs/blobs | Yes | **Preferred first/final foundation** |
| D. AOSP-based custom system | Broader AOSP system with device/vendor integration | Yes | Possible later, but more framework and porting than the first product needs |
| E. Conventional Linux on existing kernel | glibc/musl desktop-style userspace using DRM/V4L2/evdev directly | Yes | Static display/touch may be possible; Qualcomm GPU/codec/power integration is unproven |
| F. Custom kernel + custom userspace | Rebuilt kernel plus a new system | Yes | Not required now and blocked by non-matching public source/toolchain evidence |

Options B, C, D, E and F can hide MIUI. Only C currently preserves the strongest
measured route to accelerated display and hardware AVC without first solving
unknown proprietary kernel-user ABI behavior.

## Historical results preserved

The roadmap correction does not relabel any prior gate:

| Gate | Historical classification |
|---|---|
| DEVICE PHASE 1 | VERIFIED |
| STOCK IMAGE GATE | VERIFIED |
| DEVICE PHASE 2A | PARTIAL |
| DEVICE PHASE 2C-0 | VERIFIED |
| DEVICE PHASE 2C-T | PARTIAL |
| DEVICE PHASE 2C-T1 | VERIFIED |
| DEVICE PHASE 2C-PERF | VERIFIED |
| DEVICE PHASE 2C-PERF1 | PARTIAL |
| DEVICE PHASE 2C-PERF2 | VERIFIED / TRIGGERED |
| DEVICE PHASE 2C-PERF3 | VERIFIED |
| DEVICE PHASE 2C-1 | VERIFIED |
| DEVICE PHASE 2C-1E1 | DENIED |
| DEVICE PHASE 2C-1E1A | VERIFIED |
| DEVICE PHASE 2C-1E1B | PARTIAL |
| DEVICE PHASE 2C-1E1C | PARTIAL |
| DEVICE PHASE 2C-1E1D | VERIFIED |
| DEVICE PHASE 2C-1E2 | BLOCKED |
| DEVICE PHASE 2C-1E2A | VERIFIED |

In particular, E2 remains BLOCKED for the ordinary APK/ADB-shell environment.
The MIUI address-permission workaround is no longer a product prerequisite:
custom userspace is intended to own ConfigFS and `usb0`. The prepared NCM probe
is retained until FINAL-USB, where the data plane can be tested under that owner.

## Existing-work reuse matrix

Classifications: **A** directly reusable; **B** reusable with adaptation; **C**
validation/hardware evidence only; **D** likely obsolete for the final product;
**E** unknown until custom-boot testing. “Implementation” and “knowledge” are
kept distinct.

### Windows side

| Subsystem/result | Class | Final use |
|---|---:|---|
| IddCx virtual monitor and real Display 3 behavior | A | Remains the Windows display source |
| Shared D3D11 GPU-resource handoff | A | Remains the capture boundary |
| GPU BGRA-to-NV12 conversion | A | Remains the no-full-frame-readback conversion path |
| AMD hardware H.264 encoder | A | Remains the primary encoder |
| Single-owner encoder worker | A | Retains the PERF2 isolation fix |
| Bounded encoder/transport queues and backpressure | A | Same product invariants |
| SWDP framing, CRC, sessions, sequence and limits | A | Transport-independent wire contract |
| Exact ACK, drain, heartbeat and reconnect/IDR recovery | A | Same lifecycle over native USB |
| `TcpStream` generic IPv4 endpoint | A | Reused by NCM/TCP |
| `NcmProtocolProbe` | B | Retained; live endpoint/address discovery and product authentication must be adapted |
| Windows touch injection/release state machine | A | Reused while touch travels in SWDP |
| Dynamic SweetDisplay target discovery/mapping | A | Reused independent of device userspace |

### Device side

| Subsystem/result | Class | Implementation versus evidence |
|---|---:|---|
| Existing receiver APK | B | Parser, bounds, decoder lifecycle and queue rules port; APK/Activity packaging is validation-only |
| Qualcomm hardware AVC decode result | C | Strong hardware/vendor-path evidence; final native service integration remains to be proven |
| Surface rendering result | C | Proves HWC/Surface path and geometry; Activity/SurfaceView is not the final UI |
| AMOLED 1080x2400 60/120 Hz behavior | C | Exact hardware/display-mode evidence |
| MotionEvent touch capture | C | Proves logical ranges and multi-contact behavior; raw evdev implementation replaces it |
| Touch Profile 1 | A | Transport semantics and Windows lifecycle remain usable |
| Receiver reconnect/IDR behavior | B | Logic ports to the native daemon; Android lifecycle wrapper does not |
| ADB-forwarded transport | C | Valuable reference/control transport; not normal final operation |
| NCM enumeration, `usb0` and Windows inbox `UsbNcm` | A | Direct architectural evidence for final NCM choice |
| Stable screen-off NCM+ADB hold | C | Bounded hardware/composition evidence, not production endurance |
| Stock USB init/configuration findings | B | Descriptor/function ordering can guide custom ConfigFS composition |
| Goodix touch identification and coordinate range | B | Guides direct evdev consumer; exact raw MT event contract still needs live proof |
| Kernel DWC3/ConfigFS/FunctionFS/NCM/HID support | A | Directly used by custom userspace |
| DRM/SDE/DSI findings | B | Kernel path is retained; direct ownership/native-service bring-up still needs proof |
| KGSL and vendor EGL/gralloc/HWC inventory | E | Strong candidate stack, but minimal-service dependency closure is untested |
| Qualcomm VIDC nodes/vendor codec declarations | E | Kernel primitive exists; final service/library/firmware closure is untested |
| DMA-BUF/ION/SMMU inventory | E | Present, but cross-component zero-copy is not established |

The APK prototype was not wasted. It proved the exact encoded stream, decoder,
panel geometry, touch semantics, reconnect rules and performance envelope. Its
packaging may disappear while much of its protocol and state-machine knowledge
is preserved.

## Exact-device dependency map

| Area | Known dependency | First architecture treatment | Remaining uncertainty |
|---|---|---|---|
| AMOLED/display | Stock DRM/MSM/SDE/DSI kernel drivers, embedded DTB + DTBO entry 12, panel regulators; accelerated path uses HWC 2.4, allocator/mapper and `gralloc.sm6150` | Reuse exact kernel/DTB/DTBO and stock vendor graphics stack; SurfaceFlinger owns scanout | Exact panel SKU and minimum native-service/property set |
| Touch | Built-in Goodix path exposes Linux input and Android mapped 0..1079 × 0..2399 | Native `touchd` reads passive evdev/MT slots directly; no Android framework required | Exact raw event node/protocol, gestures and wake behavior in custom boot |
| USB | Qualcomm DWC3, ConfigFS, NCM, FunctionFS and HID built in | Custom USB manager composes/binds NCM initially and owns rollback; HID later | Negotiated speed, stable descriptor identity, suspend/reconnect behavior |
| Video decode | MSM VIDC V4L2 nodes, firmware and vendor OMX/codec declarations; MediaCodec hardware decode proved | Retain Android native media service/vendor codec path first | Minimal library/service/property/firmware closure; direct V4L2 usability |
| GPU/UI | KGSL kernel, IOMMU, vendor EGL/GLES/gralloc and HWC | Retain vendor native graphics stack; render SweetDisplay UI with EGL/GLES | Minimal dependency set and boot-time ordering |
| Memory sharing | DMA-BUF, ION, SMMU and vendor gralloc | Let proven Android graphics/media stack negotiate buffers initially | Actual VIDC-to-HWC zero-copy and format interop |
| Power | Panel/touch/USB regulators plus stock charging, health, thermal and power components | Reuse the minimum exact vendor services; no suspend in first short milestone | Safe minimal set, display blanking, charging and suspend/wake behavior |
| Audio | Vendor audio HAL and policy likely required for future local audio features | Out of first milestone; host controls can work without device audio | Exact future service dependency |
| Camera | Camera HAL, ISP, sensors, firmware, media buffers; ConfigFS UVC disabled | Explicitly last phase | Large unknown stack and likely kernel-config requirement for UVC |
| Wi-Fi/Bluetooth | Vendor firmware/HALs and Android services | Not required for first boot/USB display | Future dashboard/media-integration dependency |

## How much Android is needed

| Function | Dependency classification | Decision |
|---|---|---|
| Panel/DRM scanout primitive | KERNEL ONLY plus correct DT/power sequencing | Retain exact stock kernel metadata |
| Accelerated composition | KERNEL + VENDOR USERSPACE + ANDROID NATIVE SERVICES | Retain HWC/gralloc/EGL and SurfaceFlinger |
| Custom UI logic | CUSTOM NATIVE USERSPACE | No Android Java framework required |
| Goodix touch events | KERNEL ONLY | Consume evdev directly |
| USB ConfigFS/NCM/HID | KERNEL ONLY plus custom privileged userspace | Custom USB manager owns it |
| IPv4 on `usb0` | KERNEL ONLY plus userspace network configuration | Custom userspace uses bounded rtnetlink/ioctl configuration; no NetworkStack/netd requirement |
| Hardware AVC decode | KERNEL + VENDOR USERSPACE + ANDROID NATIVE SERVICES | Retain media codec service/vendor OMX initially |
| Software AVC fallback | CUSTOM USERSPACE | Diagnostic fallback only; not accepted as final performance proof |
| MIUI launcher/SystemUI/apps | MIUI FRAMEWORK | Do not start/include in product environment |
| Existing receiver | ORDINARY APK | Keep only on stock validation platform |

The minimum chosen Android footprint is therefore **partial native Android**, not
the full Java framework: compatible init/early mount and service managers,
SurfaceFlinger/graphics HALs, media codec services/vendor codec stack, and the
minimum power/health/thermal support. Exact dependency closure is a build-time
unknown and must be measured rather than guessed.

## Preferred UI and feature model

The preferred UI stack is **native C++ + SurfaceFlinger/ANativeWindow +
EGL/OpenGL ES using the exact vendor HWC/gralloc stack**. It supports the native
2400x1080 portrait AMOLED, GPU animation, textures/album art, overlays and future
hybrid composition while reusing the proven Surface path. A direct DRM/KMS dumb
buffer is a diagnostic fallback only; it is not the selected product renderer.
Qt, SDL and a desktop compositor are unnecessary for the first product and would
add another abstraction/packaging layer before vendor acceleration is closed.

The application model stays small:

```text
sweetdisplay-init (tiny supervisor)
└── sweetdisplay-core
    ├── UI/state machine
    ├── Display Mode
    ├── Dashboard modules
    ├── SWDP transport/session
    ├── decode worker
    ├── touch/gesture worker
    ├── USB/network owner
    └── settings stored only in a future approved persistent area
```

Use threads and modules inside one core process initially, preserving current
single-owner/bounded-queue rules. Split camera or fault-sensitive vendor helpers
into supervised processes only when evidence justifies it. Spotify/media UI will
later consume authorized host media sessions or supported APIs/SDKs; no private
Spotify protocol is designed here.

## Final USB and touch architecture

The preferred initial product transport is **CDC-NCM + TCP**:

- it reuses `TcpStream`, SWDP framing and every tested reconnect/ACK rule;
- the exact kernel supports NCM and the exact Windows host bound inbox `UsbNcm`;
- custom userspace can own ConfigFS and address `usb0` directly, so E2's MIUI
  Binder permission blocker does not apply; and
- it avoids a custom Windows driver and avoids designing FunctionFS OS
  descriptors before the media path works.

Development compositions may temporarily retain ADB for recovery, but normal
final operation must not require it. Final descriptors/USB identity and network
trust/authentication remain design requirements. FunctionFS/WinUSB remains a
later fallback if measured NCM throughput/latency fails; it is not co-equal with
the selected first path.

Touch uses a staged migration: **direct Goodix evdev → existing SWDP Touch
Profile 1 → existing Windows touch injection** first. This preserves the verified
contact/session/release machinery while custom userspace replaces MotionEvent.
USB HID multitouch is a later FINAL-HID optimization after the native media path
is stable; it is not a first-boot or first-USB requirement.

## Hardware decoder migration

Keep the host H.264 pipeline and SWDP FRAME format unchanged. The device core
should initially use Android native media APIs/services (for example, the native
MediaCodec interface) against the exact vendor Qualcomm component and present
decoded output to a SurfaceFlinger surface. This retains the only hardware decode
route actually measured on the device.

Direct V4L2 is not assumed: the kernel exposes MSM VIDC nodes, but the required
Qualcomm controls, firmware, buffer allocation and display interoperability have
not been proven outside vendor services. Linux's generic stateful-decoder API is
an architectural reference, not proof that this downstream driver conforms
without vendor userspace. Software decode is only a diagnostic fallback.

## Stock-kernel and vendor reuse

The exact installed kernel is preferred and a custom kernel is not required now.
It already demonstrates display, touch, NCM/HID/FunctionFS, KGSL, VIDC,
DMA-BUF/ION and SMMU. Rebuilding would instead introduce the known 4.14.180 versus
4.14.190 source mismatch, 20 missing enabled symbols and unresolved Snapdragon
LLVM 10.0.7 provenance.

Selected exact vendor components are expected for graphics, GPU, codec firmware/
services and safe power/thermal/charging behavior. For the owner's device they
may be consumed privately from the exact installed/verified stock build or
mounted read-only. They must not be copied into the public repository or assumed
redistributable. A future distributable image needs a separate licensing and
redistribution analysis.

## Temporary-boot feasibility and boot chain

The exact stock envelope is known: Android boot header v2, 4096-byte pages,
gzip kernel/ramdisk, kernel load `0x8000`, ramdisk load `0x01000000`, embedded
DTB at `0x01f00000`, complete command line, 128 MiB envelope, separate DTBO and
signed AVB topology. The bootloader is unlocked and previously handled ordinary
Fastboot, with a 768 MiB download limit.

A future image can technically preserve the exact stock kernel, embedded DTB,
header fields/addresses/command line and use a custom ramdisk. The preferred
ramdisk model reuses compatible Android first-stage init/dynamic-partition setup,
mounts only required stock logical partitions read-only, and starts a custom
minimal service graph instead of zygote/system_server/MIUI UI.

`fastboot boot` on this exact bootloader remains **TECHNICALLY PLAUSIBLE / LIVE
UNVERIFIED**. Community/device-family evidence and generic Fastboot semantics
support it, but the connected bootloader has never accepted a RAM-boot command.
Unlocked state alone is insufficient proof. No AVB partition may be altered; if
the bootloader rejects an unsigned/downloaded image, the experiment stops rather
than flashing or disabling verification.

The separate stock `dtbo` partition remains untouched and must continue to
select the proven entry 12. The custom environment must not mount userdata or
metadata read-write, start vold/OTA, run fsck repairs, or attempt FBE unlock. Use
tmpfs for logs/state. This avoids depending on userdata keys and keeps the first
milestone independent of FBE. Persistent SweetDisplay settings are a later,
separately designed problem.

## Recovery model and risks

The intended invariant is:

```text
no partition write + RAM-only boot + failure + normal reboot
  = unchanged stock MIUI
```

It is valid only if a separately authorized live gate first confirms the
bootloader's RAM-boot behavior and the artifact performs no storage writes.
Because `boot`, `dtbo`, `vbmeta*`, `super`, `userdata` and `metadata` remain
untouched, a normal reboot should load the exact stock boot partition. This is a
design expectation, not a live-verified recovery claim.

Before any future live command: verify the exact private stock hashes; confirm
Fastboot visibility, battery margin, cable stability and owner presence; prepare
hardware-key return to Fastboot; and define a bounded observation timeout. If
download/boot is rejected, the command hangs unexpectedly, display stays dark,
power/thermal/charging state is unsafe, or stock recovery is not certain, stop.
No flash fallback is permitted. Risks that remain are bootloader command support,
AVB handling of a downloaded image, early SELinux policy loading, dynamic-super
mapping, vendor firmware availability, panel initialization and safe reboot from
the custom service graph.

## Smallest first custom-boot milestone

The first live milestone is deliberately not a streaming milestone:

1. RAM-only temporary boot using the exact stock kernel/DTB envelope.
2. Custom init reaches a bounded SweetDisplay service graph without mounting
   userdata read-write.
3. Selected native graphics/vendor services start.
4. AMOLED shows a SweetDisplay-owned diagnostic screen:

   ```text
   SWEETDISPLAY
   Custom Environment
   Display: OK
   Touch: OK / coordinates
   USB: detected
   Host: disconnected
   ```

5. Goodix touch events update coordinates on screen.
6. DWC3 presence is read and reported; no NCM throughput/video/HID requirement.
7. A controlled reboot returns to unchanged stock MIUI.

This proves “our environment boots and owns the screen.” It excludes H.264,
Spotify, camera, persistent settings, HID and performance testing.

## Revised meaningful roadmap

| Phase | Meaningful acceptance |
|---|---|
| FINAL-ARCH 1 | **This VERIFIED architecture/migration decision** |
| FINAL-BOOT PREP 1 | Offline exact artifact, dependency closure, boot/recovery validation; no device command |
| FINAL-BOOT 1 | Separately authorized RAM-only boot owns AMOLED and returns to untouched MIUI |
| FINAL-INPUT 1 | Direct Goodix evdev multi-touch drives the custom UI reliably |
| FINAL-USB 1 | Custom userspace composes NCM, configures isolated IP and passes SWDP probe/reconnect |
| FINAL-MEDIA 1 | Existing H.264/SWDP path reaches vendor hardware decode and accelerated presentation |
| FINAL-HID 1 | Optional standards-compliant USB multitouch replaces protocol injection if justified |
| FINAL-UI 1 | Product dashboard/mode framework, settings model and polished startup |
| FINAL-FEATURES | Authorized host media controls, telemetry, macros and additional modules |
| FINAL-CAMERA | Camera/HAL/UVC work last, under a separate kernel/USB decision |

Stock MIUI + APK remains installed and documented as a comparison platform for
decoder, display, touch, protocol, performance and hardware-regression testing.
It is not deleted and is not the normal product environment.

## Final architecture answers

| Question | Decision |
|---|---|
| MIUI visible in final product | **NO** |
| Manual APK launch | **NO** |
| ADB required in normal use | **NO** |
| Stock kernel reused | **YES** |
| Android framework retained | **PARTIAL: native graphics/media/init services only; no full Java product shell** |
| MIUI framework retained | **NO** |
| Selected vendor components reused | **YES, exact/private and read-only where possible** |
| Custom userspace | **YES** |
| Custom SweetDisplay UI | **YES** |
| First transport | **NCM/TCP** |
| First touch path | **SweetDisplay protocol from direct evdev; HID later** |
| Custom kernel required now | **NO** |
| Partition write required for first live development boot | **NO by gate design; unsupported RAM boot means STOP** |

## One next phase — not started

The one recommended next phase is **SWEETDISPLAY FINAL-BOOT PREP 1 — OFFLINE
TEMPORARY-BOOT ARTIFACT AND RECOVERY VALIDATION**.

Its exact safety boundary is host-only: privately assemble and inspect one
RAM-boot candidate using the exact stock kernel/DTB/header envelope; define the
minimal ramdisk/service/dependency manifest; verify size, alignment, hashes,
cpio contents, library/HAL closure, read-only mount policy, no-userdata-write
policy, safe reboot path and owner-run live checklist. It may build an offline
artifact only under new explicit authorization. It must not connect to Fastboot,
run `fastboot boot`, flash/write a partition, modify AVB, install to Android,
activate NCM, or begin the live FINAL-BOOT 1 gate.

## Important unknowns

- Whether this exact bootloader accepts `fastboot boot` and how it treats AVB on
  a downloaded custom-ramdisk image.
- Exact early-init/SELinux and dynamic-partition steps needed while suppressing
  zygote/system_server/MIUI services.
- Minimum exact vendor graphics and media service/library/property/firmware set.
- Whether SurfaceFlinger + HWC can start cleanly without the broader framework.
- Direct Goodix evdev MT slot/wake semantics in the custom environment.
- NCM negotiated speed and real TCP/media performance under custom ownership.
- Safe charging/thermal/display-power/suspend service closure.
- Hardware-decoder buffer interoperability and end-to-end presentation latency.
- Long-term persistent settings/update strategy and final USB identity/trust.

These are genuine bring-up questions for the meaningful phases above, not a
reason to return to MIUI networking diagnostics.

## Evidence and safety closeout

The architecture uses the repository's exact-device evidence and official AOSP/
Linux interface documentation. Android's graphics documentation confirms that
SurfaceFlinger, vendor HWC and gralloc are the accelerated composition chain;
Linux documents ConfigFS userspace ownership of NCM functions, evdev/MT input,
DRM/KMS and the generic stateful decoder model. Those generic interfaces do not
replace the exact-device runtime evidence or prove downstream vendor behavior.

No build, compile, ramdisk edit, boot-image creation, ADB/Fastboot command, phone
write, USB mutation, NCM/IP/TCP test, root, SELinux/service/package change,
partition/AVB operation, Windows driver/network/security mutation, HID,
FunctionFS or camera operation occurred. Proprietary artifacts and raw evidence
remain private/ignored. `git diff --check` passed with only normal line-ending
notices. The publication checker passed for 225 publishable working-tree files,
the index and 125 reachable history blobs; public architecture text contains no
device serial,MAC address,unique USB/PnP identity,personal path or ADB-key
material. No commit or push occurred.
