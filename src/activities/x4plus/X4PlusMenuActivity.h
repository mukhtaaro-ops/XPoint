#pragma once
#include <array>
#include "activities/UiListActivity.h"

class X4PlusMenuActivity final : public UiListActivity {
 public:
  X4PlusMenuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
 private:
  int listCount() const override;
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  const char* headerTitle() const override;
  std::array<freeink::ui::ListItem, 4> rows{};
};
