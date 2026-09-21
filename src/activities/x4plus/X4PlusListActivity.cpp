#include "X4PlusListActivity.h"

#include <ArduinoJson.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>
#include <PersistableStore.h>
#include <HalClock.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "activities/ActivityManager.h"
#include "activities/ActivityResult.h"
#include "activities/reader/QrDisplayActivity.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

namespace {
constexpr double kPi = 3.14159265358979323846;

double degToRad(const double degrees) { return degrees * kPi / 180.0; }
double radToDeg(const double radians) { return radians * 180.0 / kPi; }

int dayOfYear(const int year, const int month, const int day) {
  static constexpr int beforeMonth[] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  int result = beforeMonth[std::clamp(month, 1, 12)] + day;
  if (month > 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) ++result;
  return result;
}

double hourAngleDegrees(const double latitudeDeg, const double declinationRad, const double altitudeDeg) {
  const double lat = degToRad(latitudeDeg);
  const double altitude = degToRad(altitudeDeg);
  const double denom = std::cos(lat) * std::cos(declinationRad);
  if (std::abs(denom) < 1e-9) return 0.0;
  double value = (std::sin(altitude) - std::sin(lat) * std::sin(declinationRad)) / denom;
  value = std::clamp(value, -1.0, 1.0);
  return radToDeg(std::acos(value));
}

struct PrayerTimes {
  int fajr = 0;
  int dhuhr = 0;
  int asr = 0;
  int maghrib = 0;
  int isha = 0;
};

PrayerTimes calculatePrayerTimes(const int year, const int month, const int day, const double latitude,
                                 const double longitude, const int utcOffsetMinutes, const double fajrAngle,
                                 const double ishaAngle, const double asrShadowFactor) {
  const int n = dayOfYear(year, month, day);
  const double gamma = 2.0 * kPi / 365.0 * (static_cast<double>(n) - 1.0);
  const double eqTime =
      229.18 * (0.000075 + 0.001868 * std::cos(gamma) - 0.032077 * std::sin(gamma) -
                0.014615 * std::cos(2.0 * gamma) - 0.040849 * std::sin(2.0 * gamma));
  const double declination =
      0.006918 - 0.399912 * std::cos(gamma) + 0.070257 * std::sin(gamma) -
      0.006758 * std::cos(2.0 * gamma) + 0.000907 * std::sin(2.0 * gamma) -
      0.002697 * std::cos(3.0 * gamma) + 0.00148 * std::sin(3.0 * gamma);

  const double noon = 720.0 - 4.0 * longitude - eqTime + static_cast<double>(utcOffsetMinutes);
  const double fajrH = hourAngleDegrees(latitude, declination, -fajrAngle);
  const double sunsetH = hourAngleDegrees(latitude, declination, -0.833);
  const double ishaH = hourAngleDegrees(latitude, declination, -ishaAngle);

  const double latitudeRad = degToRad(latitude);
  const double asrAltitude =
      radToDeg(std::atan(1.0 / (asrShadowFactor + std::tan(std::abs(latitudeRad - declination)))));
  const double asrH = hourAngleDegrees(latitude, declination, asrAltitude);

  PrayerTimes result;
  result.fajr = static_cast<int>(std::lround(noon - fajrH * 4.0));
  result.dhuhr = static_cast<int>(std::lround(noon + 1.0));
  result.asr = static_cast<int>(std::lround(noon + asrH * 4.0));
  result.maghrib = static_cast<int>(std::lround(noon + sunsetH * 4.0));
  result.isha = static_cast<int>(std::lround(noon + ishaH * 4.0));
  return result;
}

std::string formatClockMinutes(int minutes) {
  minutes = ((minutes % 1440) + 1440) % 1440;
  char buffer[8];
  std::snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes / 60, minutes % 60);
  return buffer;
}

class ExpressionParser {
 public:
  explicit ExpressionParser(const char* input) : cursor(input) {}
  bool evaluate(double& output) {
    valid = true;
    output = expression();
    skipSpaces();
    return valid && *cursor == '\0' && std::isfinite(output);
  }

