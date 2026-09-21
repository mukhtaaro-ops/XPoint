#pragma once

#include <cstdint>
#include "activities/Activity.h"

class X4PlusMinesweeperActivity final : public Activity {
 public:
  X4PlusMinesweeperActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr int kColumns = 6;
  static constexpr int kRows = 8;
  static constexpr int kMines = 8;
  static constexpr uint8_t kMine = 1u << 0;
  static constexpr uint8_t kRevealed = 1u << 1;
  static constexpr uint8_t kFlagged = 1u << 2;

  enum class Status : uint8_t { Fresh, Playing, Won, Lost };

  uint8_t cells[kColumns][kRows]{};
  uint32_t rngState = 1;
  Status status = Status::Fresh;
  bool flagMode = false;
  int wins = 0;
  int losses = 0;
  bool resultRecorded = false;

  void newGame();
  uint32_t nextRandom();
  bool inside(int column, int row) const;
  int neighbouringMines(int column, int row) const;
  void layMines(int safeColumn, int safeRow);
  void floodReveal(int column, int row);
  void reveal(int column, int row);
  void toggleFlag(int column, int row);
  bool allSafeRevealed() const;
  int flagsPlaced() const;
  void settleResult();
  void loadStats();
  void saveStats() const;
};
