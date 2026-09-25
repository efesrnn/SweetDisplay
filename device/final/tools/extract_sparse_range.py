#!/usr/bin/env python3
"""Extract one byte range from an Android sparse image without expanding it all."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
from pathlib import Path


SPARSE_MAGIC = 0xED26FF3A
CHUNK_RAW = 0xCAC1
CHUNK_FILL = 0xCAC2
CHUNK_DONT_CARE = 0xCAC3
CHUNK_CRC32 = 0xCAC4
COPY_SIZE = 8 * 1024 * 1024


def parse_int(value: str) -> int:
    return int(value, 0)


def copy_exact(source, output, count: int, digest: hashlib._Hash) -> None:
    remaining = count
    while remaining:
        data = source.read(min(remaining, COPY_SIZE))
        if not data:
            raise ValueError("unexpected end of sparse RAW payload")
        output.write(data)
        digest.update(data)
        remaining -= len(data)


def write_fill(output, pattern: bytes, count: int, digest: hashlib._Hash) -> None:
    if len(pattern) != 4:
        raise ValueError("fill pattern must be four bytes")
    block = pattern * (COPY_SIZE // len(pattern))
    remaining = count
    while remaining:
        data = block[: min(remaining, len(block))]
        output.write(data)
        digest.update(data)
        remaining -= len(data)


def write_zeros(output, count: int, digest: hashlib._Hash) -> None:
    zeros = bytes(COPY_SIZE)
    remaining = count
    while remaining:
        data = zeros[: min(remaining, len(zeros))]
        output.write(data)
        digest.update(data)
        remaining -= len(data)


def extract(source_path: Path, output_path: Path, start: int, length: int) -> dict:
    if start < 0 or length <= 0:
        raise ValueError("start must be non-negative and length must be positive")
    end = start + length
    digest = hashlib.sha256()
    chunk_counts = {"raw": 0, "fill": 0, "dont_care": 0, "crc32": 0}
    intersected_chunks = 0

    with source_path.open("rb") as source:
        header_data = source.read(28)
        if len(header_data) != 28:
            raise ValueError("truncated sparse header")
        (magic, major, minor, file_header_size, chunk_header_size,
         block_size, total_blocks, total_chunks, image_checksum) = struct.unpack(
            "<I4H4I", header_data
        )
        if magic != SPARSE_MAGIC:
            raise ValueError(f"not an Android sparse image: magic 0x{magic:08x}")
        if major != 1 or file_header_size < 28 or chunk_header_size < 12:
            raise ValueError("unsupported Android sparse format")
        expanded_size = block_size * total_blocks
        if end > expanded_size:
            raise ValueError(
                f"requested end {end} exceeds expanded image size {expanded_size}"
            )
        if file_header_size > 28:
            source.seek(file_header_size - 28, os.SEEK_CUR)

        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_offset = 0
        expanded_offset = 0
        with output_path.open("xb") as output:
            for chunk_index in range(1, total_chunks + 1):
                chunk_header = source.read(chunk_header_size)
                if len(chunk_header) != chunk_header_size:
                    raise ValueError(f"truncated chunk header {chunk_index}")
                chunk_type, _reserved, chunk_blocks, total_size = struct.unpack(
                    "<2H2I", chunk_header[:12]
                )
                payload_size = total_size - chunk_header_size
                chunk_output_size = chunk_blocks * block_size
                chunk_start = expanded_offset
                chunk_end = chunk_start + chunk_output_size
                overlap_start = max(start, chunk_start)
                overlap_end = min(end, chunk_end)
                overlap_size = max(0, overlap_end - overlap_start)
                skip_output = overlap_start - chunk_start

                if chunk_type == CHUNK_RAW:
                    chunk_counts["raw"] += 1
                    if payload_size != chunk_output_size:
                        raise ValueError(f"RAW chunk {chunk_index} size mismatch")
                    if overlap_size:
                        intersected_chunks += 1
                        source.seek(skip_output, os.SEEK_CUR)
                        copy_exact(source, output, overlap_size, digest)
                        source.seek(payload_size - skip_output - overlap_size, os.SEEK_CUR)
                    else:
                        source.seek(payload_size, os.SEEK_CUR)
                elif chunk_type == CHUNK_FILL:
                    chunk_counts["fill"] += 1
                    if payload_size != 4:
                        raise ValueError(f"FILL chunk {chunk_index} size mismatch")
                    pattern = source.read(4)
                    if overlap_size:
                        intersected_chunks += 1
                        phase = skip_output % 4
                        rotated = pattern[phase:] + pattern[:phase]
                        write_fill(output, rotated, overlap_size, digest)
                elif chunk_type == CHUNK_DONT_CARE:
                    chunk_counts["dont_care"] += 1
                    if payload_size != 0:
                        raise ValueError(f"DONT_CARE chunk {chunk_index} size mismatch")
                    if overlap_size:
                        intersected_chunks += 1
                        write_zeros(output, overlap_size, digest)
                elif chunk_type == CHUNK_CRC32:
                    chunk_counts["crc32"] += 1
                    if payload_size != 4 or chunk_blocks != 0:
                        raise ValueError(f"CRC32 chunk {chunk_index} size mismatch")
                    source.seek(4, os.SEEK_CUR)
                else:
                    raise ValueError(
                        f"unknown chunk type 0x{chunk_type:04x} at chunk {chunk_index}"
                    )

                expanded_offset = chunk_end
                output_offset += overlap_size

            if expanded_offset != expanded_size:
                raise ValueError("sparse chunks do not cover the declared expanded size")
            if output_offset != length:
                raise ValueError(
                    f"wrote {output_offset} bytes but expected requested length {length}"
                )

    return {
        "source": source_path.name,
        "output": output_path.name,
        "sparse_version": f"{major}.{minor}",
        "block_size": block_size,
        "total_chunks": total_chunks,
        "expanded_size": expanded_size,
        "image_checksum": f"0x{image_checksum:08x}",
        "range_start": start,
        "range_length": length,
        "range_end": end,
        "intersected_chunks": intersected_chunks,
        "chunk_counts": chunk_counts,
        "output_sha256": digest.hexdigest(),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--start", required=True, type=parse_int)
    parser.add_argument("--length", required=True, type=parse_int)
    args = parser.parse_args()
    result = extract(args.source, args.output, args.start, args.length)
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