 private:
  const char* cursor;
  bool valid = true;

  void skipSpaces() {
    while (*cursor == ' ' || *cursor == '\t') ++cursor;
  }
  double expression() {
    double value = term();
    while (valid) {
      skipSpaces();
      if (*cursor == '+') {
        ++cursor;
        value += term();
      } else if (*cursor == '-') {
        ++cursor;
        value -= term();
      } else {
        break;
      }
    }
    return value;
  }
  double term() {
    double value = factor();
    while (valid) {
      skipSpaces();
      if (*cursor == '*') {
        ++cursor;
        value *= factor();
      } else if (*cursor == '/') {
        ++cursor;
        const double divisor = factor();
        if (std::abs(divisor) < 1e-12) {
          valid = false;
          return 0.0;
        }
        value /= divisor;
      } else {
        break;
      }
    }
    return value;
  }
  double factor() {
    skipSpaces();
    if (*cursor == '+') {
      ++cursor;
      return factor();
    }
    if (*cursor == '-') {
      ++cursor;
      return -factor();
    }
    if (*cursor == '(') {
      ++cursor;
      const double value = expression();
      skipSpaces();
      if (*cursor != ')') {
        valid = false;
        return 0.0;
      }
      ++cursor;
      return value;
    }
    char* end = nullptr;
    const double value = std::strtod(cursor, &end);
    if (end == cursor) {
      valid = false;
      return 0.0;
    }
    cursor = end;
    return value;
  }
};

bool evaluateExpression(const std::string& expression, std::string& formatted) {
  double result = 0.0;
  ExpressionParser parser(expression.c_str());
  if (!parser.evaluate(result)) return false;
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.8g", result);
  formatted = buffer;
  return true;
}
}  // namespace

X4PlusListActivity::X4PlusListActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const Mode mode)
    : UiListActivity("X4Plus", renderer, mappedInput, true), mode(mode) {}

void X4PlusListActivity::onEnter() {
  UiListActivity::onEnter();
  load();
  rebuildRows();
}

const char* X4PlusListActivity::filePath() const {
  switch (mode) {
    case Mode::Calendar: return "/.crosspoint/x4plus-calendar.json";
    case Mode::Tasks: return "/.crosspoint/x4plus-tasks.json";
    case Mode::Notes: return "/.crosspoint/x4plus-notes.json";
    case Mode::Cards: return "/.crosspoint/x4plus-cards.json";
    case Mode::Study: return "/.crosspoint/x4plus-study.json";
    case Mode::Quran: return "/.crosspoint/x4plus-quran.json";
    case Mode::Clippings: return "/.crosspoint/x4plus-clippings.json";
    case Mode::Prayer: return "/.crosspoint/x4plus-prayer.json";
    case Mode::Focus: return "/.crosspoint/x4plus-focus.json";
    case Mode::Wallet: return "/.crosspoint/x4plus-wallet.json";
    case Mode::Calculator: return "/.crosspoint/x4plus-calculator.json";
  }
  return "/.crosspoint/x4plus.json";
}

const char* X4PlusListActivity::headerTitle() const {
  switch (mode) {
    case Mode::Calendar: return tr(STR_X4P_CALENDAR);
    case Mode::Tasks: return tr(STR_X4P_TASKS);
    case Mode::Notes: return tr(STR_X4P_NOTES);
    case Mode::Cards: return tr(STR_X4P_CARDS);
    case Mode::Study: return "Study Cards";
    case Mode::Quran: return "Qur'an";
    case Mode::Clippings: return "Clippings";
    case Mode::Prayer: return "Prayer";
    case Mode::Focus: return "Focus / Pomodoro";
    case Mode::Wallet: return "QR Wallet";
    case Mode::Calculator: return "Calculator";
  }
  return tr(STR_CROSSPOINT);
}

