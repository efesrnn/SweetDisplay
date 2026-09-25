# Stock image manifest and recovery gate

Result: **STOCK IMAGE GATE VERIFIED**

This is the immutable host-side reference for the connected Xiaomi Redmi Note
10 Pro `M2101K6G`: bootloader product `sweet`, Android product `sweet_tr`, Turkey,
Android 13 / MIUI 14 `V14.0.2.0.TKFTRXM`. The machine-readable companion is
[STOCK_IMAGE_MANIFEST.json](STOCK_IMAGE_MANIFEST.json). Any future artifact that
does not match the recorded SHA-256 is a different artifact and must not inherit
this gate.

No package script was executed. No archive or proprietary image is tracked by
Git; they remain under the ignored private evidence tree.

## Official-host and archive identity

Xiaomi's own [support page](https://www.mi.com/service/support) directly uses
`bigota.d.miui.com` for Xiaomi flashing-tool distribution. Both exact firmware
objects returned `HTTP 200` directly from that Xiaomi-controlled host without an
HTTP redirect. `bn.d.miui.com` exposed the same objects with identical byte
lengths and ETags and was used as the faster Xiaomi CDN endpoint during transfer.
No arbitrary mirror supplied local bytes.

| Archive | Bytes | SHA-256 | MD5 cross-check |
|---|---:|---|---|
| `sweet_tr_global_images_V14.0.2.0.TKFTRXM_20230803.0000.00_13.0_tr_7ab6f47e67.tgz` | 5,934,114,812 | `8367f915a4e8ca7018bc0fa3fe1a70048b38154dc0c56ec3e5870f4993dae40d` | `7ab6f47e67f2679e5974bbad86238ecc` |
| `miui_SWEETTRGlobal_V14.0.2.0.TKFTRXM_4dd7b72cd7_13.0.zip` | 3,988,152,798 | `60d2828c30a421b2e83b7cc4975d9ece33ae642c2b225f6eba304d8c9c5b409d` | `4dd7b72cd76aee890762b98c031a04f7` |

Canonical URLs:

