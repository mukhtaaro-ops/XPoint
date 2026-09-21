# X4 Pro+ Product Specification

X4 Pro+ is the custom XTEINK X4 Pro firmware being built on the existing XPoint/X4 Pro hardware and reader foundation. The objective is one coherent firmware rather than a collection of disconnected forks.

## Product principles

- X4 Pro first: ESP32-S3, touchscreen, frontlight, USB Serial/JTAG, PSRAM and USB Drive must remain first-class.
- Reader quality first: no app feature is allowed to make EPUB rendering, page turns, sleep/wake or recovery unreliable.
- Cover-first library: books should feel visual, not like a filesystem.
- Useful offline: core reading, statistics, notes, tasks, cards and utilities work without Wi-Fi.
- Recoverable: every release keeps the documented factory-backup / ROM-loader recovery route.
- E-ink native: layouts favour low ghosting, low redraw area and deliberate touch targets.
- Arabic/Qur'an is a release requirement: Arabic text must support right-to-left layout, contextual shaping/joining, tashkil/diacritics and a suitable embedded or user-selectable Arabic font before Qur'an support is marked complete.

## Home and library

- [x] X4 Pro+ native tools entry on Home.
- [x] Calendar, Tasks, Notes and Cards persistence.
- [x] Per-book progress line on Home.
- [x] X4 Pro+ Cover Carousel: focused centre cover, adjacent covers, circular navigation, progress and page dots.
- [ ] 3x3 cover-grid Library.
- [x] Visual library foundation: indexed library, Recent/Title/Author shelves, sorting, search, grouped navigation and file-type visual rows.
- [ ] Collections / shelves: Recent, Unopened, Finished, custom collections.
- [ ] Sorting and filtering.
- [ ] Search from the visual library.
- [ ] Finished-book handling from the library.

## Reading

- [x] EPUB / TXT / XTC reader foundation.
- [ ] PDF stock-parity verification: open/render/page turn/navigation must match or improve on stock before release.
- [x] Touch gestures and X4 Pro Home/frontlight controls.
- [x] Bookmarks, dictionary, footnotes and chapter navigation.
- [x] Custom fonts / TTF reader work.
- [x] Focus-reading mode.
- [x] Arabic/RTL contextual shaping and Qur'anic-mark host regression coverage.
- [ ] Physical X4 Pro validation of Arabic font rendering, joining and tashkil/diacritics.
- [ ] Bionic Reading presentation mode.
- [x] Persistent Clippings browser foundation.
- [x] Reader `Save clipping` action persists a page-text summary into Clippings.
- [ ] Fine-grained text-selection/highlight capture (beyond page-summary clipping).
- [x] Persistent question/answer flashcard surface.
- [x] Tap a saved clipping to create an editable Study Card.
- [ ] Fine-grained selected-text highlight -> flashcard conversion.
- [ ] Auto page turn controls.
- [ ] Better end-of-book recommendations.

## Reading intelligence

- [x] Per-book reading statistics foundation.
- [x] Global stats / Reading Rhythm / Finished Books foundation.
- [ ] Daily and weekly heatmaps.
- [x] Current/longest reading streak foundation and recent active-day summaries.
- [ ] Daily / weekly reading goals.
- [x] WPM window, reading-speed foundation and per-book ETA / estimated finish date.
- [ ] Achievement system.
- [ ] Stats-first sleep screens.
- [ ] Cover-centric per-book analytics browser.

## Connectivity

- [x] Wi-Fi web transfer.
- [x] OPDS.
- [x] WebDAV / Calibre / KOReader sync foundation where supported by the base.
- [x] USB Drive.
- [ ] LocalSend receiver.
- [ ] Phone-first send-to-reader workflow.
- [ ] Instapaper/read-later import.
- [ ] Wikipedia article send/read workflow.
- [ ] OTA channel for X4 Pro+ stable / beta builds.

## X4 Pro+ apps

- [x] Calendar shell with persistence.
- [x] Tasks shell with completion state.
- [x] Notes shell.
- [x] Cards shell.
- [x] Persistent question/answer Study Cards with tap-to-reveal.
- [ ] FSRS-style scheduling / spaced-repetition queue.
- [x] Calculator expression engine with persistent history.
- [x] Functional Focus / Pomodoro timer: 25/5 presets, start/pause/resume/reset, persisted completed-session count.
- [ ] Live countdown/timer engine and completion alert.
- [x] Persistent QR / pass payload wallet.
- [x] Render stored wallet payload as scannable QR from wallet row.
- [x] Calculated Fajr/Dhuhr/Asr/Maghrib/Isha dashboard with editable latitude, longitude, UTC offset and Fajr/Isha angles.
- [x] Qur'an reader shortcut with dedicated `/Books/Quran` folder plus persistent Saved Ayat / Notes.
- [ ] Mark Qur'an rendering release-ready only after physical Arabic shaping/RTL/tashkil validation.
- [ ] Weather snapshot when online.
- [ ] Selected e-ink games: Sudoku, 2048, Minesweeper, Solitaire/FreeCell, Chess and simple puzzles.

## UX

- [x] X4 Pro+ Carousel becomes the fresh-install default.
- [x] Carousel touch zones: previous / open / next.
- [x] Compact carousel-home menu to preserve screen space.
- [x] Quick Actions foundation: direct Library and Phone / PC Transfer actions in X4 Pro+ dashboard.\n- [ ] Enrich dashboard with current-book progress, streak/goal and next-prayer summary.
- [ ] Unified icon system across reader and apps.
- [ ] Dashboard mode with current book, tasks/calendar and reading goal.
- [ ] Configurable Home layout.
- [ ] Better first-run onboarding.

## Development / test

- [x] Dedicated `x4-pro-plus` branch.
- [x] GitHub Actions X4 Pro build artifact.
- [x] Source smoke checks for X4 Pro target, carousel default/navigation, native tools, Study/Qur'an wiring and Arabic invariants.
- [x] Full host regression suite runs before X4 Pro firmware compilation.
- [ ] Real firmware simulator based on the source tree rather than a fake HTML mock.
- [ ] Automated runtime smoke tests for Home, reader navigation, stats persistence and settings migration.
- [ ] Release manifest with SHA-256 and exact flash/recovery instructions.

## Source inspirations

Feature ideas are being selectively reimplemented or ported only when technically and license compatible from the CrossPoint ecosystem: XPoint, CrossInk, CrossInk Carousel/CrumBLE, CPR-vCodex, CrossPlay, PapyriX and CrossMux. The finished product remains one X4 Pro-specific UX and codebase.
