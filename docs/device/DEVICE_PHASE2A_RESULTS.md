# Device Phase 2A results

Result: **DEVICE PHASE 2A PARTIAL**

Phase 2A established an immutable stock-kernel reference, captured and reconciled
the complete running configuration, pinned the closest official Xiaomi source,
identified the exact linker family, and created an isolated Linux source tree.
It did **not** build a kernel. The only official Xiaomi `sweet` source is a
device-relevant Android R / Linux 4.14.180 snapshot, while the installed kernel is
an August 2023 Linux 4.14.190 Xiaomi build with source-visible downstream changes.
The exact Qualcomm Snapdragon LLVM 10.0.7 package used by Xiaomi was not obtained
from an authoritative distributor. A build from the available inputs would
therefore be a non-stock approximation and was deliberately not forced.

This phase was host-side only. No boot image was created, no device partition was
read or written, and no Fastboot command was executed. One additional ADB command
read `/proc/config.gz`; it made zero device-side changes.

## Source search and classification

The official MiCode branch listing was queried directly. It contains only two
branch names containing `sweet`: `sweet-r-oss` and `sweet_k6a-r-oss`. The latter
is for Redmi Note 12 Pro and is not a source candidate for this device. No later
official Android S/T `sweet` branch, release, or tag was found.

| Candidate | Pinned identity | Classification | Finding |
|---|---|---|---|
| Xiaomi MiCode `sweet-r-oss` | `758bb7ef50af360e728662a1ed3b3a1b977a2f13` | **DEVICE-RELEVANT NON-MATCH** | Official Redmi Note 10 Pro Android R snapshot; source is Linux 4.14.180 and is the best legitimate device tree, but it is not the installed 4.14.190 source. |
| Qualcomm CodeLinaro base named by MiCode | tag `LA.UM.9.1.r1-06700-SMxxx0.0`, peeled commit `a11fbb33a099d60caed80caab518d886f895feaa` | **DEVICE-RELEVANT NON-MATCH / comparative base only** | Authoritative Qualcomm platform baseline; it does not contain Xiaomi's complete device/vendor delta. |
| Linux stable 4.14.190 | tag peeled commit `e5a54aa2d312e75fe4bc66c7b84400b02266e946` | **authoritative comparative baseline only** | Explains the public upstream stable series, not Qualcomm/Xiaomi downstream content. |
| Embedded identifier `g6d6db67fd446` | no public commit located | **UNKNOWN** | Exact-string web searches, the current MiCode refs, and a public GitHub commit search produced no match. |

There is no **VERIFIED EXACT MATCH** or **VERIFIED CLOSER MATCH**. No
third-party custom-ROM tree was promoted to the baseline or built. Such a tree
could be evidence about individual changes, but cannot establish Xiaomi stock
provenance.

The exact-commit MiCode source archive is preserved in ignored local storage:

- URL: `https://codeload.github.com/MiCode/Xiaomi_Kernel_OpenSource/tar.gz/758bb7ef50af360e728662a1ed3b3a1b977a2f13`
- bytes: `174430207`
- local SHA-256: `5660dd82bab43bfa129a819e3d7895292e13b0865ae6ce6c74d4bd2e140e60aa`
- extracted source file count: `66862`
- source `Makefile` SHA-256: `4a7cbfee3f54b99cc989dc81a76012af5d167fd40f0c88a692c65d68b0d0d11a`

The source archive and extracted tree are not tracked by Git.

## Running configuration reconciliation

The complete running `/proc/config.gz` was captured once with read-only
`adb exec-out`. The exact stock `Image` also contains IKCONFIG. Offline extraction
proved that the two byte streams are identical:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `/proc/config.gz` and stock embedded config gzip | 36,576 | `87b77328fbe327a086e4b1f24be198b4632c73372e7651f8bc4709f4d11e670e` |
| decompressed running/embedded `.config` | 163,492 | `b8821ce37644106a92be29b1d8350ea517fade58a2d924e4e97bc9624e441ca5` |

The MiCode root and vendor `sweet_user_defconfig` files are byte-identical:

- bytes: `19,995`
- SHA-256: `e08c9880807acb8df4e0d940e83cdf520e3eba4a51eaba7d984355ff9dab147c`

Static reconciliation found 5,008 parsed running symbols and 782 explicit
defconfig symbols. Of 765 symbols present in both, 763 have the same value and two
differ. Seventeen explicit defconfig symbols are absent from the running config.
Most importantly, 20 enabled running symbols are not defined anywhere in the
published 4.14.180 Kconfig tree. These include the running Goodix/FTS K6 touch
symbols, newer Xiaomi memory features, LN8000 charging and UFS feature/HPB/TW
support. This proves a material unpublished downstream delta beyond merely changing
`SUBLEVEL` from 180 to 190.

The detailed tables and comparison limits are in
[KERNEL_CONFIG_DELTA.md](KERNEL_CONFIG_DELTA.md).

## Toolchain assessment

The immutable stock kernel reports:

- compiler: `clang version 10.0.7 for Android NDK`;
- linker: `GNU ld (binutils-2.27-bd24d23f) 2.27.0.20170315`.

MiCode's own `AndroidKernel.mk` has a distinct `SDCLANG_PATH` path for Qualcomm
SD-LLVM and an alternate AOSP LLVM path. The stock compiler string is consistent
with Qualcomm Snapdragon LLVM 10.0.7, but no authoritative public package revision,
archive checksum, or still-available direct download was established. The compiler
is therefore identified by exact reported version, but is not reproducibly pinned
as a binary package.

The linker was independently matched to the official AOSP Android 11 GCC/binutils
prebuilt at commit `606f80986096476912e04e5c2913685a8f2c3b65`. Its extracted
`aarch64-linux-android-ld.bfd` reports the exact stock version string:

- bytes: `2,046,256`;
- SHA-256: `2a663de4ce3d702fe3f2a0de48cac366be676f850c2f5732d9cc2e4acb9335e2`.

AOSP `clang-r370808` was also pinned for comparison at AOSP prebuilt commit
`252aba16f513a857bc923172f67b0e55e23de35f`; its official metadata says
`10.0.1 based on r370808`, not 10.0.7. It is therefore a useful Android R reference,
not an exact substitute for the Xiaomi stock compiler.

## 4.14.180 to 4.14.190 gap

The official Linux stable releases 4.14.181 through 4.14.190 contain 927 commits in
their published changelogs. This establishes a real public upstream delta. It does
not prove that applying those commits to the Qualcomm/Xiaomi snapshot reconstructs
the stock source:

- the MiCode release is a downstream Qualcomm/Xiaomi snapshot, not a clean
  kernel.org branch;
- the running config has 20 enabled symbols absent from the published source;
- Goodix changed from `GOODIX_GTX9896_I2C` in the public defconfig to the unpublished
  `GOODIX_GTX9896_K6`, and `TOUCHSCREEN_FTS_K6` is also enabled;
- Xiaomi/Qualcomm Android security backports and vendor-only changes cannot be
  separated from the binary alone;
- the embedded Git identifier is not publicly resolvable.

Changing `SUBLEVEL` or applying only the 927 public stable commits would therefore
misrepresent provenance and stock compatibility.

## Immutable stock kernel binary reference

The already-extracted kernel shared by the exact stock `boot.img` and
`recovery.img` is preserved privately and is not tracked by Git.

| Form | Bytes | SHA-256 |
|---|---:|---|
| gzip payload from Android boot image | 18,156,887 | `764b10ccaf74d2fb0a4569d1254bfc21251a3351afe296d149f639c3d65c7bfc` |
| decompressed Linux `Image` | 44,435,472 | `338179e3d55b5ad348c731266314249a74431b4c1c8497adc23159ff4bca8c6b` |

The decompressed file has the AArch64 Linux `ARMd` magic, text offset `0x80000`,
header image size `51,507,200` and flags `0xA`. Its version is
`4.14.190-perf-g6d6db67fd446`, SMP PREEMPT, built 2023-08-03 14:51:04 UTC. It
records the compiler and linker identities above. The payload is gzip-compressed in
Android boot header v2 with a 4,096-byte page size; see the
[stock image manifest](STOCK_IMAGE_MANIFEST.md) for load address, DTB, DTBO and AVB
requirements.

## Host-side build decision and stock comparison

No kernel build was started. This is a safety and evidence decision, not a build
failure:

1. no exact or closer official Xiaomi 4.14.190 source was found;
2. material running config symbols have no definition in the official 4.14.180 tree;
3. the exact Qualcomm compiler package is not reproducibly pinned;
4. a generated 4.14.180 config cannot represent options that do not exist in that
   source;
5. producing a 4.14.180/AOSP-Clang artifact would answer only whether an old tree
   compiles, not whether it is a stock-compatible baseline.

Consequently there is no built kernel hash, DTB/DTBO output, size comparison or
reproducibility claim. Structural stock requirements are recorded in
[KERNEL_BUILD_BASELINE.md](KERNEL_BUILD_BASELINE.md), but no output is approved for
device boot.

## SweetDisplay feasibility

The stock kernel already exposes the main primitives required for a first
display-and-touch prototype:

- DWC3 gadget plus ConfigFS, FunctionFS, NCM and HID are enabled;
- MSM VIDC V4L2 and Qualcomm AVC codec declarations exist;
- DRM/MSM/SDE/DSI, KGSL, DMA-BUF, ION and ARM SMMU are enabled;
- Goodix is the active input device and HID gadget support is built in.

This means the first prototype should prioritize a privileged phone userspace
daemon and stock Android media/display APIs before assuming a kernel patch is
required. Important unknowns remain: SELinux/device-node access, Android framework
ownership of the display and USB gadget, DMA-BUF interoperability between the
vendor decoder and display path, codec behavior under load, gadget reconfiguration
without losing recovery/ADB access, and absolute-pointer behavior on Windows.

Camera/UVC remains later scope. Stock explicitly disables ConfigFS UVC, so a future
UVC design may require a kernel config/build change. No UVC implementation was
started.

## Remaining blockers

- Exact public source for `g6d6db67fd446`: **UNKNOWN**.
- Authoritative downloadable Qualcomm Snapdragon LLVM 10.0.7 package revision and
  checksum: **UNKNOWN**.
- Xiaomi/Qualcomm/security delta from the released 4.14.180 snapshot: **UNKNOWN**.
- Generated config and successful host build from source that contains all running
  symbols: **NOT AVAILABLE**.
- Exact connected panel SKU and complete proprietary firmware compatibility:
  **UNKNOWN**.
- Runtime VIDC, zero-copy display and HID prototype behavior: **NOT YET TESTED**.

## Repository hygiene

`scripts/windows/Test-PublicRepository.ps1` passed after the documentation edits:
171 publishable working-tree files, the index and 125 reachable history blobs were
checked. A separate public-document scan found no local username/home path, device
serial field, ADB serial or private build-host string. The stock binaries, complete
configs, raw evidence, source archive/tree, toolchain binaries, analysis outputs and
local helper scripts are all ignored. No ignored file was force-added. No commit or
push was performed.

## Safety boundary

Nothing was written to the phone. No `adb root`, `su`, remount, settings change,
Fastboot boot/flash/erase/format, AVB change, custom recovery, Magisk, boot-image
packaging, partition access, or Xiaomi flash script was used. Device Phase 2B did
not begin.
