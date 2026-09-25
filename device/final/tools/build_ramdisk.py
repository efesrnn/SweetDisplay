#!/usr/bin/env python3
"""Build the private FINAL-BOOT 1 ramdisk from the exact stock recovery ramdisk.

The output contains only stock recovery init/ueventd, their shared-library
closure, the exact compiled recovery SELinux policy/contexts, reviewed rc files,
and the statically linked SweetDisplay diagnostic.  It intentionally drops ADB,
fastbootd, vold, recovery/update binaries, shells and filesystem tools.
"""

from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import pathlib
import stat
import struct
from dataclasses import dataclass


@dataclass
class Entry:
    name: str
    ino: int
    mode: int
    uid: int
    gid: int
    nlink: int
    mtime: int
    devmajor: int
    devminor: int
    rdevmajor: int
    rdevminor: int
    data: bytes


def align4(value: int) -> int:
    return (value + 3) & ~3


def read_newc(payload: bytes) -> list[Entry]:
    entries: list[Entry] = []
    offset = 0
    while offset + 110 <= len(payload):
        header = payload[offset : offset + 110]
        if header[:6] not in (b"070701", b"070702"):
            raise ValueError(f"invalid newc magic at {offset}")
        fields = [int(header[6 + i * 8 : 14 + i * 8], 16) for i in range(13)]
        offset += 110
        name_size = fields[11]
        name = payload[offset : offset + name_size - 1].decode("utf-8")
        offset = align4(offset + name_size)
        data_size = fields[6]
        data = payload[offset : offset + data_size]
        offset = align4(offset + data_size)
        if name == "TRAILER!!!":
            break
        if name.startswith("/") or ".." in pathlib.PurePosixPath(name).parts:
            raise ValueError(f"unsafe cpio path: {name}")
        entries.append(
            Entry(name, fields[0], fields[1], fields[2], fields[3], fields[4],
                  fields[5], fields[7], fields[8], fields[9], fields[10], data)
        )
    return entries


def encode_header(entry: Entry, ino: int) -> bytes:
    fields = (
        ino, entry.mode, entry.uid, entry.gid, entry.nlink, entry.mtime,
        len(entry.data), entry.devmajor, entry.devminor, entry.rdevmajor,
        entry.rdevminor, len(entry.name.encode("utf-8")) + 1, 0,
    )
    return b"070701" + b"".join(f"{value:08x}".encode("ascii") for value in fields)


def write_newc(entries: list[Entry]) -> bytes:
    output = bytearray()
    for ino, entry in enumerate(entries, 1):
        name = entry.name.encode("utf-8") + b"\0"
        output += encode_header(entry, ino)
        output += name
        output += b"\0" * (align4(len(output)) - len(output))
        output += entry.data
        output += b"\0" * (align4(len(output)) - len(output))
    trailer = Entry("TRAILER!!!", 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, b"")
    output += encode_header(trailer, len(entries) + 1)
    output += b"TRAILER!!!\0"
    output += b"\0" * (align4(len(output)) - len(output))
    return bytes(output)


def keep(name: str) -> bool:
    init_libraries = {
        "ld-android.so", "libbacktrace.so", "libbase.so",
        "libbootloader_message.so", "libc.so", "libc++.so",
        "libcgrouprc.so", "libcrypto.so", "libcrypto_utils.so",
        "libcutils.so", "libdl.so", "libext2_uuid.so", "libext4_utils.so",
        "libfec.so", "libfs_mgr.so", "libgsi.so", "libhidl-gen-utils.so",
        "libjsoncpp.so", "libkeyutils.so", "liblog.so", "liblogwrap.so",
        "liblp.so", "liblzma.so", "libm.so", "libpackagelistparser.so",
        "libpcre2.so", "libprocessgroup.so", "libprocessgroup_setup.so",
        "libselinux.so", "libsparse.so", "libsquashfs_utils.so",
        "libunwindstack.so", "libz.so",
    }
    exact = {
        "init", "default.prop", "prop.default", "sepolicy",
        "plat_file_contexts", "vendor_file_contexts", "odm_file_contexts",
        "product_file_contexts", "system_ext_file_contexts",
        "plat_property_contexts", "vendor_property_contexts",
        "odm_property_contexts", "product_property_contexts",
        "system_ext_property_contexts", "system/bin/init",
        "system/bin/ueventd", "system/bin/linker64",
        "system/etc/ld.config.txt", "system/etc/cgroups.json",
        "system/etc/init/hw/init.rc", "system/etc/ueventd.rc",
    }
    directories = {
        "dev", "dev/dri", "dev/input", "linkerconfig", "proc", "sys",
        "system", "system/bin", "system/etc", "system/etc/init",
        "system/etc/init/hw", "system/lib64", "tmp",
    }
    return (name in exact or name in directories or
            name.removeprefix("system/lib64/") in init_libraries)


