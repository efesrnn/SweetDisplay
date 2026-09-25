# SWEETDISPLAY FINAL-BOOT 1A — early-boot failure analysis

Result: **VERIFIED for the host-only/offline scope**.

Decision: **GO FOR SEPARATELY AUTHORIZED FINAL-BOOT 1B**.

This decision does not execute or implicitly authorize FINAL-BOOT 1B. No ADB or
Fastboot command was sent to the phone during 1A. FINAL-BOOT 1 remains
**FAILED / stock recovered**.

## 1. V1 failure boundary

FINAL-BOOT 1 proved that the bootloader accepted one exact-hash RAM-only image:
download and `Booting` both returned `OKAY` and the process exited zero. It did
not prove that the custom kernel entered, that PID 1 ran, that SELinux loaded,
or that the diagnostic executed. No SweetDisplay-owned pixel or durable marker
was observed. The proven failure interval therefore remains bootloader handoff
through the first custom pixel; the exact boundary is **UNKNOWN**.

Offline review found an important silent path in v1. The diagnostic retries DRM
initialization for 30 seconds; on failure it only writes stderr, waits another
10 seconds and requests a normal reboot. V1 had no durable or host-visible
marker before that path. A roughly 40-second diagnostic failure is consequently
a credible explanation for the observed early stock return, but not a proven
root cause.

## 2. Reconstructed v1 timeline

| Point | Evidence | Classification |
|---|---|---|
| Read-only identity/preflight gate | `2026-09-24T13:26:03.1710620+03:00` | CAPTURED |
| Exact boot command start | No wall-clock timestamp retained | NOT CAPTURED |
| Fastboot download | `OKAY [0.614s]` | CAPTURED duration |
| Fastboot boot request | `OKAY [0.146s]`; total `0.771s` | CAPTURED duration |
| Fastboot transport disappearance | No independent timestamp | NOT CAPTURED |
| Custom kernel/PID 1/diagnostic | No marker | UNKNOWN |
| Owner saw normal Android | During the bounded initial 60-second observation; exact instant not retained | CAPTURED observation, timestamp UNKNOWN |
| Read-only stock verification | `2026-09-24T13:29:49.3404803+03:00`; Android 13, expected build, boot completed and ADB healthy | CAPTURED |

The prepared 180-second UI timer is inconsistent with stock Android already
being observed inside the initial 60-second gate and is not a credible primary
explanation. Because the command and owner-report wall-clock instants were not
retained, timing alone cannot mathematically exclude every timer/reporting
ordering. The separate approximately 40-second DRM-failure reboot path remains
consistent with the bounded observation.

## 3. Existing postmortem evidence

No v1 pstore/ramoops, boot-reason, last-kmsg, watchdog, panic, init or diagnostic
record was captured before or after FINAL-BOOT 1. `CONFIG_PSTORE_LAST_KMSG` is
disabled in the exact kernel. Existing postmortem evidence is therefore
**NOT CAPTURED**, not negative evidence that no crash occurred.

## 4. Exact v1 candidate revalidation

The preserved private v1 image remains 25,673,728 bytes with SHA-256
`af7ff18d9fa87929b6eff1949180f9ea5788fa0d079897222d218c356852af9e`.
Its gzip ramdisk SHA-256 remains
`d8e3af020ecba694cc9bb1ab99a463de9e6ab5f82a09f7954b5f82ed3c985ef5`.
Rebuilding the non-v2 source path reproduced the exact original static
diagnostic ELF SHA-256
`dc0832544cf5108e50cae7df69cb090b94a14ebe2807224c4e280c3bbc188ddc`.
This confirms that the conditional v2 work did not alter the v1 build.

The v1 image still has an Android boot image v2 header, 4,096-byte page size,
the original complete command line, gzip ramdisk, no AVB footer, exact stock
kernel SHA-256
`764b10ccaf74d2fb0a4569d1254bfc21251a3351afe296d149f639c3d65c7bfc`
and exact embedded-DTB-bundle SHA-256
`8f1b614d87eb9c6835ecf56cfe6f7a338855a5317dad1eab6391222e04804d10`.

## 5. Boot-critical manifest and recovery delta

