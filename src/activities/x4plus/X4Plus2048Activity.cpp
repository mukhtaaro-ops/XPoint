#include "X4Plus2048Activity.h"

#include <ArduinoJson.h>
#include <GfxRenderer.h>
#include <I18n.h>
#include <PersistableStore.h>

#include <algorithm>
#include <cstdio>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr const char* kStatsPath = "/.crosspoint/x4plus-games.json";
}

X4Plus2048Activity::X4Plus2048Activity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("X4Plus2048", renderer, mappedInput) {}

void X4Plus2048Activity::onEnter() {
  Activity::onEnter();
  loadStats();
  newGame();
  requestUpdate();
}

uint32_t X4Plus2048Activity::nextRandom() {
  rngState ^= rngState << 13;
  rngState ^= rngState >> 17;
  rngState ^= rngState << 5;
  return rngState;
}

void X4Plus2048Activity::newGame() {
  for (auto& row : board) {
    for (uint16_t& value : row) value = 0;
  }
  score = 0;
  gameOver = false;
  rngState = static_cast<uint32_t>(millis()) ^ 0xA341316Cu;
  if (rngState == 0) rngState = 1;
  spawnTile();
  spawnTile();
}

void X4Plus2048Activity::spawnTile() {
  uint8_t empty[16][2]{};
  int count = 0;
  for (int row = 0; row < 4; ++row) {
    for (int column = 0; column < 4; ++column) {
      if (board[row][column] == 0) {
        empty[count][0] = static_cast<uint8_t>(row);
        empty[count][1] = static_cast<uint8_t>(column);
        ++count;
      }
    }
  }
  if (count == 0) return;
  const int choice = static_cast<int>(nextRandom() % static_cast<uint32_t>(count));
  board[empty[choice][0]][empty[choice][1]] = (nextRandom() % 10u == 0u) ? 4u : 2u;
}

bool X4Plus2048Activity::moveBoard(const MoveDir direction) {
  uint16_t before[4][4]{};
  for (int row = 0; row < 4; ++row) {
    for (int column = 0; column < 4; ++column) before[row][column] = board[row][column];
  }

  for (int line = 0; line < 4; ++line) {
    uint16_t values[4]{};
    int valueCount = 0;

    for (int step = 0; step < 4; ++step) {
      int row = 0;
      int column = 0;
      switch (direction) {
        case MoveDir::Left:
          row = line;
          column = step;
          break;
        case MoveDir::Right:
          row = line;
          column = 3 - step;
          break;
        case MoveDir::Up:
          row = step;
          column = line;
          break;
        case MoveDir::Down:
          row = 3 - step;
          column = line;
          break;
      }
      if (board[row][column] != 0) values[valueCount++] = board[row][column];
    }

    uint16_t merged[4]{};
    int mergedCount = 0;
    for (int i = 0; i < valueCount;) {
      if (i + 1 < valueCount && values[i] == values[i + 1]) {
        merged[mergedCount] = static_cast<uint16_t>(values[i] * 2u);
        score += merged[mergedCount];
        ++mergedCount;
        i += 2;
      } else {
        merged[mergedCount++] = values[i++];
      }
    }

    for (int step = 0; step < 4; ++step) {
      int row = 0;
      int column = 0;
      switch (direction) {
        case MoveDir::Left:
          row = line;
          column = step;
          break;
        case MoveDir::Right:
          row = line;
          column = 3 - step;
          break;
        case MoveDir::Up:
          row = step;
          column = line;
          break;
        case MoveDir::Down:
          row = 3 - step;
          column = line;
          break;
      }
      board[row][column] = step < mergedCount ? merged[step] : 0;
    }
  }

  bool changed = false;
  for (int row = 0; row < 4; ++row) {
    for (int column = 0; column < 4; ++column) {
      if (before[row][column] != board[row][column]) {
        changed = true;
        break;
      }
    }
    if (changed) break;
  }

  if (changed) {
    spawnTile();
    if (score > bestScore) {
      bestScore = score;
      saveStats();
    }
    gameOver = !canMove();
  }
  return changed;
}

