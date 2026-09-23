# X4 Pro+ Build 30 — RC Audit

This audit is the source-of-truth sweep for the XTEINK X4 Pro release candidate. A software PASS is not the same as permission to flash. Physical recovery and stock-parity gates remain authoritative.

## UI / navigation

- **PASS — Home carousel implementation:** real cached EPUB/XTC covers, title, reading progress, circular recent-book navigation, buttons and touch routing exist.
- **HARDENED — Carousel geometry:** side covers now stay fully inside the outer touch thirds; the previous cover positions crossed the centre touch-zone boundary and could feel glitchy.
- **PASS — Premium Lyra treatment:** editorial fallback covers, slim selection spine, stronger selected typography and restrained rules replace large stock-style grey menu pills.
- **PASS — X4 Pro+ dashboard:** Library, Qur'an, Study Cards, Prayer, Focus, Games, Organizer, Utilities and Phone / PC Transfer are routed.
- **PASS — Organizer:** Calendar, Tasks, Notes, Cards and Clippings persist through the X4Plus JSON stores.
- **PASS — Utilities:** QR Wallet, Calculator and Saved Ayat / Notes are routed and persistent where applicable.
- **PARTIAL — Visual Library:** X4 Pro has the 3×3 library grid and touch/list fallback. Current grid cards still use metadata/fallback artwork rather than guaranteed real cached thumbnails for every library item. Home carousel does use real cached covers.

## Reader / stock formats

- **PASS — EPUB:** existing XPoint EPUB reader, pagination, bookmarks/progress, fonts and recent-book integration are retained.
- **PASS — TXT:** existing XPoint TXT reader path is retained.
- **PASS — XTC:** existing XPoint XTC reader path and thumbnail generation are retained.
- **BLOCKER — PDF:** this Build 30 branch does not contain a native PDF runtime implementation. PDF stock parity is therefore **not passed**. CrossPDF remains the selected upstream reference for a future isolated PDF integration; it must not be represented as present until compiled into this branch and exercised.

## Library / transfer / connectivity

- **PASS — SD-card library/index:** existing index/search/sort/rebuild infrastructure remains present.
- **PASS — USB Drive:** existing USB mass-storage transfer path remains present.
- **PASS — Phone-first transfer:** X4 Pro+ dashboard launches the direct hotspot flow. The device starts the `XPoint-Reader` hotspot and exposes the browser upload page.
- **PASS — OPDS / Calibre foundation:** existing network transfer routes remain intact.
- **NOT IMPLEMENTED — Native LocalSend protocol:** browser/hotspot upload is the supported phone-first workflow in this RC.

## Reading intelligence / study

- **PASS — Reading Stats:** existing global/per-book stats, streak and ETA infrastructure is retained.
- **PASS — Study Cards:** `Question :: Answer`, reveal, edit/delete and persistence exist.
- **PASS — Clippings:** reader page clipping persistence and clipping-to-study-card conversion exist.
- **PASS — Qur'an route:** dedicated `/Books/Quran` route with fallback to `/Books` exists.
- **PASS (host) / PHYSICAL PENDING — Arabic:** contextual shaping, bidi, Lam-Alef, diacritics and Qur'anic annotation regression coverage exists; the physical panel remains the final visual gate.

## Utilities

- **PASS — Prayer:** offline solar calculation exists with editable latitude, longitude, UTC offset, Fajr angle, Isha angle and Asr shadow factor (1 standard / 2 Hanafi). Accuracy must still be compared on-device with a trusted prayer-time reference for the selected method.
- **PASS — Focus / Pomodoro:** 25-minute focus / 5-minute break, start, pause, resume, reset and completed-session persistence exist. Display refresh is minute-bucketed during ordinary countdown use to avoid unnecessary e-ink churn.
- **PASS — Calculator:** controlled arithmetic parser, parentheses, unary signs, decimals, history and divide-by-zero handling exist.
- **PASS — QR Wallet:** persistent payload list and QR display exist; physical scan reliability remains an on-device gate.

## Games

- **PASS — Minesweeper:** 6×8 board, 8 mines, first-tap safety, 3×3 protection, flood reveal, dig/flag, win/loss and persistent stats exist.
- **PASS — 2048:** 4×4 board, four-direction swipe, merge/spawn, score, best score and game-over detection exist.

## Platform / parity

- **PASS (software path) — Custom fonts:** existing font manager plus Arabic fonts retained.
- **PASS (software path) — Frontlight:** existing frontlight activity/settings retained.
- **PASS (software path) — Touch/buttons:** existing mapped-input paths retained; carousel visual hit-zone mismatch was corrected in Build 30.
- **PASS (software path) — Sleep/wake/persistence:** existing power manager and persisted settings/reading state retained.
- **PASS (software path) — Battery/status:** existing status/battery rendering retained.
- **PHYSICAL PENDING:** e-ink refresh behaviour, PSRAM behaviour, touch geometry, frontlight, sleep/wake and battery reporting cannot be cleared by CI.

## CI / packaging

The dedicated `X4 Pro+ Build` workflow runs source smoke checks, host regression tests, `pio run -e x4pro`, memory-budget reporting, exact firmware preservation, packaging, SHA-256 validation and artifact upload. The package includes firmware, bootloader, partition table, ELF/map, recovery docs, pre-flash tooling, simulator, memory report and build manifest.

Generic repository-wide CI also tests unrelated targets and style checks. Those failures must not be confused with the dedicated X4 Pro+ compile/package result, but formatting/static-analysis debt should still be cleaned before merge.

## Hard blockers before the label “ready to flash”

1. **Native PDF runtime / stock-parity test is still absent.**
2. **Forced ESP32-S3 ROM-download recovery must be physically demonstrated on this exact X4 Pro without relying on X4 Pro+ booting.**
3. **The verified 16 MiB factory backup SHA-256 must be rechecked before the first custom flash.**
4. **The user-approved simulator/UX must match the intended firmware layout.**
5. **Physical panel tests must clear touch, frontlight, sleep/wake, Arabic/Qur'an rendering and QR scan gates.**

Until those gates are cleared the correct release label is **Build 30 Release Candidate — NOT YET SAFE TO FLASH**.
