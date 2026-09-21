#include "X4PlusMinesweeperActivity.h"

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

X4PlusMinesweeperActivity::X4PlusMinesweeperActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("X4PlusMinesweeper", renderer, mappedInput) {}

void X4PlusMinesweeperActivity::onEnter() {
  Activity::onEnter();
  loadStats();
  newGame();
  requestUpdate();
}

void X4PlusMinesweeperActivity::newGame() {
  for (int column = 0; column < kColumns; ++column) {
    for (int row = 0; row < kRows; ++row) cells[column][row] = 0;
  }
  rngState = static_cast<uint32_t>(millis()) ^ 0x9E3779B9u;
  if (rngState == 0) rngState = 1;
  status = Status::Fresh;
  flagMode = false;
  resultRecorded = false;
}

uint32_t X4PlusMinesweeperActivity::nextRandom() {
  rngState ^= rngState << 13;
  rngState ^= rngState >> 17;
  rngState ^= rngState << 5;
  return rngState;
}

bool X4PlusMinesweeperActivity::inside(const int column, const int row) const {
  return column >= 0 && column < kColumns && row >= 0 && row < kRows;
}

int X4PlusMinesweeperActivity::neighbouringMines(const int column, const int row) const {
  int count = 0;
  for (int dc = -1; dc <= 1; ++dc) {
    for (int dr = -1; dr <= 1; ++dr) {
      if (dc == 0 && dr == 0) continue;
      const int c = column + dc;
      const int r = row + dr;
      if (inside(c, r) && (cells[c][r] & kMine)) ++count;
    }
  }
  return count;
}

void X4PlusMinesweeperActivity::layMines(const int safeColumn, const int safeRow) {
  int placed = 0;
  while (placed < kMines) {
    const uint32_t roll = nextRandom() % static_cast<uint32_t>(kColumns * kRows);
    const int column = static_cast<int>(roll % kColumns);
    const int row = static_cast<int>(roll / kColumns);
    if (cells[column][row] & kMine) continue;
    if (std::abs(column - safeColumn) <= 1 && std::abs(row - safeRow) <= 1) continue;
    cells[column][row] |= kMine;
    ++placed;
  }
}

void X4PlusMinesweeperActivity::floodReveal(const int startColumn, const int startRow) {
  if (!inside(startColumn, startRow)) return;
  if (cells[startColumn][startRow] & (kRevealed | kFlagged)) return;

  uint8_t queue[kColumns * kRows][2]{};
  int head = 0;
  int tail = 0;
  cells[startColumn][startRow] |= kRevealed;
  queue[tail][0] = static_cast<uint8_t>(startColumn);
  queue[tail][1] = static_cast<uint8_t>(startRow);
  ++tail;

  while (head < tail) {
    const int column = queue[head][0];
    const int row = queue[head][1];
    ++head;
    if (neighbouringMines(column, row) != 0) continue;

    for (int dc = -1; dc <= 1; ++dc) {
      for (int dr = -1; dr <= 1; ++dr) {
        if (dc == 0 && dr == 0) continue;
        const int c = column + dc;
        const int r = row + dr;
        if (!inside(c, r)) continue;
        if (cells[c][r] & (kRevealed | kFlagged | kMine)) continue;
        cells[c][r] |= kRevealed;
        queue[tail][0] = static_cast<uint8_t>(c);
        queue[tail][1] = static_cast<uint8_t>(r);
        ++tail;
      }
    }
  }
}

bool X4PlusMinesweeperActivity::allSafeRevealed() const {
  for (int column = 0; column < kColumns; ++column) {
    for (int row = 0; row < kRows; ++row) {
      const uint8_t value = cells[column][row];
      if (!(value & kMine) && !(value & kRevealed)) return false;
    }
  }
  return true;
}

int X4PlusMinesweeperActivity::flagsPlaced() const {
  int count = 0;
  for (int column = 0; column < kColumns; ++column) {
    for (int row = 0; row < kRows; ++row) {
      if (cells[column][row] & kFlagged) ++count;
    }
  }
  return count;
}

void X4PlusMinesweeperActivity::settleResult() {
  if (resultRecorded) return;
  if (status == Status::Won)
    ++wins;
  else if (status == Status::Lost)
    ++losses;
  else
    return;
  resultRecorded = true;
  saveStats();
}

void X4PlusMinesweeperActivity::reveal(const int column, const int row) {
  if (!inside(column, row) || status == Status::Won || status == Status::Lost) return;
  if (cells[column][row] & (kRevealed | kFlagged)) return;

  if (status == Status::Fresh) {
    layMines(column, row);
    status = Status::Playing;
  }

  if (cells[column][row] & kMine) {
    cells[column][row] |= kRevealed;
    status = Status::Lost;
    settleResult();
    return;
  }

  floodReveal(column, row);
  if (allSafeRevealed()) {
    status = Status::Won;
    settleResult();
  }
}

void X4PlusMinesweeperActivity::toggleFlag(const int column, const int row) {
  if (!inside(column, row) || status == Status::Won || status == Status::Lost) return;
  if (cells[column][row] & kRevealed) return;
  cells[column][row] ^= kFlagged;
}

void X4PlusMinesweeperActivity::loadStats() {
  JsonDocument doc;
  if (!PersistableStoreBase::readDocFromFile(kStatsPath, doc)) return;
  wins = doc["minesweeperWins"] | 0;
  losses = doc["minesweeperLosses"] | 0;
}