The v1 and v2 minimal ramdisks each contain 66 paths. The v1 ramdisk was derived
from the 868-path stock recovery ramdisk: 802 stock-only paths were deliberately
removed, no candidate-only pathname was added, and only three common payloads
were replaced. V1 replaced the recovery executable, init graph and ueventd
rules. V2 changes the first two again and restores the exact stock recovery
`ueventd.rc`.

Critical retained paths preserve stock metadata and payloads:

| Path | Mode / owner | Role |
|---|---|---|
| `init` | symlink `0120750`, 0:0 | `/system/bin/init` PID-1 path |
| `system/bin/init` | `0100755`, 0:0 | exact stock recovery init/ueventd binary |
| `system/bin/ueventd` | symlink `0120755`, 0:0 | exact init-backed ueventd entry |
| `system/bin/linker64` | `0100755`, 0:0 | stock init interpreter |
| `sepolicy` | `0100644`, 0:0 | exact compiled stock recovery policy |
| contexts and init libraries | stock modes/owners/content | exact retained dependency closure |

The complete private JSON manifest records path, mode, owner, type, size, hash,
symlink target and purpose for every entry. It contains no block-device node,
persistent mount path or unexpected executable. The only regular executables
are stock `init`, stock `linker64` and the SweetDisplay diagnostic.

## 6. Init graph and first-stage mount analysis

V1 used the normal Android recovery `init`, exact linker/library closure and
compiled enforcing policy, but unnecessarily replayed the
`early-fs -> fs -> post-fs -> post-fs-data -> early-boot -> boot` trigger chain
before starting the diagnostic. Its service was `disabled` and `oneshot`, so
init would not restart it after exit; the explicit `start` at `boot` was the
only launch.

No v1 or v2 fstab, logical-partition mount, `/data`, `/metadata`, `/cache`,
firmware partition or persistent property is required. PID 1, linker,
libraries, policy, contexts and property inputs all reside in the ramdisk.
This makes a missing persistent mount an unlikely prerequisite, while an init
parse/policy/runtime failure before the service remained possible and invisible
in v1.

V2 starts the diagnostic directly from the built-in `late-init` trigger. It
does not synthesize the filesystem trigger chain. It mounts only a RAM-backed
`/tmp` and retains the stock ueventd rules. This narrows the path to PID 1,
ueventd, enforcing policy and one explicit service start.

## 7. SELinux analysis

Both candidates preserve the exact stock compiled recovery policy and context
files. V2 explicitly runs the diagnostic as `u:r:recovery:s0`; stock contexts
label `/dev/dri/card0` as `graphics_device`, `/dev/input` as `input_device` and
`/dev/pmsg0` as `pmsg_device`. The exact stock recovery executable and UI use
display, input, logging and reboot facilities, providing strong static and
behavioral support for reuse of this bounded recovery domain.

An exact binary-policy allow-rule query was not available, so every required
DRM, input, pmsg, backlight and reboot permission is not claimed as individually
proven. A denial before visible output remains possible. V2 does not weaken
policy: it reads `/sys/fs/selinux/enforce`, records the result and refuses to
continue if the value is not enforcing. No permissive fallback exists.

## 8. Diagnostic and DRM/KMS audit

The v1 diagnostic is a static AArch64 ELF with no interpreter or dynamic
section. Its only persistent loop after display setup is a bounded 180-second
UI/touch loop. Before display setup it can fail silently at DRM node open,
master acquisition, resource/connector/CRTC discovery, dumb-buffer creation,
framebuffer creation, mapping, mmap or legacy `SETCRTC`.

The exact stock recovery binary and `librecovery_ui.so` contain direct evidence
for `drmModeGetResources`, `drmModeAtomicCommit`, `drmModeSetCrtc`, dumb-buffer
creation/mapping and `panel0-backlight`. Direct DRM on this recovery stack is
therefore **SUPPORTED BY STATIC EVIDENCE**; it is not speculative in general.
V1's deliberately simple first-connected-connector plus legacy-`SETCRTC` path
may still be incompatible with the Qualcomm SDE panel, atomic-property setup,
splash handoff, connector timing or backlight state. Exact runtime success is
UNKNOWN.

## 9. Silent failure and reboot-source matrix