const char* X4PlusListActivity::addLabel() const {
  switch (mode) {
    case Mode::Calendar: return tr(STR_X4P_ADD_EVENT);
    case Mode::Tasks: return tr(STR_X4P_ADD_TASK);
    case Mode::Notes: return tr(STR_X4P_ADD_NOTE);
    case Mode::Cards: return tr(STR_X4P_ADD_CARD);
    case Mode::Study: return "Add card: Question :: Answer";
    case Mode::Quran: return "Add ayah / note";
    case Mode::Clippings: return "Add clipping";
    case Mode::Prayer: return "Edit prayer configuration";
    case Mode::Focus: return "Focus controls";
    case Mode::Wallet: return "Add QR / pass payload";
    case Mode::Calculator: return "Enter calculation";
  }
  return tr(STR_X4P_ADD_NOTE);
}

void X4PlusListActivity::load() {
  items.clear();
  completed.clear();
  revealed.clear();
  JsonDocument doc;
  if (!PersistableStoreBase::readDocFromFile(filePath(), doc)) {
    if (mode == Mode::Prayer || mode == Mode::Focus) save();
    return;
  }

  if (mode == Mode::Focus) focusSessions = doc["focusSessions"] | 0u;
  if (mode == Mode::Prayer) {
    prayerLatitude = doc["latitude"] | prayerLatitude;
    prayerLongitude = doc["longitude"] | prayerLongitude;
    prayerUtcOffsetMinutes = doc["utcOffsetMinutes"] | prayerUtcOffsetMinutes;
    prayerFajrAngle = doc["fajrAngle"] | prayerFajrAngle;
    prayerIshaAngle = doc["ishaAngle"] | prayerIshaAngle;
    prayerAsrShadowFactor = doc["asrShadowFactor"] | prayerAsrShadowFactor;
  }

  JsonArrayConst values = doc["items"].as<JsonArrayConst>();
  items.reserve(values.size());
  completed.reserve(values.size());
  for (JsonVariantConst value : values) {
    const char* text = value["text"] | "";
    if (!text[0]) continue;
    items.emplace_back(text);
    completed.push_back(value["done"] | false);
    revealed.push_back(false);
  }
}

void X4PlusListActivity::save() const {
  JsonDocument doc;
  if (mode == Mode::Focus) doc["focusSessions"] = focusSessions;
  if (mode == Mode::Prayer) {
    doc["latitude"] = prayerLatitude;
    doc["longitude"] = prayerLongitude;
    doc["utcOffsetMinutes"] = prayerUtcOffsetMinutes;
    doc["fajrAngle"] = prayerFajrAngle;
    doc["ishaAngle"] = prayerIshaAngle;
    doc["asrShadowFactor"] = prayerAsrShadowFactor;
  }

  JsonArray values = doc["items"].to<JsonArray>();
  for (size_t i = 0; i < items.size(); ++i) {
    JsonObject value = values.add<JsonObject>();
    value["text"] = items[i];
    value["done"] = mode == Mode::Tasks && i < completed.size() ? completed[i] : false;
  }
  if (!PersistableStoreBase::writeDocToFile(filePath(), doc)) LOG_ERR("X4P", "Failed to save %s", filePath());
}

uint32_t X4PlusListActivity::focusRemainingSeconds() const {
  if (!focusRunning) return focusPausedSeconds;
  const uint32_t now = millis();
  if (static_cast<int32_t>(focusEndMs - now) <= 0) return 0;
  return (focusEndMs - now + 999u) / 1000u;
}

void X4PlusListActivity::startFocus(const uint32_t seconds, const bool isBreak) {
  focusBreak = isBreak;
  focusPausedSeconds = std::max<uint32_t>(1, seconds);
  focusEndMs = millis() + focusPausedSeconds * 1000u;
  focusRunning = true;
  focusLastBucket = UINT32_MAX;
  rebuildRows();
  requestUpdate();
}

void X4PlusListActivity::pauseFocus() {
  if (!focusRunning) return;
  focusPausedSeconds = focusRemainingSeconds();
  focusRunning = false;
  rebuildRows();
  requestUpdate();
}

