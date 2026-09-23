#include "Lyra3CoversTheme.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalStorage.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "components/icons/blocks.h"
#include "components/icons/book.h"
#include "components/icons/bookmark.h"
#include "components/icons/chartbar.h"
#include "components/icons/cover.h"
#include "components/icons/folder.h"
#include "components/icons/library.h"
#include "components/icons/recent.h"
#include "components/icons/settings2.h"
#include "components/icons/transfer.h"
#include "fontIds.h"

namespace {
constexpr int kCornerRadius = 7;
constexpr int kCentreCoverW = 150;
constexpr int kCentreCoverH = 208;
constexpr int kSideCoverW = 92;
constexpr int kSideCoverH = 146;
constexpr int kCentreOutline = 3;
constexpr int kMenuIconSize = 32;
constexpr int kMenuSidePadding = 18;
constexpr int kMenuTextGap = 10;
constexpr int kCarouselOuterMargin = 34;

const uint8_t* iconForName(UIIcon icon) {
  switch (icon) {
    case UIIcon::Folder: return FolderIcon;
    case UIIcon::Book: return BookIcon;
    case UIIcon::Recent: return RecentIcon;
    case UIIcon::Settings: return Settings2Icon;
    case UIIcon::Transfer: return TransferIcon;
    case UIIcon::Library: return LibraryIcon;
    case UIIcon::Bookmark: return BookmarkIcon;
    case UIIcon::Chart: return ChartBarIcon;
    case UIIcon::Blocks: return BlocksIcon;
    default: return nullptr;
  }
}

void drawFallbackCover(GfxRenderer& renderer, int x, int y, int w, int h, bool selected) {
  renderer.fillRoundedRect(x, y, w, h, kCornerRadius, Color::White);
  renderer.drawRoundedRect(x, y, w, h, selected ? kCentreOutline : 1, kCornerRadius, true);
  // A restrained editorial fallback rather than a large generic black block:
  // the narrow spine gives the card a physical-book cue while leaving useful
  // white space around the cover glyph.
  renderer.fillRect(x + 1, y + 1, std::max(3, w / 18), h - 2, true);
  renderer.drawIcon(CoverIcon, x + (w - 32) / 2, y + std::max(18, h / 5), 32);
  const int ruleY = y + (h * 2) / 3;
  renderer.fillRect(x + w / 5, ruleY, (w * 3) / 5, 1, true);
}

bool drawBookCover(GfxRenderer& renderer, const RecentBook& book, int x, int y, int w, int h, bool selected) {
  if (!book.coverBmpPath.empty()) {
    const std::string thumbPath = UITheme::getCoverThumbPath(book.coverBmpPath, Lyra3CoversMetrics::values.homeCoverHeight);
    HalFile file;
    if (Storage.openFileForRead("HOME", thumbPath, file)) {
      Bitmap bitmap(file);
      if (bitmap.parseHeaders() == BmpReaderError::Ok && bitmap.getWidth() > 0 && bitmap.getHeight() > 0) {
        const float srcRatio = static_cast<float>(bitmap.getWidth()) / static_cast<float>(bitmap.getHeight());
        const float dstRatio = static_cast<float>(w) / static_cast<float>(h);
        const float cropX = srcRatio > dstRatio ? std::max(0.0f, 1.0f - (dstRatio / srcRatio)) : 0.0f;
        const float cropY = srcRatio < dstRatio ? std::max(0.0f, 1.0f - (srcRatio / dstRatio)) : 0.0f;
        renderer.drawBitmap(bitmap, x, y, w, h, cropX, cropY);
        renderer.maskRoundedRectOutsideCorners(x, y, w, h, kCornerRadius, Color::White);
        renderer.drawRoundedRect(x, y, w, h, selected ? kCentreOutline : 1, kCornerRadius, true);
        file.close();
        return true;
      }
      file.close();
    }
  }
  drawFallbackCover(renderer, x, y, w, h, selected);
  return false;
}
}  // namespace

int Lyra3CoversTheme::getMenuRowHeight(const GfxRenderer&) const {
  return Lyra3CoversMetrics::values.menuRowHeight;
}