| Source | V1 assessment |
|---|---|
| 180-second diagnostic timer | Implemented, but inconsistent with the initial 60-second stock observation; exact timestamps incomplete |
| DRM retry timeout plus explicit reboot | Implemented at about 40 seconds and strongly timing-consistent; not proven to have executed |
| init service restart | Excluded by `oneshot`; no restart loop |
| init parse/fatal/policy failure | Possible; v1 had no durable marker before service execution |
| kernel panic or watchdog | Possible; no postmortem captured |
| stock recovery UI reboot logic | Not present as a running service; its executable was replaced |
| other bootloader/kernel handoff failure followed by fallback | Possible; command acceptance alone does not localize it |

## 10. Pstore/ramoops assessment

The exact stock kernel config has `CONFIG_PSTORE=y`, zlib compression,
`CONFIG_PSTORE_CONSOLE=y`, `CONFIG_PSTORE_PMSG=y` and
`CONFIG_PSTORE_RAM=y`; `CONFIG_PSTORE_LAST_KMSG` is disabled. Its built-in
command line extends `ramoops_memreserve=4M`. These are strong prerequisites for
pmsg/console persistence.

The boot DTB bundle contains a ramoops node in one parsed base tree, but the
base tree correlated with Sweet is the `qcom,sdmmagpie` tree and no ramoops node
was found in that selected tree or DTBO entry 12. The downstream
`ramoops_memreserve` platform path may still instantiate the backend. Exact
runtime backend creation, retention across a clean restart and stock-shell
readability are therefore **UNKNOWN** until a separately authorized run captures
a read-only baseline and post-return inventory.

V2 writes each bounded `SDV2` line to both `/dev/pmsg0` and `/dev/kmsg` when
available, then stderr. The exact stock ueventd rule makes `/dev/pmsg0` writable
to root/log. Pmsg is the preferred durable channel; kmsg and stderr provide
same-boot redundancy. No channel writes a persistent filesystem.

## 11. Selected observability and checkpoints

A development USB gadget was rejected. It would introduce ConfigFS, DWC3,
descriptors, UDC, host binding and SELinux dependencies before the boundary
being diagnosed. V2 creates no ADB, NCM, HID, FunctionFS or network function.

Selected channels are:

1. bounded `SDV2 <CLOCK_BOOTTIME> <stage> <detail> errno=<n>` markers to
   pmsg/kmsg/stderr;
2. two backlight pulses after diagnostic entry, seven before a non-enforcing
   hard-stop reboot, and five after DRM initialization timeout;
3. exact DRM substage and errno markers;
4. a distinct `FINAL-BOOT-1B-V2` visible label after modeset;
5. tagged restart reasons for SELinux, DRM and the 180-second timer.

Checkpoint interpretation:

| Checkpoint | Meaning |
|---|---|
| C0 | Host received Fastboot `Booting OKAY`; host evidence only |
| C1 | Kernel execution, supported only by kernel console/pstore or a later userspace checkpoint |
| C2 | Ramdisk/PID-1 path, inferred if later init/service evidence appears |
| C3-C5 | Android init, policy load and `late-init`; a C6 marker plus enforcing read substantially proves this chain |
| C6 | Diagnostic entry; `DIAGNOSTIC_EXEC` and two pulses |
| C7 | DRM node/resources/connector/CRTC/buffer stage or exact failure/errno |
| C8 | Legacy `SETCRTC` succeeded |
| C9 | First diagnostic pixels were written |
| C10 | Goodix/input discovery result |
| C11 | DWC3 read-only discovery result |
| C12 | Explicit tagged reboot request and fallback failures |

## 12. Android init versus a minimal PID 1

V2 retains Android recovery init. It is the smallest defensible choice that
keeps the exact known-bootable linker/library closure, device-node population,
file labeling, enforcing-policy load and service transition. A new static PID 1
would have to reimplement SELinux policy loading, labeling and domain transition
or silently omit enforcement. That would widen the failure surface and violate
the security invariant. The v2 isolation is achieved by a direct `late-init`
service and a 66-entry ramdisk, not by replacing PID 1.

## 13. V2 architecture and validated artifact

V2 changes exactly three v1 ramdisk payloads:

- diagnostic ELF: checkpointing, pulse codes, enforcing hard gate and tagged
  reboot reasons;
- init rc: direct `late-init` launch without filesystem trigger replay;
- ueventd rc: restore the exact stock recovery rules instead of the v1 subset.