bool X4PlusListActivity::handleCustomInput() {
  if (mode != Mode::Focus || !focusRunning) return false;

  const uint32_t remaining = focusRemainingSeconds();
  if (remaining == 0) {
    const bool completedBreak = focusBreak;
    focusRunning = false;
    if (!completedBreak) ++focusSessions;
    focusBreak = !completedBreak;
    focusPausedSeconds = focusBreak ? 5u * 60u : 25u * 60u;
    focusLastBucket = UINT32_MAX;
    save();
    rebuildRows();
    requestUpdate();
    return false;
  }

  // One redraw per minute is deliberate on e-ink; controls still react immediately.
  const uint32_t bucket = remaining / 60u;
  if (bucket != focusLastBucket) {
    focusLastBucket = bucket;
    rebuildRows();
    requestUpdate();
  }
  return false;
}

void X4PlusListActivity::rebuildRows() {
  rows.clear();
  displayLabels.clear();
  displayLabels.reserve(std::max<size_t>(items.size() + 1, 16));

  if (mode == Mode::Focus) {
    const uint32_t remaining = focusRemainingSeconds();
    char timer[48];
    std::snprintf(timer, sizeof(timer), "%s %02lu:%02lu", focusBreak ? "Break" : "Focus",
                  static_cast<unsigned long>(remaining / 60u), static_cast<unsigned long>(remaining % 60u));
    displayLabels.emplace_back(timer);
    displayLabels.emplace_back(focusRunning ? "Pause" : "Start / Resume");
    displayLabels.emplace_back("Start 25 min Focus");
    displayLabels.emplace_back("Start 5 min Break");
    displayLabels.emplace_back("Reset to 25:00");
    displayLabels.emplace_back("Completed focus sessions: " + std::to_string(focusSessions));
  } else if (mode == Mode::Prayer) {
    int year = 2026, month = 1, day = 1;
    const time_t now = time(nullptr);
    if (now > 1577836800) {
      const time_t localEpoch = now + static_cast<time_t>(prayerUtcOffsetMinutes) * 60;
      struct tm localTime {};
      gmtime_r(&localEpoch, &localTime);
      year = localTime.tm_year + 1900;
      month = localTime.tm_mon + 1;
      day = localTime.tm_mday;
    } else {
      uint16_t rtcYear = 0;
      uint8_t rtcMonth = 0, rtcDay = 0, rtcHour = 0, rtcMinute = 0;
      if (halClock.getDateTime(rtcYear, rtcMonth, rtcDay, rtcHour, rtcMinute)) {
        year = rtcYear;
        month = rtcMonth;
        day = rtcDay;
      }
    }

    const PrayerTimes times = calculatePrayerTimes(year, month, day, prayerLatitude, prayerLongitude,
                                                   prayerUtcOffsetMinutes, prayerFajrAngle, prayerIshaAngle,
                                                   prayerAsrShadowFactor);
    char location[72];
    std::snprintf(location, sizeof(location), "Location %.3f, %.3f  UTC%+.1f", prayerLatitude, prayerLongitude,
                  static_cast<double>(prayerUtcOffsetMinutes) / 60.0);
    displayLabels.emplace_back(location);
    displayLabels.emplace_back("Fajr      " + formatClockMinutes(times.fajr));
    displayLabels.emplace_back("Dhuhr     " + formatClockMinutes(times.dhuhr));
    displayLabels.emplace_back("Asr       " + formatClockMinutes(times.asr) +
                               (prayerAsrShadowFactor >= 1.5 ? "  (2x)" : "  (1x)"));
    displayLabels.emplace_back("Maghrib   " + formatClockMinutes(times.maghrib));
    displayLabels.emplace_back("Isha      " + formatClockMinutes(times.isha));
    displayLabels.emplace_back("Edit location / calculation method");
  } else {
    if (revealed.size() < items.size()) revealed.resize(items.size(), false);
    for (size_t i = 0; i < items.size(); ++i) {
      if (mode == Mode::Tasks) {
        displayLabels.emplace_back((i < completed.size() && completed[i] ? "[x] " : "[ ] ") + items[i]);
      } else if (mode == Mode::Study) {
        const size_t split = items[i].find("::");
        const std::string front = split == std::string::npos ? items[i] : items[i].substr(0, split);
        const std::string back = split == std::string::npos ? std::string{} : items[i].substr(split + 2);
        if (i < revealed.size() && revealed[i] && !back.empty())
          displayLabels.emplace_back("A: " + back);
        else
          displayLabels.emplace_back("Q: " + front);
      } else {
        displayLabels.push_back(items[i]);
      }
    }
    displayLabels.emplace_back(addLabel());
  }

  rows.reserve(displayLabels.size());
  for (size_t i = 0; i < displayLabels.size(); ++i) {
    fui::ListItem row;
    row.label = displayLabels[i].c_str();
    row.actionValue = static_cast<int16_t>(i);
    rows.push_back(row);
  }
}

