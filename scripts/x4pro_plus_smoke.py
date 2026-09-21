#!/usr/bin/env python3
"""Cheap CI smoke checks for X4 Pro+ invariants."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

checks = {
    "platformio.ini": ["[env:x4pro]"],
    "src/CrossPointSettings.h": ["LYRA_3_COVERS"],
    "src/activities/home/HomeActivity.cpp": ["X4PLUS_TOOLS", "LYRA_3_COVERS"],
    "src/components/themes/lyra/Lyra3CoversTheme.cpp": ["drawRecentBookCover", "carousel", "centre"],
    "lib/I18n/translations/english.yaml": ["X4 Pro+ Carousel"],
    "docs/X4_PRO_PLUS_PRODUCT_SPEC.md": ["X4 Pro+ Cover Carousel", "Qur'an", "Arabic"],
}

failed = False
for rel, needles in checks.items():
    path = ROOT / rel
    if not path.exists():
        print(f"FAIL missing {rel}")
        failed = True
        continue
    text = path.read_text(encoding="utf-8", errors="replace")
    for needle in needles:
        if needle not in text:
            print(f"FAIL {rel}: missing invariant {needle!r}")
            failed = True
        else:
            print(f"PASS {rel}: {needle}")

if failed:
    sys.exit(1)
print("X4 Pro+ source smoke checks passed")
