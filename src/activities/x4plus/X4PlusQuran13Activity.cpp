#include "X4PlusQuran13Activity.h"

#include <ArduinoJson.h>
#include <Bitmap.h>
#include <Epub/converters/PngToFramebufferConverter.h>
#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <HalStorage.h>
#include <Memory.h>
#include <PersistableStore.h>

#include <algorithm>
#include <cstdio>

#include "activities/ActivityManager.h"
#include "activities/ActivityResult.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int kPageX = 12;
constexpr int kPageY = 42;
constexpr int kPageW = 456;
constexpr int kPageH = 684;
constexpr int kOverlayX = 34;
constexpr int kOverlayY = 248;
constexpr int kOverlayW = 412;
constexpr int kOverlayH = 260;

bool endsWith(const std::string& value, const char* suffix) {
  const std::string needle(suffix);
  return value.size() >= needle.size() && value.compare(value.size() - needle.size(), needle.size(), needle) == 0;
}
}  // namespace

X4PlusQuran13Activity::X4PlusQuran13Activity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("Quran13", renderer, mappedInput) {}

void X4PlusQuran13Activity::onEnter() {
  Activity::onEnter();
  loadState();
  requestUpdate();
}

std::string X4PlusQuran13Activity::pagePath(const int pageNumber) const {
  char base[96];
  std::snprintf(base, sizeof(base), "%s/page-%03d", kAssetRoot, pageNumber);
  const std::string png = std::string(base) + ".png";
  if (Storage.exists(png.c_str())) return png;
  const std::string bmp = std::string(base) + ".bmp";
  if (Storage.exists(bmp.c_str())) return bmp;
  return png;
}

bool X4PlusQuran13Activity::pageExists(const int pageNumber) const {
  if (pageNumber < 1 || pageNumber > kPageCount) return false;
  const std::string path = pagePath(pageNumber);
  return Storage.exists(path.c_str());
}

void X4PlusQuran13Activity::loadState() {
  JsonDocument doc;
  if (!PersistableStoreBase::readDocFromFile(kStatePath, doc)) return;
  page = std::clamp(doc["page"] | 1, 1, kPageCount);
  bookmarks.clear();
  JsonArrayConst marks = doc["bookmarks"].as<JsonArrayConst>();
  for (JsonVariantConst mark : marks) {
    const int value = mark.as<int>();
    if (value >= 1 && value <= kPageCount) bookmarks.push_back(value);
  }
}

void X4PlusQuran13Activity::saveState() const {
  JsonDocument doc;
  doc["page"] = page;
  JsonArray marks = doc["bookmarks"].to<JsonArray>();
  for (const int mark : bookmarks) marks.add(mark);
  if (!PersistableStoreBase::writeDocToFile(kStatePath, doc)) LOG_ERR("Q13", "Failed to save state");
}

void X4PlusQuran13Activity::turnPage(const int delta) {
  const int next = std::clamp(page + delta, 1, kPageCount);
  if (next == page) return;
  page = next;
  overlayVisible = false;
  saveState();
  requestUpdate();
}

bool X4PlusQuran13Activity::isBookmarked() const {
  return std::find(bookmarks.begin(), bookmarks.end(), page) != bookmarks.end();
}

void X4PlusQuran13Activity::toggleBookmark() {
  const auto it = std::find(bookmarks.begin(), bookmarks.end(), page);
  if (it == bookmarks.end())
    bookmarks.push_back(page);
  else
    bookmarks.erase(it);
  saveState();
  requestUpdate();
}

void X4PlusQuran13Activity::openPageJump() {
  auto editor = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput, "Go to Mushaf page (1-847)",
                                                          std::to_string(page), 3, InputType::Text);
  if (!editor) return;
  startActivityForResult(std::move(editor), [this](const ActivityResult& result) {
    if (result.isCancelled || !std::holds_alternative<KeyboardResult>(result.data)) return;
    const std::string text = std::get<KeyboardResult>(result.data).text;
    const int next = std::atoi(text.c_str());
    if (next < 1 || next > kPageCount) return;
    page = next;
    overlayVisible = false;
    saveState();
    requestUpdate();
  });
}

void X4PlusQuran13Activity::drawMissingAssets() {
  renderer.clearScreen();
  renderer.drawCenteredText(UI_14_FONT_ID, 215, "13-line Qur'an not installed", EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, 270, "Official layout: Indopak 13 lines (Taj Company)");
  renderer.drawCenteredText(UI_10_FONT_ID, 292, "QUL / Tarteel resource 313 - 847 pages");
  renderer.drawCenteredText(UI_10_FONT_ID, 335, "Install page images to:");
  renderer.drawCenteredText(UI_10_FONT_ID, 357, kAssetRoot, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, 402, "Expected names: page-001.png ... page-847.png");
  renderer.drawCenteredText(UI_10_FONT_ID, 424, "BMP pages are also accepted.");
  renderer.drawCenteredText(UI_10_FONT_ID, 472, "The Mushaf is fixed-layout; AI is disabled here.");
}