int X4PlusListActivity::listCount() const { return static_cast<int>(rows.size()); }

void X4PlusListActivity::buildScreen(UiScreen& screen) {
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
  props.inputMask = fui::InputTouch | fui::InputLongPress;
  syncListViewport(screen, props);
  screen.list(props);
}

void X4PlusListActivity::openPrayerConfigEditor() {
  char initial[96];
  std::snprintf(initial, sizeof(initial), "%.4f,%.4f,%d,%.1f,%.1f,%.0f", prayerLatitude, prayerLongitude,
                prayerUtcOffsetMinutes, prayerFajrAngle, prayerIshaAngle, prayerAsrShadowFactor);
  auto editor = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput,
                                                          "lat,lon,UTCmin,FajrAngle,IshaAngle,AsrShadow(1/2)", initial, 96,
                                                          InputType::Text);
  if (!editor) {
    LOG_ERR("X4P", "OOM: prayer config editor");
    return;
  }
  startActivityForResult(std::move(editor), [this](const ActivityResult& result) {
    if (result.isCancelled || !std::holds_alternative<KeyboardResult>(result.data)) return;
    const std::string text = std::get<KeyboardResult>(result.data).text;
    double lat = 0.0, lon = 0.0, fajr = 0.0, isha = 0.0, asrShadow = 0.0;
    int utc = 0;
    if (std::sscanf(text.c_str(), "%lf,%lf,%d,%lf,%lf,%lf", &lat, &lon, &utc, &fajr, &isha, &asrShadow) != 6)
      return;
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0 || utc < -720 || utc > 840 ||
        fajr < 10.0 || fajr > 25.0 || isha < 10.0 || isha > 25.0 ||
        (std::abs(asrShadow - 1.0) > 0.01 && std::abs(asrShadow - 2.0) > 0.01))
      return;
    prayerLatitude = lat;
    prayerLongitude = lon;
    prayerUtcOffsetMinutes = utc;
    prayerFajrAngle = fajr;
    prayerIshaAngle = isha;
    prayerAsrShadowFactor = asrShadow;
    save();
    rebuildRows();
    requestUpdate();
  });
}

void X4PlusListActivity::openEditor(const int index) {
  const bool creating = index >= static_cast<int>(items.size());
  std::string initial = creating ? std::string{} : items[index];
  if (mode == Mode::Calculator && !creating) {
    const size_t equals = initial.find(" = ");
    if (equals != std::string::npos) initial.resize(equals);
  }

  auto editor = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput, addLabel(), initial, 512, InputType::Text);
  if (!editor) {
    LOG_ERR("X4P", "OOM: keyboard editor");
    return;
  }
  startActivityForResult(std::move(editor), [this, index, creating](const ActivityResult& result) {
    if (result.isCancelled || !std::holds_alternative<KeyboardResult>(result.data)) return;
    std::string text = std::get<KeyboardResult>(result.data).text;
    if (text.empty()) {
      if (!creating && mode == Mode::Study && index >= 0 && index < static_cast<int>(items.size())) {
        items.erase(items.begin() + index);
        if (index < static_cast<int>(completed.size())) completed.erase(completed.begin() + index);
        if (index < static_cast<int>(revealed.size())) revealed.erase(revealed.begin() + index);
        save();
        rebuildRows();
        requestUpdate();
      }
      return;
    }

    if (mode == Mode::Calculator) {
      std::string value;
      const std::string expression = text;
      text = expression + " = " + (evaluateExpression(expression, value) ? value : "Error");
    }

    if (creating) {
      items.push_back(std::move(text));
      completed.push_back(false);
      revealed.push_back(false);
    } else if (index >= 0 && index < static_cast<int>(items.size())) {
      items[index] = std::move(text);
    }
    save();
    rebuildRows();
    requestUpdate();
  });
}

