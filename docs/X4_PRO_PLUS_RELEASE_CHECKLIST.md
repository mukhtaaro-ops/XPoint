# X4 Pro+ Release Checklist

This checklist is intentionally stricter than a green firmware compile. X4 Pro+ must not be treated as a stock replacement until every hard gate below is satisfied on the physical XTEINK X4 Pro.

## Automated software gates

- [ ] Source smoke checks pass.
- [ ] Full host regression suite passes.
- [ ] `pio run -e x4pro -j1` succeeds.
- [ ] Packaged firmware SHA-256 verification succeeds.
- [ ] Firmware image is no larger than the X4 Pro app0 partition (0x640000 / 6,553,600 bytes).
- [ ] Build artifact contains firmware, bootloader, partition table, ELF, map, manifest, recovery guide and this checklist.

## Stock-parity gates

- [ ] EPUB opens, renders and page-turns reliably.
- [ ] TXT opens and navigates reliably.
- [ ] XTC opens and navigates reliably.
- [ ] PDF opens, renders and supports practical page navigation at stock-equivalent quality.
- [ ] SD-card library/index refresh works after file transfer.
- [ ] USB Drive works for bulk PC transfer.
- [ ] Phone-first hotspot/browser upload works and new books appear after refresh.
- [ ] Covers/library/carousel work with real books.
- [ ] Reader position and bookmarks survive restart.
- [ ] Custom fonts work.
- [ ] Frontlight controls work.
- [ ] Touch mapping and Home/Back gestures work.
- [ ] Sleep/wake and power-off persistence work.

## X4 Pro+ feature gates

- [ ] Arabic joins correctly in real reader pages.
- [ ] Tashkil/diacritics and Qur'anic annotation marks render correctly on-device.
- [ ] Qur'an files open from `/Books/Quran`.
- [ ] Study Cards, clipping -> card conversion and persistence work.
- [ ] Prayer times are checked against a trusted reference for the configured calculation method.
- [ ] Pomodoro start/pause/resume/reset/completion works across ordinary UI use.
- [ ] QR wallet codes scan from a phone.
- [ ] Calculator arithmetic is checked with representative expressions and divide-by-zero.
- [ ] Minesweeper touch hitboxes, first-tap safety, flags, win and loss states work.
- [ ] 2048 swipes, merges, score and best-score persistence work.

## Recovery hard gates

- [ ] The user's original factory backup is exactly 16 MiB (16,777,216 bytes).
- [ ] Its SHA-256 is independently rechecked against the previously verified value.
- [ ] A forced ESP32-S3 ROM-download procedure has been physically demonstrated on this exact X4 Pro without relying on X4 Pro+ booting.
- [ ] The factory image can be restored with esptool from ROM download mode at offset `0x00000000`.
- [ ] The restored factory firmware boots and its basic reader functions are confirmed.

## Flash decision

Do **not** flash the physical reader while any hard recovery gate or PDF stock-parity gate is unchecked.

A green GitHub Actions run proves the software builds and passes automated tests. It does not prove display refresh quality, PSRAM behaviour, touch geometry, frontlight behaviour, sleep/wake reliability, Arabic shaping on the physical panel, PDF parity or recoverability.