- [Fastboot archive](https://bigota.d.miui.com/V14.0.2.0.TKFTRXM/sweet_tr_global_images_V14.0.2.0.TKFTRXM_20230803.0000.00_13.0_tr_7ab6f47e67.tgz)
- [Recovery archive](https://bigota.d.miui.com/V14.0.2.0.TKFTRXM/miui_SWEETTRGlobal_V14.0.2.0.TKFTRXM_4dd7b72cd7_13.0.zip)

The MD5 values match the package-name/index values but are only transfer
cross-checks. The locally calculated SHA-256 values above identify the preserved
archives.

## Relevant extracted images

| Image / partition | Bytes | SHA-256 | Presence |
|---|---:|---|---|
| `boot.img` / `boot` | 134,217,728 | `0a9dfe4d30b600115a5484fde873cdd293c3cc99683aebcdbcc3b9176b17a8f3` | Fastboot and recovery archives; identical |
| `recovery.img` / `recovery` | 134,217,728 | `4dabd9add3145acaa81faf42f760dbdac7daadd6175d9e047d75b2e5131ede61` | Fastboot archive only |
| `dtbo.img` / `dtbo` | 33,554,432 | `1ebc0d5fa3134f7237b407a4331a006a56c953f24c392561343e0c06227b649c` | Fastboot and recovery archives; identical |
| `vbmeta.img` / `vbmeta` | 8,192 | `9a691b87896da1714d65c47fc5d45a597fcfb306d62a2497634e165f84fc5c2f` | Fastboot and recovery archives; identical |
| `vbmeta_system.img` / `vbmeta_system` | 4,096 | `9c8c54b4e0456934a14a62ef68f35e93b5b15d3906063a8aa4b72ea70ca02b70` | Fastboot and recovery archives; identical |
| `super.img` / `super` | 8,105,485,628 sparse | `314488dca73b5e42daf32e9e64ce2f3b6610097ae3a1168d1ce6bfbbf64d7db3` | Fastboot archive only |

`vendor_boot.img` and `init_boot.img` are absent from both archives.
`vbmeta_vendor.img` is also absent: the fastboot partition manifest defines the
physical `vbmeta_vendor` partition with an empty filename. The main `vbmeta`
instead carries the `vendor` hashtree descriptor. These are observed package
facts, not inferred partition deletions.

## Boot and recovery image format

Both images have `ANDROID!` magic, header version 2, 4,096-byte pages, Android
13.0.0 / patch level 2023-08, no second stage, the same gzip kernel and the same
embedded DTB payload.

| Field | `boot.img` | `recovery.img` |
|---|---:|---:|
| Kernel size / compression | 18,156,887 / gzip | 18,156,887 / gzip |
| Kernel load address | `0x00008000` | `0x00008000` |
| Ramdisk size / compression | 812,101 / gzip | 18,412,603 / gzip |
| Ramdisk load address | `0x01000000` | `0x01000000` |
| Second-stage size | 0 | 0 |
| Tags address | `0x00000100` | `0x00000100` |
| Recovery-DTBO | none | 7,078,877 bytes at `0x22e2000` file offset |
| Embedded DTB | 3,276,863 bytes at load address `0x01f00000` | same |
| AVB original image size | 22,257,664 | 46,940,160 |
| Partition-padded file size | 134,217,728 | 134,217,728 |

Shared command line:

```text
androidboot.hardware=qcom androidboot.memcg=1 lpm_levels.sleep_disabled=1 video=vfb:640x400,bpp=32,memsize=3072000 msm_rtb.filter=0x237 service_locator.enable=1 swiotlb=1 androidboot.usbcontroller=a600000.dwc3 loop.max_part=7 buildvariant=user
```

The boot ramdisk is a small first-stage Android ramdisk. The recovery ramdisk is
a full Xiaomi recovery environment with recovery init files, binaries, resources
and dynamic-partition support. Listing/extraction was offline; nothing was run.

## DTBO findings

The separate DTBO partition contains a valid version-0 DT table:

- table payload: 7,078,877 bytes; partition image: 33,554,432 bytes;
- 4,096-byte page size and 40 entries;
- every table-entry `id`, `rev` and four `custom` fields is zero;
- the recovery image embeds the exact same DT table payload, confirmed by
  SHA-256 `6a7e89d943c74839bc0c10e6b439a75aaeae74ddfc3b238fe281c23568d84034`.

Android reported runtime DTBO index `12`. Table entry 12 independently identifies
itself as model `SWEET`, compatible with `qcom,sdmmagpie-qrd`, MSM ID `0x16d`
(365), and board ID `0x2e`. This is strong correlation with the connected phone,
not a claim based on index numbering alone. Exact panel SKU still cannot be
selected from this evidence because the overlay contains several panel definitions.

## AVB topology

- `vbmeta.img`: minimum libavb 1.0, `SHA256_RSA2048`, flags 0, rollback index 0,
  public-key SHA-1 `b2a02f1e56e366d727a1a8e089762fe0b91bbc84`.
- It chains `vbmeta_system` at rollback-index location 2 with the same public-key
  fingerprint.
- Main descriptors cover `boot` and `dtbo` by SHA-256 hash and `mi_ext`, `odm`
  and `vendor` by dm-verity hashtrees.
- `vbmeta_system.img` covers `product`, `system` and `system_ext` by SHA-256
  dm-verity hashtrees. Its flags and rollback index are 0.
- `boot.img` and `dtbo.img` carry AVB 1.0 footers with unsigned (`NONE`) embedded
  descriptor blocks; the signed top-level `vbmeta` binds their hashes.
- `recovery.img` carries a self-contained `SHA256_RSA2048` AVB footer with flags
  0 and rollback index 1.

The AOSP tool verified the boot/DTBO footer hashes and the recovery footer/hash.
It also verified the cryptographic signatures of the two signed vbmeta structs.
Full hashtree verification was intentionally not attempted because logical
partition payloads were not unnecessarily unpacked from `super.img`.

## Super and logical partitions

`super.img` is Android sparse format 1.0: 4,096-byte blocks, 395 chunks, sparse
size 8,105,485,628 bytes and expanded size 9,126,805,504 bytes, exactly matching
the bootloader's 8.5-GiB `super` size.

Liblp geometry/header/table SHA-256 checks passed. Geometry uses 65,536-byte
metadata regions, 4,096-byte logical blocks and two metadata slots. Primary and
backup copies are valid; both slots contain the same unsuffixed layout and header
flags are 0. This metadata-slot count does not convert the verified physical boot
layout into A/B: the bootloader has no active slot and all logical names remain
unsuffixed.

Group `qti_dynamic_partitions` has a 9,122,611,200-byte maximum and
8,134,586,368 bytes allocated:

| Logical partition | Bytes | Attribute |
|---|---:|---|
| `odm` | 1,490,944 | read-only |
| `product` | 4,491,284,480 | read-only |
| `system` | 1,338,118,144 | read-only |
| `system_ext` | 806,674,432 | read-only |
| `vendor` | 1,496,735,744 | read-only |
| `mi_ext` | 282,624 | read-only |

The recovery package's `dynamic_partitions_op_list` independently lists the same
group, partition names and exact sizes.

## Temporary-boot prerequisite assessment

The stock image-format unknowns are resolved. A future RAM-boot candidate would
need to preserve at minimum:

- Android boot header v2 and 4,096-byte page alignment;
- the encoded load addresses and complete command line above;
- gzip kernel/ramdisk conventions;
- the matching embedded DTB bundle and selected DTBO relationship;
- the 128-MiB bootloader download/partition envelope and relevant AVB semantics.

A conservative first candidate could reuse the exact stock kernel and DTB with a
minimal read-only ramdisk, but it was not created. A custom kernel remains blocked
on reconciling the non-exact public 4.14.180 source with the running 4.14.190
kernel/config and required proprietary firmware. Whether this bootloader accepts
`fastboot boot` remains **UNVERIFIED** and requires separate authorization to test.

## Safety record

All work in this gate was host-side download, hashing, extraction and read-only
parsing. No Xiaomi flashing script was executed. No ADB or Fastboot device command
was needed. Nothing was written to the phone; no image was booted or flashed, AVB
was not changed, and DEVICE PHASE 2 did not start.
