#pragma once

#include <cstdint>
#include "activities/Activity.h"

class X4Plus2048Activity final : public Activity {
 public:
  X4Plus2048Activity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  enum class MoveDir : uint8_t { Left, Right, Up, Down };
  uint16_t board[4][4]{};
  uint32_t rngState = 1;
  uint32_t score = 0;
  uint32_t bestScore = 0;
  bool gameOver = false;

  void newGame();
  uint32_t nextRandom();
  void spawnTile();
  bool moveBoard(MoveDir direction);
  bool canMove() const;
  void loadStats();
  void saveStats() const;
};