void Lyra3CoversTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                      const std::function<std::string(int index)>& buttonLabel,
                                      const std::function<UIIcon(int index)>& rowIcon) const {
  const int rowH = Lyra3CoversMetrics::values.menuRowHeight;
  const int gap = Lyra3CoversMetrics::values.menuSpacing;
  const int tileW = rect.width - 2 * kMenuSidePadding;
  for (int i = 0; i < buttonCount; ++i) {
    const Rect tile{rect.x + kMenuSidePadding, rect.y + i * (rowH + gap), tileW, rowH};
    const bool selected = selectedIndex == i;

    // Lyra deliberately avoids the stock "large grey rounded button" look.
    // Selection is a slim book-spine marker plus stronger type; separators keep
    // the launcher readable on e-ink without filling large regions grey.
    if (selected) renderer.fillRect(tile.x, tile.y + 5, 4, rowH - 10, true);

    int textX = tile.x + 12;
    if (rowIcon) {
      const uint8_t* bmp = iconForName(rowIcon(i));
      if (bmp) {
        const int iconY = tile.y + (rowH - kMenuIconSize) / 2;
        renderer.drawIcon(bmp, textX, iconY, kMenuIconSize);
        textX += kMenuIconSize + kMenuTextGap;
      }
    }

    const std::string label = buttonLabel(i);
    const auto family = selected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
    const int lineH = renderer.getLineHeight(UI_12_FONT_ID);
    const int textY = tile.y + (rowH - lineH) / 2;
    renderer.drawText(UI_12_FONT_ID, textX, textY, label.c_str(), true, family);

    // Keep each launcher row visually anchored. The last few pixels are left
    // clear so the rule never reads as a touch target.
    if (i + 1 < buttonCount) renderer.fillRect(tile.x + 8, tile.y + rowH - 1, tile.width - 16, 1, true);
  }
}

void Lyra3CoversTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect,
                                           const std::vector<RecentBook>& recentBooks, int selectorIndex,
                                           bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                                           std::function<bool()> storeCoverBuffer,
                                           const std::vector<std::string>& recentBookProgressLines) const {
  (void)bufferRestored;
  if (recentBooks.empty()) {
    drawEmptyRecents(renderer, rect);
    return;
  }

  const int count = static_cast<int>(recentBooks.size());
  const bool carouselFocused = selectorIndex >= 0 && selectorIndex < count;
  int centre = carouselFocused ? selectorIndex : (-selectorIndex - 1);
  centre = std::clamp(centre, 0, count - 1);

  // The entire carousel composition changes when the centre book changes, so
  // HomeActivity invalidates this cached region on every carousel movement.
  if (!coverRendered) {
    renderer.fillRect(rect.x, rect.y, rect.width, rect.height, false);

    const int titleMaxW = std::max(100, rect.width - 96);
    const auto titleLines = renderer.wrappedText(UI_12_FONT_ID, recentBooks[centre].title.c_str(), titleMaxW, 2,
                                                 EpdFontFamily::BOLD);
    const int titleLineH = renderer.getLineHeight(UI_12_FONT_ID);
    int titleY = rect.y + 1;
    for (const auto& line : titleLines) {
      const int lineW = renderer.getTextWidth(UI_12_FONT_ID, line.c_str(), EpdFontFamily::BOLD);
      renderer.drawText(UI_12_FONT_ID, (renderer.getScreenWidth() - lineW) / 2, titleY, line.c_str(), true,
                        EpdFontFamily::BOLD);
      titleY += titleLineH;
    }

    const int centreX = (renderer.getScreenWidth() - kCentreCoverW) / 2;
    const int centreY = rect.y + 44;
    const int sideY = centreY + (kCentreCoverH - kSideCoverH) / 2 + 4;

    // Keep side covers completely inside the outer thirds used by HomeActivity
    // touch routing. The old 83/305px positions straddled the 160/320px zone
    // boundaries, which made a visually-right-cover tap sometimes target the
    // centre zone and feel glitchy. These positions are symmetric and stable.
    const int leftX = rect.x + kCarouselOuterMargin;
    const int rightX = rect.x + rect.width - kCarouselOuterMargin - kSideCoverW;

    if (count > 1) {
      const int left = (centre + count - 1) % count;
      drawBookCover(renderer, recentBooks[left], leftX, sideY, kSideCoverW, kSideCoverH, false);
    }
    if (count > 2) {
      const int right = (centre + 1) % count;
      drawBookCover(renderer, recentBooks[right], rightX, sideY, kSideCoverW, kSideCoverH, false);
    }

    drawBookCover(renderer, recentBooks[centre], centreX, centreY, kCentreCoverW, kCentreCoverH, carouselFocused);

    static constexpr std::string_view kEmpty = "";
    const std::string_view progress =
        centre < static_cast<int>(recentBookProgressLines.size()) ? recentBookProgressLines[centre] : kEmpty;
    if (!progress.empty() && progress != "-") {
      const int progressW = renderer.getTextWidth(UI_10_FONT_ID, progress.data(), EpdFontFamily::BOLD);
      renderer.drawText(UI_10_FONT_ID, (renderer.getScreenWidth() - progressW) / 2,
                        centreY + kCentreCoverH + 7, progress.data(), true, EpdFontFamily::BOLD);
    }

    // Page dots are capped to the recent-book carousel size and positioned as a
    // quiet footer rather than competing with the cover art.
    constexpr int dot = 4;
    constexpr int gap = 6;
    const int dotsW = count * dot + (count - 1) * gap;
    int dotX = (renderer.getScreenWidth() - dotsW) / 2;
    const int dotsY = rect.y + rect.height - 8;
    for (int i = 0; i < count; ++i) {
      if (i == centre)
        renderer.fillRect(dotX, dotsY, dot, dot, true);
      else
        renderer.drawRect(dotX, dotsY, dot, dot, true);
      dotX += dot + gap;
    }

    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
  }
}