void X4PlusMinesweeperActivity::saveStats() const {
  JsonDocument doc;
  PersistableStoreBase::readDocFromFile(kStatsPath, doc);
  doc["minesweeperWins"] = wins;
  doc["minesweeperLosses"] = losses;
  PersistableStoreBase::writeDocToFile(kStatsPath, doc);
}

void X4PlusMinesweeperActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  int x = 0;
  int y = 0;
  if (!mappedInput.wasScreenTapped(x, y)) return;

  const auto& metrics = UITheme::getInstance().getMetrics();
  const int width = renderer.getScreenWidth();
  const int height = renderer.getScreenHeight();
  const int headerBottom = metrics.topPadding + metrics.headerHeight;
  const int controlsHeight = 58;
  const int controlsTop = height - metrics.buttonHintsHeight - controlsHeight - metrics.verticalSpacing;
  const int availableHeight = controlsTop - headerBottom - 54;
  const int cell = std::min((width - 24) / kColumns, availableHeight / kRows);
  const int boardWidth = cell * kColumns;
  const int boardHeight = cell * kRows;
  const int boardLeft = (width - boardWidth) / 2;
  const int boardTop = headerBottom + 48;

  if (y >= controlsTop && y < controlsTop + controlsHeight) {
    if (x < width / 2)
      newGame();
    else
      flagMode = !flagMode;
    requestUpdate();
    return;
  }

  if (x < boardLeft || x >= boardLeft + boardWidth || y < boardTop || y >= boardTop + boardHeight) return;
  const int column = (x - boardLeft) / cell;
  const int row = (y - boardTop) / cell;
  if (flagMode)
    toggleFlag(column, row);
  else
    reveal(column, row);
  requestUpdate();
}

void X4PlusMinesweeperActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const int width = renderer.getScreenWidth();
  const int height = renderer.getScreenHeight();
  const int headerBottom = metrics.topPadding + metrics.headerHeight;
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, width, metrics.headerHeight}, "Minesweeper");

  char statusLine[72];
  const char* stateText = "First tap is safe";
  if (status == Status::Playing) stateText = flagMode ? "Flag mode" : "Dig mode";
  if (status == Status::Won) stateText = "Cleared!";
  if (status == Status::Lost) stateText = "Mine hit";
  std::snprintf(statusLine, sizeof(statusLine), "%s   Mines %d   W %d / L %d", stateText,
                std::max(0, kMines - flagsPlaced()), wins, losses);
  renderer.drawCenteredText(UI_10_FONT_ID, headerBottom + 12, statusLine);

  const int controlsHeight = 58;
  const int controlsTop = height - metrics.buttonHintsHeight - controlsHeight - metrics.verticalSpacing;
  const int availableHeight = controlsTop - headerBottom - 54;
  const int cell = std::min((width - 24) / kColumns, availableHeight / kRows);
  const int boardWidth = cell * kColumns;
  const int boardHeight = cell * kRows;
  const int boardLeft = (width - boardWidth) / 2;
  const int boardTop = headerBottom + 48;
  const int lineHeight = renderer.getLineHeight(UI_12_FONT_ID);

  for (int column = 0; column < kColumns; ++column) {
    for (int row = 0; row < kRows; ++row) {
      const int x = boardLeft + column * cell;
      const int y = boardTop + row * cell;
      const uint8_t value = cells[column][row];
      const bool revealMine = (status == Status::Lost || status == Status::Won) && (value & kMine);

      if (!(value & kRevealed) && !revealMine) renderer.fillRectDither(x + 1, y + 1, cell - 2, cell - 2, Color::LightGray);
      renderer.drawRect(x, y, cell, cell, 1, true);

      const char* text = nullptr;
      char number[2] = {'\0', '\0'};
      if (value & kFlagged) {
        text = "F";
      } else if (revealMine) {
        text = "*";
      } else if (value & kRevealed) {
        const int nearby = neighbouringMines(column, row);
        if (nearby > 0) {
          number[0] = static_cast<char>('0' + nearby);
          text = number;
        }
      }

      if (text) {
        const int textWidth = renderer.getTextWidth(UI_12_FONT_ID, text, EpdFontFamily::BOLD);
        renderer.drawText(UI_12_FONT_ID, x + (cell - textWidth) / 2, y + (cell - lineHeight) / 2, text, true,
                          EpdFontFamily::BOLD);
      }
    }
  }

  const int half = width / 2;
  renderer.drawRoundedRect(8, controlsTop, half - 12, controlsHeight, 2, 8, true);
  renderer.drawRoundedRect(half + 4, controlsTop, half - 12, controlsHeight, 2, 8, true);
  const char* newLabel = "NEW GAME";
  const char* modeLabel = flagMode ? "MODE: FLAG" : "MODE: DIG";
  const int newWidth = renderer.getTextWidth(UI_10_FONT_ID, newLabel, EpdFontFamily::BOLD);
  const int modeWidth = renderer.getTextWidth(UI_10_FONT_ID, modeLabel, EpdFontFamily::BOLD);
  renderer.drawText(UI_10_FONT_ID, (half - newWidth) / 2, controlsTop + 18, newLabel, true, EpdFontFamily::BOLD);
  renderer.drawText(UI_10_FONT_ID, half + (half - modeWidth) / 2, controlsTop + 18, modeLabel, true,
                    EpdFontFamily::BOLD);

  const auto hints = mappedInput.mapLabels(tr(STR_BACK), "", "", "");
  GUI.drawButtonHints(renderer, hints.btn1, hints.btn2, hints.btn3, hints.btn4);
  renderer.displayBuffer();
}