All other 63 paths and their metadata/content remain unchanged from v1. The
private v2 artifact is deterministic and re-unpacked successfully:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `sweetdisplay-diag-v2` | 435,848 | `3f6d08f45de5d3878738cfbec012f3b19ae1025672bd1ecbdcf3bdf662bec174` |
| v2 gzip ramdisk | 4,230,458 | `1cdd0b95f9f2bfbb1f0791783a4c1d824c4b9a03442a40d394b1280a4be0959f` |
| `SWEETDISPLAY-TEMP-BOOT-v2.img` | 25,673,728 | `764c8886547dd2f3010f643ecb5789c57af3694aa4dadff91093476facc9cbd1` |

The image preserves the exact stock kernel and embedded DTB hashes above,
header v2, 4,096-byte page, Android 13 / 2023-08 metadata, original command
line and load addresses. It has no AVB footer. Its size is far below the
previously observed 805,306,368-byte Fastboot download bound. It is a private,
ignored, RAM-only diagnostic candidate and is prohibited from flashing.

The ramdisk has no `/data`, `/metadata`, `/cache`, `/mnt`, block-device node,
fstab, partition utility, shell, ADB, network or USB gadget component. No AVB,
vbmeta, boot slot, stock partition or persistent setting is modified by its
design.

## 14. Separately authorized FINAL-BOOT 1B scope — not executed

A future 1B may use only the exact v2 image/hash above and exactly one
`fastboot boot` attempt after a fresh owner/recovery gate. It must first capture
precise host wall-clock timestamps, the read-only stock identity and boot reason,
the pre-existing pstore/ramoops inventory if shell-readable, boot completion and
USB baseline. During the attempt it records command start, download/boot
completion, Fastboot transport disappearance and the owner's pulse/screen
observations. It performs no USB/network/input experiment.

On automatic stock return, it captures only bounded read-only boot-reason and
pstore/ramoops inventory/content available to the ordinary authorized shell,
preserving `SDV2`, panic and watchdog evidence with timestamps. It must not
delete or clear pstore. If the image/hash/identity/recovery gate is ambiguous,
if a write/flash prompt appears, or if stock recovery cannot be assured, it
stops without booting.

No live command in that procedure was executed during 1A.

## 15. Decision and remaining unknowns

FINAL-BOOT 1A is **VERIFIED** because the v1 artifact was revalidated, a concrete
silent early-reboot path was found, boot/init/policy/DRM assumptions were
bounded, a substantially more discriminating non-persistent observability
design was implemented, and the deterministic v2 artifact passed structural,
hash, static-ELF, manifest and no-persistence checks.

Decision: **GO FOR SEPARATELY AUTHORIZED FINAL-BOOT 1B**.

Important unknowns remain: whether the v1 kernel or userspace executed; exact v1
reboot cause; exact runtime recovery-domain permissions; actual selected-panel
DRM/atomic requirements; pstore backend creation and ordinary-shell readability;
backlight pulse visibility before modeset; and the exact post-boot USB baseline.
These are 1B observations, not grounds to relabel FINAL-BOOT 1.

## 16. Files and hygiene

Phase 1A changed these publishable files:

- `device/final/diagnostic/sweetdisplay_diag.cpp`
- `device/final/diagnostic/init.sweetdisplay.v2.rc`
- `device/final/diagnostic/README.md`
- `device/final/tools/build_ramdisk.py`
- `scripts/windows/Build-FinalDiagnosticV2.ps1`
- `scripts/windows/Build-FinalTempBootV2.ps1`
- `docs/final-architecture/SWEETDISPLAY_FINAL_BOOT1A_ANALYSIS.md`
- `STATUS.md`
- `docs/ROADMAP.md`
- `docs/TEST_LOG.md`
- `docs/ARCHITECTURE.md`

Generated ELF, manifest, ramdisk, unpacked validation files and v2 image remain
under ignored output/private evidence paths. `git diff --check` passed with only
normal line-ending notices. The repository publication checker passed for 238
publishable working-tree files, the index and 125 reachable history blobs. A
targeted review found no unique device identifier, USB instance identity, MAC,
ADB key, personal path, private log, firmware/image payload or raw evidence in
the 1A public changes. No commit or push occurred.
