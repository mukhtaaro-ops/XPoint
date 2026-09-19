#include "X4PlusMenuActivity.h"
#include <I18n.h>
#include "activities/ActivityManager.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

X4PlusMenuActivity::X4PlusMenuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiListActivity("X4PlusMenu", renderer, mappedInput, true) {
  rows[0].label = tr(STR_X4P_CALENDAR); rows[0].actionValue = 0;
  rows[1].label = tr(STR_X4P_TASKS); rows[1].actionValue = 1;
  rows[2].label = tr(STR_X4P_NOTES); rows[2].actionValue = 2;
  rows[3].label = tr(STR_X4P_CARDS); rows[3].actionValue = 3;
}

int X4PlusMenuActivity::listCount() const { return static_cast<int>(rows.size()); }
const char* X4PlusMenuActivity::headerTitle() const { return tr(STR_X4P_TOOLS); }

void X4PlusMenuActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect safe = UITheme::getInstance().getScreenSafeArea(renderer, true, false);
  screen.setContentMarginFromScreen(fui::Insets{
      static_cast<int16_t>(safe.y + metrics.topPadding + metrics.headerHeight),
      static_cast<int16_t>(renderer.getScreenWidth() - (safe.x + safe.width)),
      static_cast<int16_t>(renderer.getScreenHeight() - (safe.y + safe.height) + metrics.buttonHintsHeight),
      static_cast<int16_t>(safe.x)});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));
  fui::ListProps props;
  props.items = rows.data();
  props.count = static_cast<uint16_t>(rows.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  syncListViewport(screen, props);
  screen.list(props);
}

void X4PlusMenuActivity::activateIndex(const int index) {
  app.clearTapFlash();
  switch (index) {
    case 0: activityManager.goToX4PlusCalendar(); break;
    case 1: activityManager.goToX4PlusTasks(); break;
    case 2: activityManager.goToX4PlusNotes(); break;
    case 3: activityManager.goToX4PlusCards(); break;
    default: break;
  }
}
