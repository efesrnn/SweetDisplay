# Kernel build baseline

Status: **HOST BASELINE PREPARED; BUILD GATED / NOT EXECUTED**

This document defines the reproducible inputs and stop conditions for a future
stock-aligned `sweet` kernel build. It does not approve any artifact for device use.

## Pinned source input

| Field | Value |
|---|---|
| Repository | `MiCode/Xiaomi_Kernel_OpenSource` |
| Branch | `sweet-r-oss` |
| Commit | `758bb7ef50af360e728662a1ed3b3a1b977a2f13` |
| Commit date | 2021-03-05 |
| Source version | 4.14.180 |
| Classification | **DEVICE-RELEVANT NON-MATCH** |
| Exact-commit archive SHA-256 | `5660dd82bab43bfa129a819e3d7895292e13b0865ae6ce6c74d4bd2e140e60aa` |
| Extracted `Makefile` SHA-256 | `4a7cbfee3f54b99cc989dc81a76012af5d167fd40f0c88a692c65d68b0d0d11a` |
| `sweet_user_defconfig` SHA-256 | `e08c9880807acb8df4e0d940e83cdf520e3eba4a51eaba7d984355ff9dab147c` |

The source is extracted on the Linux filesystem under an ignored, Phase 2A-only
`$HOME/.local/share/sweetdisplay/device-phase2a/source-<commit>` directory. The
Windows partial-clone cache and exact-commit archive also remain ignored. No source
or proprietary firmware payload is publishable project content.

## Isolated host environment

The available execution environment is WSL2, Ubuntu 26.04, x86-64. At inspection
time it had `bc`, Perl, Python and OpenSSL, but did not have `make`, GCC, Clang,
Bison, Flex, `pahole` or `cpio`. No system packages were installed and no existing
PATH or development environment was changed.

The source was extracted because its legitimate Linux paths include `aux.c` and
`aux.h`, which cannot be represented safely in a normal Windows checkout. A future
build environment must remain isolated and must pin both the OS/package snapshot and
every toolchain archive checksum.

## Toolchain pin state

| Component | Required/observed identity | Reproducible pin | Status |
|---|---|---|---|
| C compiler | Qualcomm/Snapdragon LLVM `10.0.7 for Android NDK` | No authoritative package commit/archive SHA established | **BLOCKER** |
| GNU linker | `GNU ld (binutils-2.27-bd24d23f) 2.27.0.20170315` | AOSP prebuilt commit `606f80986096476912e04e5c2913685a8f2c3b65`; binary SHA-256 `2a663de4ce3d702fe3f2a0de48cac366be676f850c2f5732d9cc2e4acb9335e2` | **EXACT VERSION MATCH** |
| AOSP comparison compiler | `clang-r370808` | AOSP prebuilt commit `252aba16f513a857bc923172f67b0e55e23de35f`; official metadata SHA-256 `b65bbcd19952a8f3d9f3d10c12e2fdb0f023c079855c1f540d18f3f97ca58a49` | `10.0.1`, explicitly **NOT** stock 10.0.7 |

MiCode's `AndroidKernel.mk` supports `KERNEL_SD_LLVM_SUPPORT=true` and obtains the
compiler from `SDCLANG_PATH`; it passes the compiler through `REAL_CC` and uses a
GNU cross-prefix for the rest of the toolchain. This is direct source evidence that
the Snapdragon LLVM path is intentional.

No third-party mirror of Snapdragon LLVM was accepted. Matching only the displayed
version number is insufficient without authoritative origin and a package checksum.

## Configuration input

The primary configuration input is the exact running/embedded config:

- decompressed bytes: `163492`;
- SHA-256: `b8821ce37644106a92be29b1d8350ea517fade58a2d924e4e97bc9624e441ca5`;
- enabled source-missing symbols: 20.

The published 4.14.180 Kconfig tree cannot consume the running config faithfully.
In particular, the running Goodix/FTS K6 and newer UFS symbols are undefined there.
Blindly running `olddefconfig` would silently discard or replace evidence and is not
a valid reconciliation.

## Intended build contract, not executed

After the blockers are resolved, a build should use a clean isolated output
directory and record all environment values. The shape of the command is:

```sh
make -C "$SOURCE" O="$OUT" ARCH=arm64 vendor/sweet_user_defconfig
make -C "$SOURCE" O="$OUT" ARCH=arm64 \
  CROSS_COMPILE="$GNU_PREFIX" \
  REAL_CC="$SDLLVM_CLANG" \
  CLANG_TRIPLE=aarch64-linux-gnu- \
  Image modules dtbs
```

This is a command contract, not a claim that the current source accepts the running
config or produces stock-compatible DTBs. Before running it, the environment must
also pin host `make`, Bison, Flex, OpenSSL, Python, `pahole`, `cpio`, the complete
GNU cross-toolchain and any source-required host libraries.

A reproducibility record must include:

- source commit/archive SHA-256;
- toolchain archive SHA-256 and every `--version` output;
- input and generated config SHA-256;
- complete environment and command line;
- build log and warnings;
- uncompressed and compressed kernel hashes/sizes/formats;
- DTB/DTBO outputs and selection assumptions;
- a second clean-build comparison, including any timestamp/build-host differences.

## Immutable stock comparison target

| Property | Stock reference |
|---|---|
| Version | `4.14.190-perf-g6d6db67fd446` |
| Architecture/format | AArch64 Linux `Image`, `ARMd` magic |
| Compression in boot image | gzip |
| Compressed bytes / SHA-256 | `18,156,887` / `764b10ccaf74d2fb0a4569d1254bfc21251a3351afe296d149f639c3d65c7bfc` |
| Decompressed bytes / SHA-256 | `44,435,472` / `338179e3d55b5ad348c731266314249a74431b4c1c8497adc23159ff4bca8c6b` |
| AArch64 text offset | `0x80000` |
| Header image size / flags | `51,507,200` / `0xA` |
| Config SHA-256 | `b8821ce37644106a92be29b1d8350ea517fade58a2d924e4e97bc9624e441ca5` |
| Android boot envelope | v2 header, 4,096-byte pages, kernel load `0x8000`, embedded DTB |

Binary identity is not expected without exact source and toolchain. A future
comparison must at least cover architecture, version provenance, compression,
config, load format, critical built-ins/modules, firmware references, DTB/DTBO
relationship and the 128 MiB boot-partition/download envelope.

## Current gate decision

The host build is **NOT EXECUTED** because it would be a 4.14.180 approximation
using a non-exact compiler. No kernel, module, DTB, DTBO or boot image was produced.
No artifact is approved for `fastboot boot` or any form of flashing.

