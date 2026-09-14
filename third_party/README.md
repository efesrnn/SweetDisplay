# Upstream source policy

Use SOURCES.json pins. Microsoft checkout is ignored; its root LICENSE is the Microsoft Public License (MS-PL). Inspect per-file notices and preserve required license/attribution when deriving our driver. Never commit the entire upstream tree. No community kernel is selected.

Reproduce the Microsoft checkout from repository root:

```powershell
git clone --depth 1 --filter=blob:none --sparse https://github.com/microsoft/Windows-driver-samples.git third_party/upstream/Windows-driver-samples
git -C third_party/upstream/Windows-driver-samples fetch --depth 1 origin 67d81f217bc01edf7a4320e4911c11065635acfa
git -C third_party/upstream/Windows-driver-samples checkout --detach 67d81f217bc01edf7a4320e4911c11065635acfa
git -C third_party/upstream/Windows-driver-samples sparse-checkout set video/IndirectDisplay
```

The kernel source is only pinned now. When needed, clone/fetch the exact Xiaomi commit into an ignored directory on the WSL Linux filesystem (preserves case-sensitive paths). A matching running kernel/vendor build has not been established. Record compiler/config/DT provenance before building; see BUILD_DEVICE.md. Source pins do not certify that a boot image is compatible.
