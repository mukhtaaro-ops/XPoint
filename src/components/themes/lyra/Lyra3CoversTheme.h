#pragma once

#include "components/themes/lyra/LyraTheme.h"

class GfxRenderer;

// X4 Pro+ Carousel deliberately reuses the historical LYRA_3_COVERS setting
// value so existing settings remain forward-compatible.
namespace Lyra3CoversMetrics {
constexpr ThemeMetrics values = [] {
  ThemeMetrics v = LyraMetrics::values;
  v.homeTopPadding = 50;
  v.homeCoverHeight = 220;
  v.homeCoverTileHeight = 286;
  v.homeRecentBooksCount = 5;
  v.homeMenuTopOffset = 6;
  v.menuRowHeight = 46;
  v.menuSpacing = 4;
  return v;
}();
}  // namespace Lyra3CoversMetrics

class Lyra3CoversTheme : public LyraTheme {
 public:
  int getMenuRowHeight(const GfxRenderer& renderer) const override;
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer,
                           const std::vector<std::string>& recentBookProgressLines) const override;
};
