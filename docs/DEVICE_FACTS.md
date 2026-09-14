# Device facts

This project targets the Xiaomi Redmi Note 10 Pro. Exact regional variant,
installed firmware/kernel, panel/touch/camera vendors, partition layout, USB speed
and decoder API remain unverified. No phone image has been built or flashed.

The pinned candidate Xiaomi sweet-r-oss source has Makefile version 4.14.180.
This is a source fact, not the running kernel version. See third_party/SOURCES.json;
the upstream Makefile itself is not redistributed in the public repository.

Device-specific unlock state, inventory, serials and account details remain local.
Consult local operator notes before hardware work. Fastboot was independently
confirmed working; there is no current evidence requiring setup changes.

Collect-DeviceInventory.ps1 records bounded read-only queries to ignored private
evidence. Pass tool paths locally or resolve them through PATH. Never promote raw
inventory to public documentation without review and removal of identifiers.
