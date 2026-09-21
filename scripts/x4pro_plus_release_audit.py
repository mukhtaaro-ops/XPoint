#!/usr/bin/env python3
"""Software-side release audit for X4 Pro+ Build 30.

This audit is intentionally incapable of clearing physical-device gates.
A PASS means the packaged firmware is internally consistent and has adequate
flash headroom; it never means the X4 Pro is safe to flash.
"""
from __future__ import annotations

import argparse
from pathlib import Path

APP_PARTITION_BYTES = 0x640000
MIN_FLASH_HEADROOM_BYTES = 512 * 1024

REQUIRED_SOURCE_FILES = (
    "docs/X4_PRO_PLUS_RELEASE_CHECKLIST.md",
    "docs/X4_PRO_PLUS_FACTORY_RECOVERY.md",
    "scripts/x4pro_plus_preflash.py",
    "tools/x4pro_plus_phone_simulator.html",
)

PDF_TOKENS = ("PdfReader", "PDFReader", "application/pdf", '".pdf"', "'.pdf'")


def runtime_pdf_evidence(root: Path) -> list[str]:
    matches: list[str] = []
    for base in (root / "src", root / "lib"):
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix.lower() not in {".c", ".cc", ".cpp", ".h", ".hpp"}:
                continue
            text = path.read_text(encoding="utf-8", errors="ignore")
            if any(token in text for token in PDF_TOKENS):
                matches.append(str(path.relative_to(root)))
    return sorted(set(matches))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--firmware", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    root = args.root.resolve()
    firmware = args.firmware.resolve()
    problems: list[str] = []

    if not firmware.is_file():
        problems.append("firmware_missing")
        firmware_bytes = 0
    else:
        firmware_bytes = firmware.stat().st_size
        if firmware_bytes > APP_PARTITION_BYTES:
            problems.append("firmware_exceeds_app_partition")
        if APP_PARTITION_BYTES - firmware_bytes < MIN_FLASH_HEADROOM_BYTES:
            problems.append("flash_headroom_below_512KiB")

    for rel in REQUIRED_SOURCE_FILES:
        if not (root / rel).is_file():
            problems.append(f"missing:{rel}")

    simulator = root / "tools/x4pro_plus_phone_simulator.html"
    if simulator.is_file():
        sim_text = simulator.read_text(encoding="utf-8", errors="ignore")
        for marker in ("X4 Pro+ UX simulator", "Qur’an", "Minesweeper", "Phone / PC Transfer"):
            if marker not in sim_text:
                problems.append(f"simulator_missing:{marker}")

    pdf_evidence = runtime_pdf_evidence(root)
    software_ok = not problems
    headroom = max(0, APP_PARTITION_BYTES - firmware_bytes)

    lines = [
        "X4 Pro+ Build 30 release audit",
        f"software_release_gates={'PASS' if software_ok else 'FAIL'}",
        f"firmware_bytes={firmware_bytes}",
        f"app_partition_bytes={APP_PARTITION_BYTES}",
        f"flash_headroom_bytes={headroom}",
        f"pdf_runtime_implementation={'FOUND' if pdf_evidence else 'NOT_FOUND'}",
        "pdf_stock_parity=PHYSICAL_VERIFICATION_REQUIRED",
        "forced_rom_recovery=PHYSICAL_VERIFICATION_REQUIRED",
        "physical_hardware_validation=REQUIRED",
        "safe_to_flash=no",
    ]
    if pdf_evidence:
        lines.append("pdf_runtime_evidence=" + ",".join(pdf_evidence))
    if problems:
        lines.append("software_problems=" + ",".join(problems))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))
    return 0 if software_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
