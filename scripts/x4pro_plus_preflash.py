#!/usr/bin/env python3
"""Pre-flash verifier for XTEINK X4 Pro / X4 Pro+.

This script performs file-integrity checks only. It intentionally cannot mark
ROM recovery, PDF parity, display behaviour or other physical gates as passed.
"""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

FACTORY_BYTES = 16 * 1024 * 1024
APP_PARTITION_BYTES = 0x640000


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def normalized(value: str) -> str:
    return value.strip().lower().replace("sha256:", "")


def verify(path: Path, expected: str, label: str) -> bool:
    actual = sha256(path)
    ok = actual == normalized(expected)
    print(f"{label}: {path}")
    print(f"  SHA-256: {actual}")
    print(f"  expected: {normalized(expected)}")
    print(f"  result: {'PASS' if ok else 'FAIL'}")
    return ok


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--factory-backup", type=Path, required=True)
    parser.add_argument("--factory-sha256", required=True)
    parser.add_argument("--firmware", type=Path, required=True)
    parser.add_argument("--firmware-sha256", required=True)
    args = parser.parse_args()

    ok = True
    if not args.factory_backup.is_file():
        print("FAIL: factory backup does not exist")
        return 2
    size = args.factory_backup.stat().st_size
    if size != FACTORY_BYTES:
        print(f"FAIL: factory backup is {size} bytes; expected {FACTORY_BYTES}")
        ok = False
    else:
        print(f"PASS: factory backup size is {FACTORY_BYTES} bytes")

    ok = verify(args.factory_backup, args.factory_sha256, "Factory backup") and ok

    if not args.firmware.is_file():
        print("FAIL: X4 Pro+ firmware does not exist")
        return 2
    firmware_size = args.firmware.stat().st_size
    if firmware_size > APP_PARTITION_BYTES:
        print(f"FAIL: firmware is {firmware_size} bytes; app0 partition allows {APP_PARTITION_BYTES}")
        ok = False
    else:
        print(f"PASS: firmware size {firmware_size} <= app0 partition {APP_PARTITION_BYTES}")
    ok = verify(args.firmware, args.firmware_sha256, "X4 Pro+ firmware") and ok

    print()
    print("File integrity:", "PASS" if ok else "FAIL")
    print("Physical release gates are NOT checked by this script.")
    print("Required before first flash: verified forced ESP32-S3 ROM recovery + PDF stock parity.")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
