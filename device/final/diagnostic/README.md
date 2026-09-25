# SweetDisplay first-boot diagnostic

This directory contains the public, non-proprietary source inputs for the first
temporary-boot diagnostic. It is not an Android application and does not use
Java, Binder, SurfaceFlinger, EGL, networking, ADB, MTP, NCM or HID.

The diagnostic opens the first connected DRM/KMS connector, allocates one dumb
XRGB8888 buffer, draws a SweetDisplay-owned status screen, discovers a Goodix or
generic touch device by name and `EV_ABS` capabilities, reads multitouch X/Y
coordinates, observes DWC3 via sysfs, and displays the read-only SELinux
enforcement state. It automatically requests a normal reboot after three
minutes. It never opens a block device or mounts persistent storage.

This is intentionally a disposable bring-up renderer. The selected product UI
remains native C++ over SurfaceFlinger/ANativeWindow and vendor EGL/HWC after the
minimal graphics closure is proven.

`init.sweetdisplay.rc` is a reviewed design input, not something to copy over the
stock OS. A future private ramdisk may reuse exact stock recovery `init`,
`ueventd` and compiled SELinux policy only for the bounded first diagnostic.
The recovery domain is wider than the final product should need; a dedicated
least-privilege policy remains mandatory before production.

For the host-only FINAL-BOOT 1A analysis, the same source has a compile-time
`SWEETDISPLAY_DIAGNOSTIC_V2` path. It writes bounded `SDV2` checkpoints to
RAM-backed pmsg/kmsg, emits distinct backlight pulse codes before DRM, records
the exact DRM stage and errno, and tags every requested reboot. The v2 init graph
starts this service directly from `late-init`, retains the exact stock recovery
ueventd rules and does not depend on `fs`, `post-fs-data`, `boot`, USB or a
persistent filesystem. This is diagnostic-only observability, not product
functionality.
