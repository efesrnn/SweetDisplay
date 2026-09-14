# Device build plan — after unlock only

No kernel, boot image, initramfs or gadget has been built or run. Official source baseline is in third_party/SOURCES.json; published Makefile says 4.14.180. Exact compatibility with the installed Android/vendor build is unknown.

1. Owner completes Xiaomi wait and unlocks manually. Verify unlocked state read-only, record output, update DEVICE_FACTS and STATUS. Do not infer unlock from a timer expiration.
2. Complete recovery identity, stock archive/image hashes, partition/slot map, boot header, ramdisk compression, DTB/DTBO placement, AVB and rollback analysis. Recoverability is a gate before device experiments.
3. Inspect Ubuntu release and dependencies. Clone the pinned Xiaomi commit into WSL's case-sensitive Linux filesystem. Inspect AndroidKernel.mk, build.config files, product defconfigs and device trees; select matching compiler and firmware from evidence, not a generic ARM64 recipe.
4. Reproduce the stock-aligned kernel/config before changes. Audit display, input, USB peripheral/UDC, configfs, ACM, FunctionFS, HID, UVC, DRM and V4L2 options actually present in that downstream tree. Current upstream docs are API guidance, not proof of backported support.
5. Prepare a minimal ARM64 initramfs with /init, dev/proc/sys/configfs mounts, BusyBox/equivalent and serial logging. Keep filesystems read-only by default; do not mount/write user partitions. Keep original kernel charging/thermal behavior and battery; monitor early boot temperature.
6. Investigate `fastboot boot custom_boot.img` only after unlock/recovery validation. This support is UNKNOWN. If unsupported, STOP before permanent flashing and propose a specific recoverable alternative for approval.
7. First boot acceptance: reliable debug shell, preferably supported ACM/ttyGS0 over a deliberately configured gadget; capture complete boot log. Only then bring up graphics.
8. Discover DRM/KMS versus framebuffer from runtime/sysfs/config/logs. If DRM usable, ARM64 test with libdrm enumerates connector/CRTC/plane and valid formats, then presents black/white/R/G/B/checkerboard/gradient. Do not assume fb0 or that an Android DRM node supports standard userspace scanout without vendor initialization.
9. Local touch diagnostics precede forwarding. Raw 800x360@10 display transport precedes Qualcomm decoder work. Camera follows stable display and touch.

Decoder investigation: correlate /dev/video and /dev/media to driver names/capabilities, inspect device-tree/config for Venus or downstream Qualcomm vidc, firmware loading and V4L2 M2M support. Determine formats/buffer ownership and DRM import compatibility with real tests. Do not assume upstream Venus ABI matches Xiaomi's downstream kernel, nor assume MediaCodec is required. Software decoding, if used, is an explicit temporary limitation.

References: [DRM/KMS](https://docs.kernel.org/gpu/drm-kms.html), [V4L2](https://docs.kernel.org/userspace-api/media/v4l/v4l2.html), [USB configfs](https://docs.kernel.org/usb/gadget_configfs.html). No executable flashing scripts are provided.
