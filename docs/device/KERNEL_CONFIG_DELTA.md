# Kernel configuration reconciliation

Status: **COMPLETE FOR AVAILABLE EVIDENCE; SOURCE MISMATCH CONFIRMED**

## Evidence identity

| Input | Bytes | SHA-256 |
|---|---:|---|
| Running `/proc/config.gz` | 36,576 | `87b77328fbe327a086e4b1f24be198b4632c73372e7651f8bc4709f4d11e670e` |
| Stock `Image` embedded config gzip | 36,576 | `87b77328fbe327a086e4b1f24be198b4632c73372e7651f8bc4709f4d11e670e` |
| Decompressed running/embedded config | 163,492 | `b8821ce37644106a92be29b1d8350ea517fade58a2d924e4e97bc9624e441ca5` |
| MiCode root/vendor `sweet_user_defconfig` | 19,995 | `e08c9880807acb8df4e0d940e83cdf520e3eba4a51eaba7d984355ff9dab147c` |
| Static MiCode Kconfig symbol index | 18,399 symbols | `565e249be6b1fef6743c561807a841b558a5170a2e1f52abfa0c622131f328d1` |

The runtime capture and stock embedded IKCONFIG are byte-identical. This removes
ambiguity about whether the captured config belongs to the exact stock kernel
payload.

## Method and limits

Both `CONFIG_X=value` and `# CONFIG_X is not set` lines were parsed. Xiaomi's root
and vendor defconfig files were independently hashed and are byte-identical.

The defconfig is a compact input fragment, while the running file is an expanded
Kconfig output. Therefore a running option absent from the defconfig is **not** by
itself called a delta; it may be a default or dependency result. The reliable
comparisons are:

1. options explicitly present in both files;
2. explicit defconfig options absent from the running file;
3. running options whose symbol is absent from the entire published Kconfig tree.

A generated 4.14.180 config was not produced. Doing so without the exact toolchain
and then feeding it the 4.14.190 config would silently discard undefined symbols.

## Summary

| Metric | Count |
|---|---:|
| Parsed running symbols | 5,008 |
| Explicit defconfig symbols | 782 |
| Symbols present in both | 765 |
| Same value | 763 |
| Different value | 2 |
| Explicit defconfig symbols absent from running config | 17 |
| Running symbols absent from published 4.14.180 Kconfig | 34 |
| Enabled running symbols absent from published Kconfig | 20 |

### Two explicit value differences

| Symbol | MiCode defconfig | Running stock |
|---|---|---|
| `CONFIG_ANDROID_LOW_MEMORY_KILLER` | `y` | `n` |
| `CONFIG_CMDLINE` | `"ramoops_memreserve=4M"` | `"ramoops_memreserve=4M cgroup_disable=pressure"` |

### Explicit MiCode options absent from the running config

The 17 options are:

`CONFIG_BATT_VERIFY`, `CONFIG_IPA_UT`, `CONFIG_LKDTM`,
`CONFIG_MEDIA_RC_SUPPORT`, `CONFIG_MODULE_SIG_FORCE`,
`CONFIG_MODULE_SIG_SHA512`, `CONFIG_NETFILTER_XT_MATCH_QTAGUID`,
`CONFIG_QMP_DEBUGFS_CLIENT`, `CONFIG_QTI_RPM_STATS_LOG`,
`CONFIG_REGMAP_ALLOW_WRITE_DEBUGFS`, `CONFIG_STACK_HASH_ORDER_SHIFT`,
`CONFIG_TOUCHSCREEN_GOODIX_GTX9896_I2C`,
`CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_CORE`,
`CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_FW_UPDATE`,
`CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_FW_UPDATE_EXTRA_SYSFS`,
`CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_RMI_DEV`, and
`CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_TEST_REPORTING`.

Absence means the symbol does not appear in the expanded running config; it does
not alone prove whether a later source removed, renamed or conditioned it.

### Enabled running symbols not defined by the published source

These 20 symbols are the strongest direct evidence of a later downstream source
delta:

- `CONFIG_CHARGER_LN8000=y`
- `CONFIG_CJSON=y`
- `CONFIG_HWCONF_MANAGER=y`
- `CONFIG_MI_MEMORY_SYSFS=y`
- `CONFIG_MI_RECLAIM=y`
- `CONFIG_MIGT=y`
- `CONFIG_MIHW=y`
- `CONFIG_MILLET=y`
- `CONFIG_MIUI_ZRAM_MEMORY_TRACKING=y`
- `CONFIG_PERF_CRITICAL_RT_TASK=y`
- `CONFIG_RTMM=y`
- `CONFIG_SCSI_SKHPB=y`
- `CONFIG_SF_BINDER=y`
- `CONFIG_TOUCHSCREEN_FTS_DIRECTORY="focaltech_touch"`
- `CONFIG_TOUCHSCREEN_FTS_K6=y`
- `CONFIG_TOUCHSCREEN_GOODIX_GTX9896_K6=y`
- `CONFIG_UFSFEATURE=y`
- `CONFIG_UFSHPB=y`
- `CONFIG_UFSTW=y`
- `CONFIG_UFSTW_IGNORE_GUARANTEE_BIT=y`

## SweetDisplay-critical options

`implicit/absent` means the compact MiCode defconfig does not state the value; it is
not a claim that the generated source config would disable it.

| Area / symbol | MiCode defconfig | Running stock | Assessment |
|---|---|---|---|
| `CONFIG_DRM` | `y` | `y` | Core DRM aligned |
| `CONFIG_DRM_MSM` / `CONFIG_DRM_MIPI_DSI` / `CONFIG_DRM_SDE_WB` | implicit/absent | `y` | Running DRM/SDE/DSI writeback stack present |
| `CONFIG_TOUCHSCREEN_GOODIX_GTX9896_I2C` | `y` | absent | Published touch variant is not the stock one |
| `CONFIG_TOUCHSCREEN_GOODIX_GTX9896_K6` | undefined | `y` | Material missing vendor source |
| `CONFIG_TOUCHSCREEN_FTS_K6` | undefined | `y` | Alternate built-in touch support; not active-hardware proof |
| `CONFIG_TOUCHSCREEN_XIAOMI_TOUCHFEATURE` | `y` | `y` | Aligned |
| `CONFIG_USB_DWC3` / `CONFIG_USB_DWC3_MSM` | `y` / `y` | `y` / `y` | Aligned device-controller foundation |
| `CONFIG_CONFIGFS_FS` | implicit/absent | `y` | Running ConfigFS present |
| `CONFIG_USB_CONFIGFS` / `F_FS` / `NCM` / `F_HID` | all `y` | all `y` | FunctionFS/NCM/HID prototype foundation present |
| `CONFIG_USB_CONFIGFS_F_UVC` | implicit/absent | `n` | UVC is later scope and disabled in stock |
| `CONFIG_QCOM_KGSL` | `y` | `y` | Aligned GPU driver |
| `CONFIG_MSM_VIDC_V4L2` | `y` | `y` | Aligned codec driver; runtime workload untested |
| `CONFIG_DMA_SHARED_BUFFER` | implicit/absent | `y` | Running DMA-BUF framework present |
| `CONFIG_ION` / `CONFIG_ARM_SMMU` | `y` / `y` | `y` / `y` | Aligned memory/IOMMU foundation |
| `CONFIG_REGULATOR` / `CONFIG_THERMAL` / `CONFIG_QPNP_SMB5` | implicit/`y`/`y` | `y`/`y`/`y` | Running power/thermal/charging foundation present |
| `CONFIG_SCSI_UFSHCD` / `CONFIG_SCSI_UFS_QCOM` | `y` / `y` | `y` / `y` | Core UFS aligned; newer UFS extensions are source-missing |
| `CONFIG_BLK_DEV_INITRD` / `CONFIG_TMPFS` / `CONFIG_PROC_FS` / `CONFIG_SYSFS` | `y`/implicit/implicit/implicit | all `y` | Basic initramfs filesystems present |
| `CONFIG_DEVTMPFS` | implicit/absent | `n` | Minimal userspace must provide device nodes/uevent handling |
| `CONFIG_EXT4_FS` / `CONFIG_F2FS_FS` | `y` / `y` | `y` / `y` | Aligned Android data/storage filesystems |
| `CONFIG_MODULES` / `CONFIG_MODVERSIONS` | `y` / implicit | `y` / `y` | Module ABI must be treated as exact-build-sensitive |
| `CONFIG_LOCALVERSION` / `CONFIG_LOCALVERSION_AUTO` | implicit / `y` | `"-perf"` / `y` | Produces the observed style only with the correct Git state |

## Conclusion

The configuration evidence is exact, but it cannot be reproduced by the published
source without inventing or removing downstream options. The MiCode defconfig is a
strong architectural baseline for display, USB gadget, codec, GPU, memory and power
subsystems; it is not a faithful stock 4.14.190 build input. No SweetDisplay-specific
option was enabled and no source file was modified.

