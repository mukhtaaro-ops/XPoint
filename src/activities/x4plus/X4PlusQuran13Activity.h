#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "activities/Activity.h"

class X4PlusQuran13Activity final : public Activity {
 public:
  X4PlusQuran13Activity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr int kPageCount = 847;
  static constexpr const char* kAssetRoot = "/.inkos/quran13/pages";
  static constexpr const char* kStatePath = "/.crosspoint/quran13-state.json";

  int page = 1;
  bool overlayVisible = false;
  std::vector<int> bookmarks;

  std::string pagePath(int pageNumber) const;
  bool pageExists(int pageNumber) const;
  void loadState();
  void saveState() const;
  void turnPage(int delta);
  void toggleBookmark();
  bool isBookmarked() const;
  void openPageJump();
  void drawMissingAssets();
  void drawPageImage();
  void drawOverlay();
};
