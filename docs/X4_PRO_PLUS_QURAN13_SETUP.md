# X4 Pro+ — Taj Company 13-line Qur'an setup

X4 Pro+ uses a **fixed-layout** Mushaf mode for the 13-line Qur'an. It is intentionally separate from the normal EPUB/TXT/PDF reader so line breaks, page breaks and page numbering are never reflowed.

## Required source

- Layout: **Indopak 13 lines (Taj Company)**
- QUL / Tarteel resource ID: **313**
- Page count: **847**
- Expected international page image size: **633 × 948**
- X4 Pro+ page location: `/.inkos/quran13/pages/`
- Accepted runtime formats: PNG or BMP

X4 Pro+ does **not** bundle or automatically download the page corpus or any Mushaf font. Install only a copy you are permitted to use. This keeps licensing and provenance explicit.

## Prepare a page set

Run:

```bash
python scripts/prepare_quran13.py --source-dir /path/to/official-pages --output-dir /path/to/sd/.inkos/quran13/pages
```

The tool requires exactly 847 PNG/BMP files, checks the expected 633×948 dimensions, renames them to `page-001.*` through `page-847.*`, and writes a SHA-256 manifest beside the pages.

Do not use `--allow-dimension-mismatch` unless the source has independently been verified as the correct Taj Company 13-line resource.

## Reader behaviour

- Swipe left/right or use the page buttons to turn pages.
- Confirm / centre action opens the page overlay.
- Overlay provides previous/next, bookmark and page jump.
- Current page and bookmarks persist in `/.crosspoint/quran13-state.json`.
- The Mushaf is fixed-layout. Reader font size, line spacing and margin settings do not reflow it.
- **INK AI is disabled inside the Mushaf.** The AI feature is restricted to manually supplied prompts or, once selected-text reader integration is complete, explicitly selected text in ordinary books.

## Verification before release

Before treating the Mushaf feature as verified, test real resource-313 pages on the X4 Pro panel at the beginning, middle and end of the 847-page set; check page turning, resume, bookmark persistence, page jump, Arabic legibility and e-ink refresh behaviour.