void X4PlusListActivity::createStudyCardFromClipping(const int index) {
  if (index < 0 || index >= static_cast<int>(items.size())) return;
  static constexpr const char* kStudyPath = "/.crosspoint/x4plus-study.json";
  JsonDocument doc;
  PersistableStoreBase::readDocFromFile(kStudyPath, doc);
  JsonArray values;
  if (doc["items"].is<JsonArray>())
    values = doc["items"].as<JsonArray>();
  else
    values = doc["items"].to<JsonArray>();
  JsonObject value = values.add<JsonObject>();
  value["text"] = items[index] + " :: ";
  value["done"] = false;
  if (!PersistableStoreBase::writeDocToFile(kStudyPath, doc)) {
    LOG_ERR("X4P", "Failed to create Study Card from clipping");
    return;
  }
  activityManager.goToX4PlusStudy();
}

void X4PlusListActivity::activateIndex(const int index) {
  app.clearTapFlash();

  if (mode == Mode::Focus) {
    switch (index) {
      case 0: break;
      case 1:
        if (focusRunning)
          pauseFocus();
        else
          startFocus(focusPausedSeconds, focusBreak);
        break;
      case 2: startFocus(25u * 60u, false); break;
      case 3: startFocus(5u * 60u, true); break;
      case 4:
        focusRunning = false;
        focusBreak = false;
        focusPausedSeconds = 25u * 60u;
        rebuildRows();
        requestUpdate();
        break;
      default: break;
    }
    return;
  }

  if (mode == Mode::Prayer) {
    if (index == static_cast<int>(rows.size()) - 1) openPrayerConfigEditor();
    return;
  }

  if (index == static_cast<int>(items.size())) {
    openEditor(index);
    return;
  }
  if (index < 0 || index >= static_cast<int>(items.size())) return;

  if (mode == Mode::Tasks) {
    completed[index] = !completed[index];
    save();
    rebuildRows();
    requestUpdate();
    return;
  }
  if (mode == Mode::Study) {
    if (revealed.size() < items.size()) revealed.resize(items.size(), false);
    revealed[index] = !revealed[index];
    rebuildRows();
    requestUpdate();
    return;
  }
  if (mode == Mode::Clippings) {
    createStudyCardFromClipping(index);
    return;
  }
  if (mode == Mode::Wallet) {
    auto qr = makeUniqueNoThrow<QrDisplayActivity>(renderer, mappedInput, items[index]);
    if (!qr) {
      LOG_ERR("X4P", "OOM: QR wallet display");
      return;
    }
    activityManager.pushActivity(std::move(qr));
    return;
  }
  openEditor(index);
}

void X4PlusListActivity::onRowLongPress(const int index) {
  if (mode == Mode::Focus || mode == Mode::Prayer) return;
  if (index < 0 || index >= static_cast<int>(items.size())) return;
  if (mode == Mode::Study) {
    openEditor(index);
    return;
  }
  items.erase(items.begin() + index);
  if (index < static_cast<int>(completed.size())) completed.erase(completed.begin() + index);
  if (index < static_cast<int>(revealed.size())) revealed.erase(revealed.begin() + index);
  save();
  rebuildRows();
  nav.selected = std::min(index, static_cast<int>(rows.size()) - 1);
  requestUpdate();
}
