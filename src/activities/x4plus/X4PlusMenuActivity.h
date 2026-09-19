#pragma once
#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

class X4PlusMenuActivity final : public Activity {
 public:
  X4PlusMenuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr int kItemCount = 4;
  ButtonNavigator buttonNavigator;
  int selectedIndex = 0;

  void activateSelection();
};
