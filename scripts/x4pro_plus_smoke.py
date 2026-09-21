#!/usr/bin/env python3
"""Cheap CI smoke checks for X4 Pro+ invariants."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

checks = {
    "platformio.ini": ["[env:x4pro]"],
    "src/CrossPointSettings.h": ["LYRA_3_COVERS"],
    "src/activities/home/HomeActivity.cpp": ["X4PLUS_TOOLS", "LYRA_3_COVERS"],
    "src/activities/library/LibraryListActivity.cpp": ["buildHeader", "buildTabBar", "openSearch", "LibraryListActivity"],
    "src/activities/x4plus/X4PlusListActivity.cpp": ["Mode::Study", "Mode::Quran", "Mode::Clippings", "Mode::Prayer", "Mode::Focus", "Mode::Wallet", "Mode::Calculator", "Question :: Answer", "createStudyCardFromClipping", "x4plus-study.json", "x4plus-quran.json", "x4plus-clippings.json", "x4plus-prayer.json", "x4plus-focus.json", "x4plus-wallet.json", "x4plus-calculator.json", "QrDisplayActivity", "calculatePrayerTimes", "focusRemainingSeconds", "evaluateExpression"],
    "src/activities/x4plus/X4PlusMenuActivity.cpp": ["Study Cards", "Qur'an", "Clippings", "goToX4PlusStudy", "goToX4PlusQuran", "goToX4PlusClippings", "Prayer", "Focus / Pomodoro", "QR Wallet", "Calculator", "Phone / PC Transfer", "goToFileTransfer", "goToLibrary", "Games", "Minesweeper", "2048", "Organizer", "Utilities", "goToX4PlusMinesweeper", "goToX4Plus2048"],
    "src/activities/x4plus/X4PlusMinesweeperActivity.cpp": ["First tap is safe", "minesweeperWins", "toggleFlag", "allSafeRevealed"],
    "src/activities/x4plus/X4Plus2048Activity.cpp": ["Swipe the board to move tiles", "best2048", "moveBoard", "spawnTile"],
    "src/activities/reader/EpubReaderMenuActivity.cpp": ["SAVE_CLIPPING", "Save clipping"],
    "src/activities/reader/EpubReaderActivity.cpp": ["saveCurrentPageClipping", "x4plus-clippings.json", "Clipping saved"],
    "test/minibidi_arabic/MiniBidiArabicTest.cpp": ["0x06D6", "isTransparentMark"],
    "src/components/themes/lyra/Lyra3CoversTheme.cpp": ["drawRecentBookCover", "carousel", "centre"],
    "lib/I18n/translations/english.yaml": ["X4 Pro+ Carousel"],
    "docs/X4_PRO_PLUS_PRODUCT_SPEC.md": ["X4 Pro+ Cover Carousel", "Qur'an", "Arabic", "Visual library foundation"],
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
