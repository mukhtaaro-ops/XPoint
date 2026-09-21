#pragma once
#include "activities/Activity.h"
#include "util/ButtonNavigator.h"
#include "components/UITheme.h"

class X4PlusMenuActivity final : public Activity {
 public:
  X4PlusMenuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void loop() override;
  void render(RenderLock&&) override;

 private:
  enum class Page : uint8_t { Dashboard, Organizer, Utilities, Games };

  ButtonNavigator buttonNavigator;
  Page page = Page::Dashboard;
  int selectedIndex = 0;

  int itemCount() const;
  const char* pageTitle() const;
  const char* itemLabel(int index) const;
  UIIcon itemIcon(int index) const;
  void activateSelection();
  void openPage(Page next);
};
