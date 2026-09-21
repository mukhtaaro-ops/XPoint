#include "X4PlusMenuActivity.h"

#include <I18n.h>
#include <algorithm>
#include <array>
#include <climits>
#include <string>

#include "activities/ActivityManager.h"
#include "components/UITheme.h"

namespace {
constexpr std::array<UIIcon, 12> kIcons = {Recent, Bookmark, Text, Blocks, Bookmark, Book, Bookmark, Text, Recent, Recent, Blocks, Text};
}

X4PlusMenuActivity::X4PlusMenuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("X4PlusMenu", renderer, mappedInput) {}

void X4PlusMenuActivity::activateSelection() {
  switch (selectedIndex) {
    case 0: activityManager.goToX4PlusCalendar(); break;
    case 1: activityManager.goToX4PlusTasks(); break;
    case 2: activityManager.goToX4PlusNotes(); break;
    case 3: activityManager.goToX4PlusCards(); break;
    case 4: activityManager.goToX4PlusStudy(); break;
    case 5: activityManager.goToX4PlusQuran(); break;
    case 6: activityManager.goToX4PlusQuranNotes(); break;
    case 7: activityManager.goToX4PlusClippings(); break;
    case 8: activityManager.goToX4PlusPrayer(); break;
    case 9: activityManager.goToX4PlusFocus(); break;
    case 10: activityManager.goToX4PlusWallet(); break;
    case 11: activityManager.goToX4PlusCalculator(); break;
    default: break;
  }
}

void X4PlusMenuActivity::loop() {
  const auto& metrics = UITheme::getInstance().getMetrics();

  buttonNavigator.onNext([this] {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, kItemCount);
    requestUpdate();
  });
  buttonNavigator.onPrevious([this] {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, kItemCount);
    requestUpdate();
  });

  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Up) {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, kItemCount);
    requestUpdate();
    return;
  }
  if (swipe == MappedInputManager::SwipeDir::Down) {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, kItemCount);
    requestUpdate();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    activityManager.goHome(HomeMenuItem::X4PLUS_TOOLS);
    return;
  }

  const int menuTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  int touchedRow = -1;
  const int rowHeight = GUI.getMenuRowHeight(renderer);
  const auto touch = mappedInput.rowTouch(touchedRow, menuTop, rowHeight + metrics.menuSpacing, kItemCount,
                                          0, INT32_MAX, rowHeight);
  if (touch != MappedInputManager::RowTouch::None) {
    selectedIndex = std::clamp(touchedRow, 0, kItemCount - 1);
    if (touch == MappedInputManager::RowTouch::Down) requestUpdate();
    else activateSelection();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) activateSelection();
}

void X4PlusMenuActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int width = renderer.getScreenWidth();
  const int height = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, width, metrics.headerHeight}, tr(STR_X4P_TOOLS));

  const std::array<const char*, 12> labels = {
      tr(STR_X4P_CALENDAR), tr(STR_X4P_TASKS), tr(STR_X4P_NOTES), tr(STR_X4P_CARDS),
      "Study Cards", "Qur'an Reader", "Saved Ayat / Notes", "Clippings",
      "Prayer", "Focus / Pomodoro", "QR Wallet", "Calculator"};

  const int menuTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  GUI.drawButtonMenu(renderer,
                     Rect{0, menuTop, width,
                          height - menuTop - metrics.buttonHintsHeight - metrics.verticalSpacing},
                     kItemCount, selectedIndex,
                     [&labels](int index) { return std::string(labels[index]); },
                     [](int index) { return kIcons[static_cast<size_t>(index)]; });

  const auto hints = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, hints.btn1, hints.btn2, hints.btn3, hints.btn4);
  renderer.displayBuffer();
}
