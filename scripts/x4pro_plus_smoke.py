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
    "src/activities/x4plus/X4PlusListActivity.cpp": ["Mode::Study", "Mode::Quran", "Mode::Clippings", "Mode::Prayer", "Mode::Focus", "Mode::Wallet", "Mode::Calculator", "Question :: Answer", "createStudyCardFromClipping", "x4plus-study.json", "x4plus-quran.json", "x4plus-clippings.json", "x4plus-prayer.json", "x4plus-focus.json", "x4plus-wallet.json", "x4plus-calculator.json", "QrDisplayActivity", "calculatePrayerTimes", "focusRemainingSeconds", "evaluateExpression", "asrShadowFactor", "remaining / 60u"],
    "src/activities/x4plus/X4PlusMenuActivity.cpp": ["Daily Brief", "Reader & Library", "Faith", "INK AI", "Connected Services", "Phone / PC Transfer", "13-line Qur'an", "Salaah Settings", "Minesweeper", "2048", "goToPhoneTransfer", "goToLibrary", "X4PlusQuran13Activity", "X4PlusServiceActivity"],
    "src/activities/x4plus/X4PlusQuran13Activity.cpp": ["kPageCount = 847", "QUL / Tarteel resource 313", "Taj 13-line Mushaf", "AI is disabled here", "quran13-state.json"],
    "src/activities/x4plus/X4PlusServiceActivity.cpp": ["Open-Meteo", "Daily Brief", "Salaah Settings", "Sunrise", "Umm al-Qura", "Hanafi", "Notion", "Quick Capture", "OpenAI-compatible", "INK AI", "RSS / Atom", "SecureHttpClient", "manual only; no background book upload"],
    "src/activities/x4plus/X4PlusMinesweeperActivity.cpp": ["First tap is safe", "minesweeperWins", "toggleFlag", "allSafeRevealed"],
    "src/activities/x4plus/X4Plus2048Activity.cpp": ["Swipe the board to move tiles", "best2048", "moveBoard", "spawnTile"],
    "src/activities/network/CrossPointWebServerActivity.cpp": ["Phone-first transfer: starting hotspot directly", "NetworkMode::CREATE_HOTSPOT"],
    "src/activities/reader/EpubReaderMenuActivity.cpp": ["SAVE_CLIPPING", "Save clipping"],
    "src/activities/reader/EpubReaderActivity.cpp": ["saveCurrentPageClipping", "x4plus-clippings.json", "Clipping saved"],
    "test/minibidi_arabic/MiniBidiArabicTest.cpp": ["0x06D6", "isTransparentMark"],
    "src/components/themes/lyra/Lyra3CoversTheme.cpp": ["drawRecentBookCover", "kMenuIconTile", "kCarouselOuterMargin", "centre"],
    "lib/I18n/translations/english.yaml": ["X4 Pro+ Carousel"],
    "docs/X4_PRO_PLUS_PRODUCT_SPEC.md": ["X4 Pro+ Cover Carousel", "Qur'an", "Arabic", "Visual library foundation"],
    "docs/X4_PRO_PLUS_QURAN13_SETUP.md": ["resource ID: **313**", "**847**", "fixed-layout", "INK AI is disabled inside the Mushaf"],
    "scripts/prepare_quran13.py": ["RESOURCE_ID = 313", "PAGE_COUNT = 847", "EXPECTED_WIDTH = 633", "EXPECTED_HEIGHT = 948"],
    "docs/X4_PRO_PLUS_RELEASE_CHECKLIST.md": ["PDF stock-parity", "forced ESP32-S3 ROM-download", "Do **not** flash"],
    "docs/X4_PRO_PLUS_FACTORY_RECOVERY.md": ["16,777,216 bytes", "0x00000000", "0x00010000"],
    "scripts/x4pro_plus_preflash.py": ["FACTORY_BYTES", "Physical release gates are NOT checked"],
    "scripts/x4pro_plus_release_audit.py": ["MIN_FLASH_HEADROOM_BYTES", "pdf_runtime_implementation", "safe_to_flash=no"],
    "tools/x4pro_plus_phone_simulator.html": ["width:480px", "height:800px", "homeCarousel", "Minesweeper", "Phone / PC Transfer", "Prayer", "Qur'an Reader", "readerMenu:readerMenu"],
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