bool X4Plus2048Activity::canMove() const {
  for (int row = 0; row < 4; ++row) {
    for (int column = 0; column < 4; ++column) {
      if (board[row][column] == 0) return true;
      if (column + 1 < 4 && board[row][column] == board[row][column + 1]) return true;
      if (row + 1 < 4 && board[row][column] == board[row + 1][column]) return true;
    }
  }
  return false;
}

void X4Plus2048Activity::loadStats() {
  JsonDocument doc;
  if (!PersistableStoreBase::readDocFromFile(kStatsPath, doc)) return;
  bestScore = doc["best2048"] | 0u;
}

void X4Plus2048Activity::saveStats() const {
  JsonDocument doc;
  PersistableStoreBase::readDocFromFile(kStatsPath, doc);
  doc["best2048"] = bestScore;
  PersistableStoreBase::writeDocToFile(kStatsPath, doc);
}

void X4Plus2048Activity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  const auto swipe = mappedInput.wasSwipe();
  bool moved = false;
  switch (swipe) {
    case MappedInputManager::SwipeDir::Left: moved = moveBoard(MoveDir::Left); break;
    case MappedInputManager::SwipeDir::Right: moved = moveBoard(MoveDir::Right); break;
    case MappedInputManager::SwipeDir::Up: moved = moveBoard(MoveDir::Up); break;
    case MappedInputManager::SwipeDir::Down: moved = moveBoard(MoveDir::Down); break;
    case MappedInputManager::SwipeDir::None: break;
  }
  if (moved) {
    requestUpdate();
    return;
  }

  int x = 0;
  int y = 0;
  if (!mappedInput.wasScreenTapped(x, y)) return;
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int height = renderer.getScreenHeight();
  const int buttonTop = height - metrics.buttonHintsHeight - 66;
  if (y >= buttonTop) {
    newGame();
    requestUpdate();
  }
}

void X4Plus2048Activity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const int width = renderer.getScreenWidth();
  const int height = renderer.getScreenHeight();
  const int headerBottom = metrics.topPadding + metrics.headerHeight;

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, width, metrics.headerHeight}, "2048");

  char status[72];
  std::snprintf(status, sizeof(status), "%sScore %lu   Best %lu", gameOver ? "GAME OVER   " : "",
                static_cast<unsigned long>(score), static_cast<unsigned long>(bestScore));
  renderer.drawCenteredText(UI_10_FONT_ID, headerBottom + 18, status, true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, headerBottom + 48, "Swipe the board to move tiles");

  const int buttonTop = height - metrics.buttonHintsHeight - 66;
  const int available = buttonTop - (headerBottom + 92);
  const int cell = std::min((width - 36) / 4, available / 4);
  const int boardSize = cell * 4;
  const int left = (width - boardSize) / 2;
  const int top = headerBottom + 92;
  const int lineHeight = renderer.getLineHeight(UI_12_FONT_ID);

  for (int row = 0; row < 4; ++row) {
    for (int column = 0; column < 4; ++column) {
      const int x = left + column * cell;
      const int y = top + row * cell;
      renderer.drawRoundedRect(x, y, cell - 4, cell - 4, 2, 8, true);
      if (board[row][column] == 0) continue;

      char value[8];
      std::snprintf(value, sizeof(value), "%u", static_cast<unsigned>(board[row][column]));
      const int textWidth = renderer.getTextWidth(UI_12_FONT_ID, value, EpdFontFamily::BOLD);
      renderer.drawText(UI_12_FONT_ID, x + (cell - 4 - textWidth) / 2,
                        y + (cell - 4 - lineHeight) / 2, value, true, EpdFontFamily::BOLD);
    }
  }

  renderer.drawRoundedRect(28, buttonTop, width - 56, 54, 2, 8, true);
  const char* label = "NEW GAME";
  const int labelWidth = renderer.getTextWidth(UI_10_FONT_ID, label, EpdFontFamily::BOLD);
  renderer.drawText(UI_10_FONT_ID, (width - labelWidth) / 2, buttonTop + 16, label, true, EpdFontFamily::BOLD);

  const auto hints = mappedInput.mapLabels(tr(STR_BACK), "", "", "");
  GUI.drawButtonHints(renderer, hints.btn1, hints.btn2, hints.btn3, hints.btn4);
  renderer.displayBuffer();
}