def regular(name: str, source: pathlib.Path, mode: int = 0o100755) -> Entry:
    return Entry(name, 0, mode, 0, 0, 1, 0, 0, 0, 0, 0, source.read_bytes())


def entry_type(mode: int) -> str:
    if stat.S_ISREG(mode):
        return "file"
    if stat.S_ISDIR(mode):
        return "directory"
    if stat.S_ISLNK(mode):
        return "symlink"
    return "other"


def entry_purpose(name: str, mode: int) -> str:
    if name == "init":
        return "kernel PID1 path"
    if name == "bin":
        return "exact stock root /bin symlink required by init vendor subcontext"
    if name == "system/bin/init":
        return "exact stock recovery init and ueventd binary"
    if name == "system/bin/ueventd":
        return "ueventd symlink"
    if name == "system/bin/linker64":
        return "dynamic interpreter for stock init"
    if name == "system/bin/recovery":
        return "SweetDisplay static diagnostic service"
    if name == "system/bin/sweetdisplay-usbd":
        return "SweetDisplay static volatile NCM IPv4/TCP service"
    if name == "config":
        return "empty runtime ConfigFS mountpoint"
    if name == "system/etc/init/hw/init.rc":
        return "minimal enforcing recovery init graph"
    if name == "system/etc/ueventd.rc":
        return "device-node ownership and firmware search rules"
    if name == "sepolicy":
        return "exact compiled stock recovery SELinux policy"
    if name.endswith("_file_contexts"):
        return "SELinux filesystem labels"
    if name.endswith("_property_contexts"):
        return "SELinux property labels"
    if name in ("default.prop", "prop.default"):
        return "recovery property input"
    if name == "system/etc/ld.config.txt":
        return "stock init linker namespace configuration"
    if name == "system/etc/cgroups.json":
        return "stock init process-group configuration"
    if name == "etc":
        return "minimal early-init configuration directory; not the stock /etc symlink"
    if name == "etc/cgroups.json":
        return "exact stock cgroups.json at the Android 11 early-init lookup path"
    if name == "acct":
        return "stock empty cpuacct cgroup mountpoint"
    if name in ("mnt", "debug_ramdisk"):
        return "required empty Android first-stage init mountpoint"
    if name == "apex":
        return "required empty Android second-stage tmpfs mountpoint"
    if name.startswith("system/lib64/"):
        return "recursive shared-library dependency of stock init"
    if stat.S_ISDIR(mode):
        return "ramdisk directory"
    return "boot-support path"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stock-recovery-ramdisk", required=True, type=pathlib.Path)
    parser.add_argument("--diagnostic", required=True, type=pathlib.Path)
    parser.add_argument("--usb-daemon", type=pathlib.Path,
                        help="optional static FINAL-USB daemon")
    parser.add_argument("--sepolicy", type=pathlib.Path,
                        help="optional reviewed enforcing binary-policy replacement")
    parser.add_argument("--init-rc", required=True, type=pathlib.Path)
    parser.add_argument("--ueventd-rc", type=pathlib.Path,
                        help="optional replacement; omit to retain exact stock recovery rules")
    parser.add_argument(
        "--required-empty-directory", action="append", default=[],
        help="copy this exact empty directory entry from stock and validate its closure")
    parser.add_argument(
        "--required-stock-symlink", action="append", default=[],
        help="copy and validate this exact stock root symlink")
    parser.add_argument(
        "--early-cgroups-config", action="store_true",
        help="add only /etc/cgroups.json for pre-trigger SetupCgroups; never expose recovery.fstab")
    parser.add_argument("--output", required=True, type=pathlib.Path)
    parser.add_argument("--manifest", type=pathlib.Path)
    args = parser.parse_args()

    original = gzip.decompress(args.stock_recovery_ramdisk.read_bytes())
    original_entries = read_newc(original)
    stock_by_name = {entry.name: entry for entry in original_entries}
    selected = [entry for entry in original_entries if keep(entry.name)]
    by_name = {entry.name: entry for entry in selected}

    for name in args.required_empty_directory:
        path = pathlib.PurePosixPath(name)
        if path.is_absolute() or ".." in path.parts or not path.parts:
            raise ValueError(f"unsafe required directory: {name}")
        stock_entry = stock_by_name.get(name)
        if stock_entry is None:
            raise ValueError(f"required directory missing from stock: {name}")
        if not stat.S_ISDIR(stock_entry.mode):
            raise ValueError(f"required path is not a stock directory: {name}")
        by_name[name] = stock_entry

    for name in args.required_stock_symlink:
        path = pathlib.PurePosixPath(name)
        if path.is_absolute() or ".." in path.parts or len(path.parts) != 1:
            raise ValueError(f"unsafe required stock root symlink: {name}")
        stock_entry = stock_by_name.get(name)
        if stock_entry is None or not stat.S_ISLNK(stock_entry.mode):
            raise ValueError(f"required stock symlink missing or wrong type: {name}")
        by_name[name] = stock_entry

    if args.early_cgroups_config:
        source = stock_by_name.get("system/etc/cgroups.json")
        if source is None or not stat.S_ISREG(source.mode):
            raise ValueError("stock system/etc/cgroups.json is missing or not a file")
        # The stock ramdisk uses /etc -> /system/etc.  Reusing that broad
        # symlink would also expose recovery.fstab and permit first-stage
        # persistent-partition mount discovery.  Supply only the one file that
        # Android 11 SetupCgroups reads before rc triggers instead.
        by_name["etc"] = Entry(
            "etc", 0, stat.S_IFDIR | 0o755, 0, 0, 2, 0, 0, 0, 0, 0, b"")
        by_name["etc/cgroups.json"] = Entry(
            "etc/cgroups.json", 0, source.mode, source.uid, source.gid,
            source.nlink, source.mtime, source.devmajor, source.devminor,
            source.rdevmajor, source.rdevminor, source.data)

    by_name["system/etc/init/hw/init.rc"] = regular(
        "system/etc/init/hw/init.rc", args.init_rc, 0o100644)
    if args.ueventd_rc:
        by_name["system/etc/ueventd.rc"] = regular(
            "system/etc/ueventd.rc", args.ueventd_rc, 0o100644)
    # The recovery path keeps stock recovery-mode detection and the existing
    # explicit u:r:recovery:s0 service label while replacing the UI executable.
    by_name["system/bin/recovery"] = regular(
        "system/bin/recovery", args.diagnostic, 0o100755)
    if args.usb_daemon:
        by_name["system/bin/sweetdisplay-usbd"] = regular(
            "system/bin/sweetdisplay-usbd", args.usb_daemon, 0o100755)
        by_name["config"] = Entry(
            "config", 0, stat.S_IFDIR | 0o755, 0, 0, 2, 0, 0, 0, 0, 0, b"")
    if args.sepolicy:
        by_name["sepolicy"] = regular("sepolicy", args.sepolicy, 0o100644)

    # Never add persistent mount points or device nodes to the archive.
    forbidden = ("data", "metadata", "cache", "dev/block")
    for name in by_name:
        if name in forbidden or any(name.startswith(item + "/") for item in forbidden):
            raise ValueError(f"forbidden persistent path selected: {name}")
        if name.startswith("mnt/"):
            raise ValueError(f"unexpected pre-populated runtime mount path: {name}")

    if args.early_cgroups_config:
        early_etc = {name for name in by_name if name == "etc" or name.startswith("etc/")}
        if early_etc != {"etc", "etc/cgroups.json"}:
            raise ValueError(f"unexpected early /etc closure: {sorted(early_etc)}")
        if by_name["etc/cgroups.json"].data != by_name["system/etc/cgroups.json"].data:
            raise ValueError("early cgroups.json differs from exact stock content")

    # Every selected child must have a real directory parent in the archive.
    # This catches packaging mistakes before the image reaches first-stage init.
    for name in by_name:
        parent = pathlib.PurePosixPath(name).parent
        while str(parent) != ".":
            parent_name = parent.as_posix()
            parent_entry = by_name.get(parent_name)
            if parent_entry is None:
                raise ValueError(f"directory closure missing {parent_name} for {name}")
            if not stat.S_ISDIR(parent_entry.mode):
                raise ValueError(f"directory closure parent is not a directory: {parent_name}")
            parent = parent.parent

    for name in args.required_empty_directory:
        stock_entry = stock_by_name[name]
        selected_entry = by_name[name]
        if (selected_entry.mode, selected_entry.uid, selected_entry.gid) != (
                stock_entry.mode, stock_entry.uid, stock_entry.gid):
            raise ValueError(f"required directory metadata changed: {name}")
        if any(candidate.startswith(name + "/") for candidate in by_name):
            raise ValueError(f"required directory is not empty in ramdisk: {name}")

    for name in args.required_stock_symlink:
        stock_entry = stock_by_name[name]
        selected_entry = by_name[name]
        if (selected_entry.mode, selected_entry.uid, selected_entry.gid,
                selected_entry.data) != (
                stock_entry.mode, stock_entry.uid, stock_entry.gid,
                stock_entry.data):
            raise ValueError(f"required stock symlink changed: {name}")

    allowed_executables = {"system/bin/init", "system/bin/linker64",
                           "system/bin/recovery"}
    if args.usb_daemon:
        allowed_executables.add("system/bin/sweetdisplay-usbd")
    actual_executables = {
        name for name, entry in by_name.items()
        if stat.S_ISREG(entry.mode) and entry.mode & 0o111
    }
    if actual_executables != allowed_executables:
        raise ValueError(f"unexpected executable closure: {actual_executables}")
    if any(stat.S_ISBLK(entry.mode) or stat.S_ISCHR(entry.mode)
           for entry in by_name.values()):
        raise ValueError("device node unexpectedly embedded in ramdisk")
    diagnostic_data = by_name["system/bin/recovery"].data
    if diagnostic_data[:4] != b"\x7fELF":
        raise ValueError("diagnostic is not ELF")
    if args.usb_daemon and by_name["system/bin/sweetdisplay-usbd"].data[:4] != b"\x7fELF":
        raise ValueError("USB daemon is not ELF")
    sensitive_markers = (b"-----BEGIN " + b"PRIVATE " + b"KEY-----",
                         b"adb" + b"key",
                         b"C:\\Users\\", b"/home/")
    for name, entry in by_name.items():
        for marker in sensitive_markers:
            if marker.lower() in entry.data.lower():
                raise ValueError(f"sensitive host/key marker in {name}")

    cpio = write_newc([by_name[name] for name in sorted(by_name)])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("wb") as raw:
        with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=0, compresslevel=9) as zipped:
            zipped.write(cpio)

    if args.manifest:
        manifest = []
        for name in sorted(by_name):
            entry = by_name[name]
            kind = entry_type(entry.mode)
            purpose = entry_purpose(name, entry.mode)
            if name == "sepolicy" and args.sepolicy:
                purpose = ("reviewed enforcing stock-derived SELinux policy with "
                           "the bounded FINAL-USB networking delta")
            manifest.append({
                "path": name,
                "mode": f"{entry.mode:07o}",
                "uid": entry.uid,
                "gid": entry.gid,
                "type": kind,
                "size": len(entry.data),
                "sha256": hashlib.sha256(entry.data).hexdigest()
                    if kind == "file" else None,
                "target": entry.data.decode("utf-8")
                    if kind == "symlink" else None,
                "purpose": purpose,
            })
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(json.dumps(manifest, indent=2) + "\n",
                                 encoding="utf-8")

    print(f"entries={len(by_name)}")
    print(f"uncompressed_bytes={len(cpio)}")
    print(f"compressed_bytes={args.output.stat().st_size}")
    print(f"sha256={hashlib.sha256(args.output.read_bytes()).hexdigest()}")
    print(f"diagnostic_sha256={hashlib.sha256(diagnostic_data).hexdigest()}")
    print("audit=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
