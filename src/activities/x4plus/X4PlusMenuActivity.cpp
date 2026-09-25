#include "X4PlusMenuActivity.h"

#include <I18n.h>
#include <Memory.h>

#include <algorithm>
#include <climits>
#include <string>

#include "activities/ActivityManager.h"
#include "activities/x4plus/X4PlusQuran13Activity.h"
#include "activities/x4plus/X4PlusServiceActivity.h"
#include "components/UITheme.h"

X4PlusMenuActivity::X4PlusMenuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("X4PlusMenu", renderer, mappedInput) {}

int X4PlusMenuActivity::itemCount() const {
  switch (page) {
    case Page::Dashboard: return 9;
    case Page::Reading: return 5;
    case Page::Faith: return 5;
    case Page::Organizer: return 5;
    case Page::Utilities: return 6;
    case Page::Games: return 2;
  }
  return 0;
}

const char* X4PlusMenuActivity::pageTitle() const {
  switch (page) {
    case Page::Dashboard: return "X4 Pro+";
    case Page::Reading: return "Reader & Library";
    case Page::Faith: return "Faith";
    case Page::Organizer: return "Planner";
    case Page::Utilities: return "Tools";
    case Page::Games: return "Games";
  }
  return "X4 Pro+";
}

const char* X4PlusMenuActivity::itemLabel(const int index) const {
  switch (page) {
    case Page::Dashboard: {
      static const char* labels[] = {"Daily Brief", "Reader & Library", "Faith", "Planner", "INK AI",
                                     "Tools", "Games", "Phone / PC Transfer", "Settings"};
      return index >= 0 && index < 9 ? labels[index] : "";
    }
    case Page::Reading: {
      static const char* labels[] = {"Library", "Study Cards", "Clippings", "Reading Stats", "Phone / PC Transfer"};
      return index >= 0 && index < 5 ? labels[index] : "";
    }
    case Page::Faith: {
      static const char* labels[] = {"13-line Qur'an", "Salaah", "Ramadan", "Saved Ayat / Notes", "Salaah Settings"};
      return index >= 0 && index < 5 ? labels[index] : "";
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
      static const char* labels[] = {"Focus / Pomodoro", "QR Wallet", "Calculator", "Maps / Trip Packs", "News Brief",
                                     "Connected Services"};
      return index >= 0 && index < 6 ? labels[index] : "";
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
      static constexpr UIIcon icons[] = {Recent, Book, Bookmark, Text, Blocks, Blocks, Blocks, Transfer, Settings};
      return icons[std::clamp(index, 0, 8)];
    }
    case Page::Reading: {
      static constexpr UIIcon icons[] = {Library, Bookmark, Text, Chart, Transfer};
      return icons[std::clamp(index, 0, 4)];
    }
    case Page::Faith: {
      static constexpr UIIcon icons[] = {Book, Recent, Recent, Bookmark, Settings};
      return icons[std::clamp(index, 0, 4)];
    }
    case Page::Organizer: {
      static constexpr UIIcon icons[] = {Recent, Bookmark, Text, Blocks, Text};
      return icons[std::clamp(index, 0, 4)];
    }
    case Page::Utilities: {
      static constexpr UIIcon icons[] = {Recent, Blocks, Text, Recent, Text, Settings};
      return icons[std::clamp(index, 0, 5)];
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
  auto openService = [this](const X4PlusServiceActivity::Mode mode) {
    auto activity = makeUniqueNoThrow<X4PlusServiceActivity>(renderer, mappedInput, mode);
    if (activity) activityManager.pushActivity(std::move(activity));
  };
  auto openQuran13 = [this] {
    auto activity = makeUniqueNoThrow<X4PlusQuran13Activity>(renderer, mappedInput);
    if (activity) activityManager.pushActivity(std::move(activity));
  };

  switch (page) {
    case Page::Dashboard:
      switch (selectedIndex) {
        case 0: openService(X4PlusServiceActivity::Mode::DailyBrief); break;
        case 1: openPage(Page::Reading); break;
        case 2: openPage(Page::Faith); break;
        case 3: openPage(Page::Organizer); break;
        case 4: openService(X4PlusServiceActivity::Mode::Ai); break;
        case 5: openPage(Page::Utilities); break;
        case 6: openPage(Page::Games); break;
        case 7: activityManager.goToPhoneTransfer(); break;
        case 8: activityManager.goToSettings(); break;
        default: break;
      }
      break;
    case Page::Reading:
      switch (selectedIndex) {
        case 0: activityManager.goToLibrary(); break;
        case 1: activityManager.goToX4PlusStudy(); break;
        case 2: activityManager.goToX4PlusClippings(); break;
        case 3: activityManager.goToReadingStats(); break;
        case 4: activityManager.goToPhoneTransfer(); break;
        default: break;
      }
      break;
    case Page::Faith:
      switch (selectedIndex) {
        case 0: openQuran13(); break;
        case 1: openService(X4PlusServiceActivity::Mode::Prayer); break;
        case 2: openService(X4PlusServiceActivity::Mode::Ramadan); break;
        case 3: activityManager.goToX4PlusQuranNotes(); break;
        case 4: openService(X4PlusServiceActivity::Mode::PrayerSettings); break;
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
        case 0: activityManager.goToX4PlusFocus(); break;
        case 1: activityManager.goToX4PlusWallet(); break;
        case 2: activityManager.goToX4PlusCalculator(); break;
        case 3: openService(X4PlusServiceActivity::Mode::Maps); break;
        case 4: openService(X4PlusServiceActivity::Mode::News); break;
        case 5: openService(X4PlusServiceActivity::Mode::Connections); break;
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
