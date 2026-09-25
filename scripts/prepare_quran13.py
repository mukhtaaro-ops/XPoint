#!/usr/bin/env python3
"""Prepare user-supplied QUL resource 313 (Indopak 13-line, Taj Company).

This tool deliberately does not download or bundle Mushaf pages or fonts. It
verifies a user-supplied 847-page image set and copies it into the filename
layout expected by X4 Pro+.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path

PAGE_COUNT = 847
EXPECTED_WIDTH = 633
EXPECTED_HEIGHT = 948
RESOURCE_ID = 313


def png_size(path: Path) -> tuple[int, int] | None:
    with path.open("rb") as f:
        head = f.read(24)
    if len(head) >= 24 and head[:8] == b"\x89PNG\r\n\x1a\n":
        return struct.unpack(">II", head[16:24])
    return None


def bmp_size(path: Path) -> tuple[int, int] | None:
    with path.open("rb") as f:
        head = f.read(26)
    if len(head) >= 26 and head[:2] == b"BM":
        width, height = struct.unpack("<ii", head[18:26])
        return abs(width), abs(height)
    return None


def image_size(path: Path) -> tuple[int, int] | None:
    return png_size(path) or bmp_size(path)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def natural_candidates(source: Path) -> list[Path]:
    files = [p for p in source.iterdir() if p.is_file() and p.suffix.lower() in {".png", ".bmp"}]
    def key(p: Path):
        digits = "".join(ch for ch in p.stem if ch.isdigit())
        return (int(digits) if digits else 10**9, p.name.lower())
    return sorted(files, key=key)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-dir", type=Path, required=True, help="folder containing the 847 official page images")
    parser.add_argument("--output-dir", type=Path, required=True, help="destination pages folder")
    parser.add_argument("--allow-dimension-mismatch", action="store_true",
                        help="copy pages even when they are not the expected 633x948 international resource size")
    args = parser.parse_args()

    source = args.source_dir.resolve()
    output = args.output_dir.resolve()
    if not source.is_dir():
        raise SystemExit(f"Source folder not found: {source}")

    pages = natural_candidates(source)
    if len(pages) != PAGE_COUNT:
        raise SystemExit(f"Expected exactly {PAGE_COUNT} PNG/BMP pages for QUL resource {RESOURCE_ID}; found {len(pages)}")

    mismatches: list[str] = []
    for index, path in enumerate(pages, 1):
        size = image_size(path)
        if size != (EXPECTED_WIDTH, EXPECTED_HEIGHT):
            mismatches.append(f"page {index}: {path.name} -> {size}")
    if mismatches and not args.allow_dimension_mismatch:
        preview = "\n".join(mismatches[:20])
        raise SystemExit(
            f"{len(mismatches)} page(s) do not match expected {EXPECTED_WIDTH}x{EXPECTED_HEIGHT}.\n{preview}\n"
            "Use --allow-dimension-mismatch only if you have independently verified the source is the correct Taj Company 13-line layout."
        )

    output.mkdir(parents=True, exist_ok=True)
    manifest = {
        "resource_id": RESOURCE_ID,
        "layout": "Indopak 13 lines (Taj Company)",
        "page_count": PAGE_COUNT,
        "expected_dimensions": [EXPECTED_WIDTH, EXPECTED_HEIGHT],
        "source": "user-supplied; not bundled by X4 Pro+",
        "pages": [],
    }
    for index, path in enumerate(pages, 1):
        ext = path.suffix.lower()
        dest = output / f"page-{index:03d}{ext}"
        shutil.copy2(path, dest)
        manifest["pages"].append({
            "page": index,
            "file": dest.name,
            "dimensions": list(image_size(dest) or (0, 0)),
            "sha256": sha256(dest),
        })

    (output.parent / "quran13-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Prepared {PAGE_COUNT} pages in {output}")
    print(f"Manifest: {output.parent / 'quran13-manifest.json'}")
    if mismatches:
        print(f"WARNING: {len(mismatches)} dimension mismatch(es) were explicitly allowed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
