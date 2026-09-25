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
constexpr int kCentreCoverW = 142;
constexpr int kCentreCoverH = 200;
constexpr int kSideCoverW = 84;
constexpr int kSideCoverH = 134;
constexpr int kCentreOutline = 3;
constexpr int kMenuIconTile = 36;
constexpr int kMenuIconSize = 24;
constexpr int kMenuSidePadding = 14;
constexpr int kMenuTextGap = 11;
constexpr int kCarouselOuterMargin = 44;

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
  renderer.fillRect(x + 1, y + 1, std::max(3, w / 18), h - 2, true);
  renderer.drawIcon(CoverIcon, x + (w - 32) / 2, y + std::max(18, h / 5), 32);
  const int ruleY = y + (h * 2) / 3;
  renderer.fillRect(x + w / 5, ruleY, (w * 3) / 5, 1, true);
}

bool drawBookCover(GfxRenderer& renderer, const RecentBook& book, int x, int y, int w, int h, bool selected) {
  if (!book.coverBmpPath.empty()) {
    const std::string thumbPath =
        UITheme::getCoverThumbPath(book.coverBmpPath, Lyra3CoversMetrics::values.homeCoverHeight);
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

int Lyra3CoversTheme::getMenuRowHeight(const GfxRenderer&) const { return Lyra3CoversMetrics::values.menuRowHeight; }

void Lyra3CoversTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                      const std::function<std::string(int index)>& buttonLabel,
                                      const std::function<UIIcon(int index)>& rowIcon) const {
  const int rowH = Lyra3CoversMetrics::values.menuRowHeight;
  const int gap = Lyra3CoversMetrics::values.menuSpacing;
  const int tileW = rect.width - 2 * kMenuSidePadding;
  for (int i = 0; i < buttonCount; ++i) {
    const Rect tile{rect.x + kMenuSidePadding, rect.y + i * (rowH + gap), tileW, rowH};
    const bool selected = selectedIndex == i;

    if (selected) renderer.fillRect(tile.x, tile.y + 6, 3, rowH - 12, true);

    int textX = tile.x + 10;
    if (rowIcon) {
      const uint8_t* bmp = iconForName(rowIcon(i));
      if (bmp) {
        const int iconTileX = textX;
        const int iconTileY = tile.y + (rowH - kMenuIconTile) / 2;
        renderer.fillRoundedRect(iconTileX, iconTileY, kMenuIconTile, kMenuIconTile, 8, Color::White);
        renderer.drawRoundedRect(iconTileX, iconTileY, kMenuIconTile, kMenuIconTile, selected ? 2 : 1, 8, true);
        const int iconX = iconTileX + (kMenuIconTile - kMenuIconSize) / 2;
        const int iconY = iconTileY + (kMenuIconTile - kMenuIconSize) / 2;
        renderer.drawIcon(bmp, iconX, iconY, kMenuIconSize);
        textX += kMenuIconTile + kMenuTextGap;
      }
    }

    const std::string label = buttonLabel(i);
    const auto family = selected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
    const int lineH = renderer.getLineHeight(UI_12_FONT_ID);
    const int textY = tile.y + (rowH - lineH) / 2;
    renderer.drawText(UI_12_FONT_ID, textX, textY, label.c_str(), true, family);

    if (i + 1 < buttonCount)
      renderer.fillRect(tile.x + kMenuIconTile + 21, tile.y + rowH - 1,
                        std::max(0, tile.width - kMenuIconTile - 29), 1, true);
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

  if (!coverRendered) {
    renderer.fillRect(rect.x, rect.y, rect.width, rect.height, false);

    const int titleMaxW = std::max(100, rect.width - 116);
    const auto titleLines =
        renderer.wrappedText(UI_12_FONT_ID, recentBooks[centre].title.c_str(), titleMaxW, 2, EpdFontFamily::BOLD);
    const int titleLineH = renderer.getLineHeight(UI_12_FONT_ID);
    int titleY = rect.y + 1;
    for (const auto& line : titleLines) {
      const int lineW = renderer.getTextWidth(UI_12_FONT_ID, line.c_str(), EpdFontFamily::BOLD);
      renderer.drawText(UI_12_FONT_ID, (renderer.getScreenWidth() - lineW) / 2, titleY, line.c_str(), true,
                        EpdFontFamily::BOLD);
      titleY += titleLineH;
    }

    const int centreX = (renderer.getScreenWidth() - kCentreCoverW) / 2;
    const int centreY = rect.y + 42;
    const int sideY = centreY + (kCentreCoverH - kSideCoverH) / 2;
    const int leftX = rect.x + kCarouselOuterMargin;
    const int rightX = rect.x + rect.width - kCarouselOuterMargin - kSideCoverW;

    if (count > 1) {
      const int left = (centre + count - 1) % count;
      drawBookCover(renderer, recentBooks[left], leftX, sideY, kSideCoverW, kSideCoverH, false);
      const int right = (centre + 1) % count;
      drawBookCover(renderer, recentBooks[right], rightX, sideY, kSideCoverW, kSideCoverH, false);
    }

    drawBookCover(renderer, recentBooks[centre], centreX, centreY, kCentreCoverW, kCentreCoverH, carouselFocused);

    static constexpr std::string_view kEmpty = "";
    const std::string_view progress =
        centre < static_cast<int>(recentBookProgressLines.size()) ? recentBookProgressLines[centre] : kEmpty;
    const int progressY = centreY + kCentreCoverH + 7;
    if (!progress.empty() && progress != "-") {
      const int progressW = renderer.getTextWidth(UI_10_FONT_ID, progress.data(), EpdFontFamily::BOLD);
      renderer.drawText(UI_10_FONT_ID, (renderer.getScreenWidth() - progressW) / 2, progressY, progress.data(), true,
                        EpdFontFamily::BOLD);
    }

    constexpr int dot = 4;
    constexpr int dotGap = 7;
    const int dotsW = count * dot + std::max(0, count - 1) * dotGap;
    int dotX = (renderer.getScreenWidth() - dotsW) / 2;
    const int dotsY = rect.y + rect.height - 7;
    for (int i = 0; i < count; ++i) {
      if (i == centre)
        renderer.fillRoundedRect(dotX, dotsY, dot, dot, 2, Color::Black);
      else
        renderer.drawRoundedRect(dotX, dotsY, dot, dot, 1, 2, true);
      dotX += dot + dotGap;
    }

    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
  }
}
