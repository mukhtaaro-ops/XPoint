#include "X4PlusMenuActivity.h"

#include <I18n.h>
#include <algorithm>
#include <climits>
#include <string>

#include "activities/ActivityManager.h"
#include "components/UITheme.h"

X4PlusMenuActivity::X4PlusMenuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("X4PlusMenu", renderer, mappedInput) {}

int X4PlusMenuActivity::itemCount() const {
  switch (page) {
    case Page::Dashboard: return 9;
    case Page::Organizer: return 5;
    case Page::Utilities: return 3;
    case Page::Games: return 2;
  }
  return 0;
}

const char* X4PlusMenuActivity::pageTitle() const {
  switch (page) {
    case Page::Dashboard: return "X4 Pro+";
    case Page::Organizer: return "Organizer";
    case Page::Utilities: return "Utilities";
    case Page::Games: return "Games";
  }
  return "X4 Pro+";
}

const char* X4PlusMenuActivity::itemLabel(const int index) const {
  switch (page) {
    case Page::Dashboard: {
      static const char* labels[] = {"Library", "Qur'an Reader", "Study Cards", "Prayer", "Focus / Pomodoro",
                                     "Games", "Organizer", "Utilities", "Phone / PC Transfer"};
      return index >= 0 && index < 9 ? labels[index] : "";
    }
    case Page::Organizer:
      switch (index) {
        case 0: return tr(STR_X4P_CALENDAR);
        case 1: return tr(STR_X4P_TASKS);
        case 2: return tr(STR_X4P_NOTES);
        case 3: return tr(STR_X4P_CARDS);
        case 4: return "Clippings";
        default: return "";
      }
    case Page::Utilities: {
      static const char* labels[] = {"QR Wallet", "Calculator", "Saved Ayat / Notes"};
      return index >= 0 && index < 3 ? labels[index] : "";
    }
    case Page::Games: {
      static const char* labels[] = {"Minesweeper", "2048"};
      return index >= 0 && index < 2 ? labels[index] : "";
    }
  }
  return "";
}

UIIcon X4PlusMenuActivity::itemIcon(const int index) const {
  switch (page) {
    case Page::Dashboard: {
      static constexpr UIIcon icons[] = {Book, Book, Bookmark, Recent, Recent, Blocks, Recent, Blocks, Blocks};
      return icons[std::clamp(index, 0, 8)];
    }
    case Page::Organizer: {
      static constexpr UIIcon icons[] = {Recent, Bookmark, Text, Blocks, Text};
      return icons[std::clamp(index, 0, 4)];
    }
    case Page::Utilities: {
      static constexpr UIIcon icons[] = {Blocks, Text, Bookmark};
      return icons[std::clamp(index, 0, 2)];
    }
    case Page::Games: {
      static constexpr UIIcon icons[] = {Blocks, Blocks};
      return icons[std::clamp(index, 0, 1)];
    }
  }
  return Blocks;
}

void X4PlusMenuActivity::openPage(const Page next) {
  page = next;
  selectedIndex = 0;
  requestUpdate();
}

void X4PlusMenuActivity::activateSelection() {
  switch (page) {
    case Page::Dashboard:
      switch (selectedIndex) {
        case 0: activityManager.goToLibrary(); break;
        case 1: activityManager.goToX4PlusQuran(); break;
        case 2: activityManager.goToX4PlusStudy(); break;
        case 3: activityManager.goToX4PlusPrayer(); break;
        case 4: activityManager.goToX4PlusFocus(); break;
        case 5: openPage(Page::Games); break;
        case 6: openPage(Page::Organizer); break;
        case 7: openPage(Page::Utilities); break;
        case 8: activityManager.goToFileTransfer(); break;
        default: break;
      }
      break;
    case Page::Organizer:
      switch (selectedIndex) {
        case 0: activityManager.goToX4PlusCalendar(); break;
        case 1: activityManager.goToX4PlusTasks(); break;
        case 2: activityManager.goToX4PlusNotes(); break;
        case 3: activityManager.goToX4PlusCards(); break;
        case 4: activityManager.goToX4PlusClippings(); break;
        default: break;
      }
      break;
    case Page::Utilities:
      switch (selectedIndex) {
        case 0: activityManager.goToX4PlusWallet(); break;
        case 1: activityManager.goToX4PlusCalculator(); break;
        case 2: activityManager.goToX4PlusQuranNotes(); break;
        default: break;
      }
      break;
    case Page::Games:
      switch (selectedIndex) {
        case 0: activityManager.goToX4PlusMinesweeper(); break;
        case 1: activityManager.goToX4Plus2048(); break;
        default: break;
      }
      break;
  }
}

void X4PlusMenuActivity::loop() {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int count = itemCount();

  buttonNavigator.onNext([this, count] {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, count);
    requestUpdate();
  });
  buttonNavigator.onPrevious([this, count] {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, count);
    requestUpdate();
  });

  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Up) {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, count);
    requestUpdate();
    return;
  }
  if (swipe == MappedInputManager::SwipeDir::Down) {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, count);
    requestUpdate();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (page != Page::Dashboard) {
      openPage(Page::Dashboard);
    } else {
      activityManager.goHome(HomeMenuItem::X4PLUS_TOOLS);
    }
    return;
  }

  const int menuTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  int touchedRow = -1;
  const int rowHeight = GUI.getMenuRowHeight(renderer);
  const auto touch = mappedInput.rowTouch(touchedRow, menuTop, rowHeight + metrics.menuSpacing, count, 0, INT32_MAX,
                                          rowHeight);
  if (touch != MappedInputManager::RowTouch::None) {
    selectedIndex = std::clamp(touchedRow, 0, count - 1);
    if (touch == MappedInputManager::RowTouch::Down)
      requestUpdate();
    else
      activateSelection();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) activateSelection();
}

void X4PlusMenuActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int width = renderer.getScreenWidth();
  const int height = renderer.getScreenHeight();
  const int count = itemCount();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, width, metrics.headerHeight}, pageTitle());

  const int menuTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  GUI.drawButtonMenu(renderer,
                     Rect{0, menuTop, width,
                          height - menuTop - metrics.buttonHintsHeight - metrics.verticalSpacing},
                     count, selectedIndex,
                     [this](int index) { return std::string(itemLabel(index)); },
                     [this](int index) { return itemIcon(index); });

  const auto hints = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, hints.btn1, hints.btn2, hints.btn3, hints.btn4);
  renderer.displayBuffer();
}