void X4PlusQuran13Activity::drawPageImage() {
  const std::string path = pagePath(page);
  if (endsWith(path, ".png")) {
    PngToFramebufferConverter converter;
    RenderConfig config{kPageX, kPageY, kPageW, kPageH};
    if (!converter.decodeToFramebuffer(path, renderer, config)) {
      renderer.drawCenteredText(UI_10_FONT_ID, 380, "Failed to render Qur'an page");
    }
    return;
  }

  HalFile file;
  if (!Storage.openFileForRead("Q13", path, file)) {
    renderer.drawCenteredText(UI_10_FONT_ID, 380, "Failed to open Qur'an page");
    return;
  }
  Bitmap bitmap(file, true);
  if (bitmap.parseHeaders() != BmpReaderError::Ok ||
      !renderer.drawBitmap(bitmap, kPageX, kPageY, kPageW, kPageH, 0.0f, 0.0f)) {
    renderer.drawCenteredText(UI_10_FONT_ID, 380, "Failed to render Qur'an page");
  }
  file.close();
}

void X4PlusQuran13Activity::drawOverlay() {
  renderer.fillRoundedRect(kOverlayX, kOverlayY, kOverlayW, kOverlayH, 10, Color::White);
  renderer.drawRoundedRect(kOverlayX, kOverlayY, kOverlayW, kOverlayH, 2, 10, true);
  renderer.drawText(UI_14_FONT_ID, kOverlayX + 18, kOverlayY + 18, "Taj 13-line Mushaf", true, EpdFontFamily::BOLD);
  char pageLine[48];
  std::snprintf(pageLine, sizeof(pageLine), "Page %d / %d", page, kPageCount);
  renderer.drawText(UI_10_FONT_ID, kOverlayX + 18, kOverlayY + 54, pageLine, true);
  renderer.drawText(UI_10_FONT_ID, kOverlayX + 18, kOverlayY + 77, "QUL resource 313  |  fixed layout", true);
  renderer.drawText(UI_10_FONT_ID, kOverlayX + 18, kOverlayY + 100,
                    isBookmarked() ? "Bookmark: saved" : "Bookmark: not saved", true,
                    isBookmarked() ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);

  const int buttonY = kOverlayY + 142;
  constexpr int buttonH = 42;
  constexpr int gap = 8;
  const int buttonW = (kOverlayW - 36 - gap) / 2;
  renderer.drawRoundedRect(kOverlayX + 14, buttonY, buttonW, buttonH, 1, 7, true);
  renderer.drawRoundedRect(kOverlayX + 14 + buttonW + gap, buttonY, buttonW, buttonH, 1, 7, true);
  renderer.drawCenteredText(UI_10_FONT_ID, buttonY + 12, "Previous        Next", EpdFontFamily::BOLD);
  renderer.drawRoundedRect(kOverlayX + 14, buttonY + 54, buttonW, buttonH, 1, 7, true);
  renderer.drawRoundedRect(kOverlayX + 14 + buttonW + gap, buttonY + 54, buttonW, buttonH, 1, 7, true);
  renderer.drawCenteredText(UI_10_FONT_ID, buttonY + 66, "Bookmark      Go to page", EpdFontFamily::BOLD);
}

void X4PlusQuran13Activity::render(RenderLock&&) {
  if (!pageExists(page)) {
    drawMissingAssets();
    const auto labels = mappedInput.mapLabels("Back", "", "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    renderer.displayBuffer(HalDisplay::HALF_REFRESH);
    return;
  }

  renderer.clearScreen();
  drawPageImage();
  char header[64];
  std::snprintf(header, sizeof(header), "Taj 13-line  |  %d / %d%s", page, kPageCount, isBookmarked() ? "  *" : "");
  renderer.drawCenteredText(UI_10_FONT_ID, 12, header, EpdFontFamily::BOLD);
  if (overlayVisible) drawOverlay();
  const auto labels = mappedInput.mapLabels("Back", "Menu", "<", ">");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}

void X4PlusQuran13Activity::loop() {
  Activity::loop();

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    activityManager.popActivity();
    return;
  }

  if (!pageExists(page)) return;

  const auto swipe = mappedInput.wasSwipe();
  if (!overlayVisible && swipe == MappedInputManager::SwipeDir::Left) {
    turnPage(1);
    return;
  }
  if (!overlayVisible && swipe == MappedInputManager::SwipeDir::Right) {
    turnPage(-1);
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    turnPage(-1);
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    turnPage(1);
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    overlayVisible = !overlayVisible;
    requestUpdate();
    return;
  }

  if (overlayVisible) {
    int row = -1;
    const auto touch = mappedInput.rowTouch(row, kOverlayY + 142, 54, 2, kOverlayX + 14, kOverlayX + kOverlayW - 14, 42);
    if (touch == MappedInputManager::RowTouch::Up) {
      int col = -1;
      const int buttonW = (kOverlayW - 44) / 2;
      const auto column = mappedInput.colTouch(col, kOverlayX + 14, buttonW + 8, 2, kOverlayY + 142 + row * 54,
                                               kOverlayY + 142 + row * 54 + 42, buttonW);
      if (column != MappedInputManager::RowTouch::None) {
        if (row == 0 && col == 0) turnPage(-1);
        else if (row == 0 && col == 1) turnPage(1);
        else if (row == 1 && col == 0) toggleBookmark();
        else if (row == 1 && col == 1) openPageJump();
      }
      return;
    }
  } else {
    int zone = -1;
    const auto touch = mappedInput.colTouch(zone, 96, 288, 1, 160, 660, 288);
    if (touch == MappedInputManager::RowTouch::Up) {
      overlayVisible = true;
      requestUpdate();
    }
  }
}
