#include "X4PlusServiceActivity.h"

#include <ArduinoJson.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <Memory.h>
#include <PersistableStore.h>
#include <SecureHttpClient.h>
#include <WiFi.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "activities/ActivityManager.h"
#include "activities/ActivityResult.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

namespace {
constexpr const char* kServicesPath = "/.crosspoint/x4plus-services.json";
constexpr const char* kPrayerPath = "/.crosspoint/x4plus-prayer.json";
constexpr double kPi = 3.14159265358979323846;

struct PrayerTimes {
  int fajr = 0;
  int sunrise = 0;
  int dhuhr = 0;
  int asr = 0;
  int maghrib = 0;
  int isha = 0;
};

struct PrayerMethod {
  const char* name;
  double fajrAngle;
  double ishaAngle;
  int ishaIntervalMinutes;
};

constexpr PrayerMethod kPrayerMethods[] = {
    {"Muslim World League", 18.0, 17.0, 0},
    {"ISNA", 15.0, 15.0, 0},
    {"Egyptian", 19.5, 17.5, 0},
    {"Karachi", 18.0, 18.0, 0},
    {"Umm al-Qura", 18.5, 0.0, 90},
    {"Custom", 18.0, 17.0, 0},
};
constexpr int kPrayerMethodCount = sizeof(kPrayerMethods) / sizeof(kPrayerMethods[0]);

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

PrayerTimes calculatePrayerTimes(const int year, const int month, const int day, const double latitude,
                                 const double longitude, const int utcOffsetMinutes, const PrayerMethod& method,
                                 const double customFajr, const double customIsha, const bool hanafi,
                                 const std::array<int, 6>& offsets) {
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

  const double fajrAngle = method.name == kPrayerMethods[5].name ? customFajr : method.fajrAngle;
  const double ishaAngle = method.name == kPrayerMethods[5].name ? customIsha : method.ishaAngle;
  const double fajrH = hourAngleDegrees(latitude, declination, -fajrAngle);
  const double sunsetH = hourAngleDegrees(latitude, declination, -0.833);
  const double asrShadow = hanafi ? 2.0 : 1.0;
  const double latitudeRad = degToRad(latitude);
  const double asrAltitude =
      radToDeg(std::atan(1.0 / (asrShadow + std::tan(std::abs(latitudeRad - declination)))));
  const double asrH = hourAngleDegrees(latitude, declination, asrAltitude);

  PrayerTimes result;
  result.fajr = static_cast<int>(std::lround(noon - fajrH * 4.0)) + offsets[0];
  result.sunrise = static_cast<int>(std::lround(noon - sunsetH * 4.0)) + offsets[1];
  result.dhuhr = static_cast<int>(std::lround(noon + 1.0)) + offsets[2];
  result.asr = static_cast<int>(std::lround(noon + asrH * 4.0)) + offsets[3];
  result.maghrib = static_cast<int>(std::lround(noon + sunsetH * 4.0)) + offsets[4];
  if (method.ishaIntervalMinutes > 0)
    result.isha = result.maghrib + method.ishaIntervalMinutes + offsets[5];
  else
    result.isha = static_cast<int>(std::lround(noon + hourAngleDegrees(latitude, declination, -ishaAngle) * 4.0)) +
                  offsets[5];
  return result;
}

std::string formatClock(int minutes, const bool clock24h) {
  minutes = ((minutes % 1440) + 1440) % 1440;
  char buffer[16];
  if (clock24h) {
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes / 60, minutes % 60);
  } else {
    const int hour24 = minutes / 60;
    int hour = hour24 % 12;
    if (hour == 0) hour = 12;
    std::snprintf(buffer, sizeof(buffer), "%d:%02d %s", hour, minutes % 60, hour24 < 12 ? "AM" : "PM");
  }
  return buffer;
}

bool currentDateAndMinutes(const int utcOffsetMinutes, int& year, int& month, int& day, int& minuteOfDay) {
  const time_t now = time(nullptr);
  if (now > 1577836800) {
    const time_t localEpoch = now + static_cast<time_t>(utcOffsetMinutes) * 60;
    struct tm localTime {};
    gmtime_r(&localEpoch, &localTime);
    year = localTime.tm_year + 1900;
    month = localTime.tm_mon + 1;
    day = localTime.tm_mday;
    minuteOfDay = localTime.tm_hour * 60 + localTime.tm_min;
    return true;
  }
  uint16_t rtcYear = 0;
  uint8_t rtcMonth = 0, rtcDay = 0, rtcHour = 0, rtcMinute = 0;
  if (!halClock.getDateTime(rtcYear, rtcMonth, rtcDay, rtcHour, rtcMinute)) return false;
  year = rtcYear;
  month = rtcMonth;
  day = rtcDay;
  minuteOfDay = rtcHour * 60 + rtcMinute;
  return true;
}

std::string nextPrayerLine(const PrayerTimes& times, int nowMinutes, const bool clock24h) {
  const struct Entry {
    const char* name;
    int time;
  } entries[] = {{"Fajr", times.fajr},       {"Sunrise", times.sunrise}, {"Dhuhr", times.dhuhr},
                 {"Asr", times.asr},         {"Maghrib", times.maghrib}, {"Isha", times.isha}};
  for (const auto& entry : entries) {
    const int normalized = ((entry.time % 1440) + 1440) % 1440;
    if (normalized > nowMinutes) return std::string(entry.name) + "  " + formatClock(normalized, clock24h);
  }
  return std::string("Fajr tomorrow  ") + formatClock(times.fajr, clock24h);
}

std::string jsonString(JsonVariantConst value, const char* fallback = "") {
  const char* text = value | fallback;
  return text ? std::string(text) : std::string(fallback);
}

std::string trimXml(std::string value) {
  const auto begin = value.find_first_not_of(" \t\r\n");
  const auto end = value.find_last_not_of(" \t\r\n");
  if (begin == std::string::npos) return {};
  value = value.substr(begin, end - begin + 1);
  const struct Entity {
    const char* from;
    const char* to;
  } entities[] = {{"&amp;", "&"}, {"&quot;", "\""}, {"&#39;", "'"}, {"&lt;", "<"}, {"&gt;", ">"}};
  for (const auto& entity : entities) {
    size_t pos = 0;
    while ((pos = value.find(entity.from, pos)) != std::string::npos) {
      value.replace(pos, std::strlen(entity.from), entity.to);
      pos += std::strlen(entity.to);
    }
  }
  if (value.size() > 96) value.resize(93), value += "...";
  return value;
}
}  // namespace

X4PlusServiceActivity::X4PlusServiceActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const Mode mode)
    : UiListActivity("X4Services", renderer, mappedInput), mode(mode) {}

void X4PlusServiceActivity::onEnter() {
  UiListActivity::onEnter();
  loadConfig();
  rebuildRows();
}

const char* X4PlusServiceActivity::headerTitle() const {
  switch (mode) {
    case Mode::DailyBrief: return "Daily Brief";
    case Mode::Prayer: return "Salaah";
    case Mode::PrayerSettings: return "Salaah Settings";
    case Mode::Ramadan: return "Ramadan";
    case Mode::Ai: return "INK AI";
    case Mode::Connections: return "Connected Services";
    case Mode::Maps: return "Maps / Trip Packs";
    case Mode::News: return "News Brief";
  }
  return "X4 Pro+";
}

void X4PlusServiceActivity::loadConfig() {
  config = Config{};
  JsonDocument services;
  if (PersistableStoreBase::readDocFromFile(kServicesPath, services)) {
    config.notionWebhook = jsonString(services["notionWebhook"]);
    config.calendarIcs = jsonString(services["calendarIcs"]);
    config.rssFeed = jsonString(services["rssFeed"]);
    config.aiEndpoint = jsonString(services["aiEndpoint"]);
    config.aiModel = jsonString(services["aiModel"]);
    config.aiApiKey = jsonString(services["aiApiKey"]);
    config.mapProvider = jsonString(services["mapProvider"], "Offline / OSM");
    config.weatherSummary = jsonString(services["weatherSummary"]);
    config.weatherUpdated = jsonString(services["weatherUpdated"]);
    config.aiLastResponse = jsonString(services["aiLastResponse"]);
    config.notionStatus = jsonString(services["notionStatus"], "Not configured");
    JsonArrayConst headlines = services["newsHeadlines"].as<JsonArrayConst>();
    for (JsonVariantConst item : headlines) config.newsHeadlines.push_back(jsonString(item));
    JsonArrayConst places = services["savedPlaces"].as<JsonArrayConst>();
    for (JsonVariantConst item : places) config.savedPlaces.push_back(jsonString(item));
  }

  JsonDocument prayer;
  if (PersistableStoreBase::readDocFromFile(kPrayerPath, prayer)) {
    config.latitude = prayer["latitude"] | 0.0;
    config.longitude = prayer["longitude"] | 0.0;
    config.utcOffsetMinutes = prayer["utcOffsetMinutes"] | 0;
    config.prayerMethod = std::clamp(prayer["method"] | 0, 0, kPrayerMethodCount - 1);
    config.hanafiAsr = prayer["hanafiAsr"] | ((prayer["asrShadowFactor"] | 1.0) >= 1.5);
    config.clock24h = prayer["clock24h"] | true;
    config.customFajrAngle = prayer["customFajrAngle"] | (prayer["fajrAngle"] | 18.0);
    config.customIshaAngle = prayer["customIshaAngle"] | (prayer["ishaAngle"] | 17.0);
    config.prayerConfigured = prayer["configured"] | (std::abs(config.latitude) > 0.001 || std::abs(config.longitude) > 0.001);
    JsonArrayConst offsets = prayer["offsets"].as<JsonArrayConst>();
    for (size_t i = 0; i < config.offsets.size() && i < offsets.size(); ++i) config.offsets[i] = offsets[i] | 0;
  }
}

void X4PlusServiceActivity::saveConfig() const {
  JsonDocument services;
  services["notionWebhook"] = config.notionWebhook;
  services["calendarIcs"] = config.calendarIcs;
  services["rssFeed"] = config.rssFeed;
  services["aiEndpoint"] = config.aiEndpoint;
  services["aiModel"] = config.aiModel;
  services["aiApiKey"] = config.aiApiKey;
  services["mapProvider"] = config.mapProvider;
  services["weatherSummary"] = config.weatherSummary;
  services["weatherUpdated"] = config.weatherUpdated;
  services["aiLastResponse"] = config.aiLastResponse;
  services["notionStatus"] = config.notionStatus;
  JsonArray headlines = services["newsHeadlines"].to<JsonArray>();
  for (const auto& item : config.newsHeadlines) headlines.add(item);
  JsonArray places = services["savedPlaces"].to<JsonArray>();
  for (const auto& item : config.savedPlaces) places.add(item);
  PersistableStoreBase::writeDocToFile(kServicesPath, services);

  JsonDocument prayer;
  PersistableStoreBase::readDocFromFile(kPrayerPath, prayer);
  prayer["latitude"] = config.latitude;
  prayer["longitude"] = config.longitude;
  prayer["utcOffsetMinutes"] = config.utcOffsetMinutes;
  prayer["method"] = config.prayerMethod;
  prayer["hanafiAsr"] = config.hanafiAsr;
  prayer["clock24h"] = config.clock24h;
  prayer["customFajrAngle"] = config.customFajrAngle;
  prayer["customIshaAngle"] = config.customIshaAngle;
  prayer["configured"] = config.prayerConfigured;
  const PrayerMethod& method = kPrayerMethods[config.prayerMethod];
  prayer["fajrAngle"] = config.prayerMethod == kPrayerMethodCount - 1 ? config.customFajrAngle : method.fajrAngle;
  prayer["ishaAngle"] = config.prayerMethod == kPrayerMethodCount - 1 ? config.customIshaAngle : method.ishaAngle;
  prayer["asrShadowFactor"] = config.hanafiAsr ? 2.0 : 1.0;
  JsonArray offsets = prayer["offsets"].to<JsonArray>();
  for (const int offset : config.offsets) offsets.add(offset);
  PersistableStoreBase::writeDocToFile(kPrayerPath, prayer);
}

void X4PlusServiceActivity::rebuildListItems() {
  rows.clear();
  rows.reserve(labels.size());
  for (size_t i = 0; i < labels.size(); ++i) {
    fui::ListItem row;
    row.label = labels[i].c_str();
    row.actionValue = static_cast<int16_t>(i);
    rows.push_back(row);
  }
}

void X4PlusServiceActivity::rebuildRows() {
  labels.clear();
  int year = 2026, month = 1, day = 1, nowMinutes = 0;
  currentDateAndMinutes(config.utcOffsetMinutes, year, month, day, nowMinutes);
  const PrayerMethod& method = kPrayerMethods[std::clamp(config.prayerMethod, 0, kPrayerMethodCount - 1)];
  const PrayerTimes times = calculatePrayerTimes(year, month, day, config.latitude, config.longitude,
                                                 config.utcOffsetMinutes, method, config.customFajrAngle,
                                                 config.customIshaAngle, config.hanafiAsr, config.offsets);

  switch (mode) {
    case Mode::DailyBrief:
      labels.emplace_back(config.weatherSummary.empty() ? "Weather: tap Sync now" : "Weather: " + config.weatherSummary);
      labels.emplace_back(config.prayerConfigured ? "Next Salaah: " + nextPrayerLine(times, nowMinutes, config.clock24h)
                                                  : "Salaah: configure location first");
      labels.emplace_back(config.calendarIcs.empty() ? "Calendar: not linked" : "Calendar: ICS linked");
      labels.emplace_back("Tasks: local planner ready");
      labels.emplace_back("Notion: " + config.notionStatus);
      labels.emplace_back("Sync weather now");
      labels.emplace_back("Connected services settings");
      break;
    case Mode::Prayer:
      if (!config.prayerConfigured) {
        labels.emplace_back("Location is not configured");
      } else {
        labels.emplace_back(std::string("Method: ") + method.name + (config.hanafiAsr ? " | Hanafi Asr" : " | Standard Asr"));
        labels.emplace_back("Fajr       " + formatClock(times.fajr, config.clock24h));
        labels.emplace_back("Sunrise    " + formatClock(times.sunrise, config.clock24h));
        labels.emplace_back("Dhuhr      " + formatClock(times.dhuhr, config.clock24h));
        labels.emplace_back("Asr        " + formatClock(times.asr, config.clock24h));
        labels.emplace_back("Maghrib    " + formatClock(times.maghrib, config.clock24h));
        labels.emplace_back("Isha       " + formatClock(times.isha, config.clock24h));
        labels.emplace_back("Next: " + nextPrayerLine(times, nowMinutes, config.clock24h));
      }
      labels.emplace_back("Salaah settings");
      labels.emplace_back("Ramadan 7-day view");
      break;
    case Mode::PrayerSettings: {
      labels.emplace_back(std::string("Calculation method: ") + method.name);
      labels.emplace_back(std::string("Asr: ") + (config.hanafiAsr ? "Hanafi (2x shadow)" : "Standard (1x shadow)"));
      char location[96];
      std::snprintf(location, sizeof(location), "Location: %.4f, %.4f | UTC%+.1f", config.latitude, config.longitude,
                    static_cast<double>(config.utcOffsetMinutes) / 60.0);
      labels.emplace_back(location);
      char offsets[96];
      std::snprintf(offsets, sizeof(offsets), "Offsets F/S/D/A/M/I: %d,%d,%d,%d,%d,%d", config.offsets[0],
                    config.offsets[1], config.offsets[2], config.offsets[3], config.offsets[4], config.offsets[5]);
      labels.emplace_back(offsets);
      labels.emplace_back(std::string("Time format: ") + (config.clock24h ? "24-hour" : "12-hour"));
      if (config.prayerMethod == kPrayerMethodCount - 1) {
        char custom[72];
        std::snprintf(custom, sizeof(custom), "Custom angles: Fajr %.1f / Isha %.1f", config.customFajrAngle,
                      config.customIshaAngle);
        labels.emplace_back(custom);
      }
      labels.emplace_back("Preview Salaah times");
      break;
    }
    case Mode::Ramadan:
      labels.emplace_back("Suhoor close = Fajr | Iftar = Maghrib");
      for (int offset = 0; offset < 7; ++offset) {
        const time_t now = time(nullptr);
        int y = year, m = month, d = day;
        if (now > 1577836800) {
          const time_t local = now + static_cast<time_t>(config.utcOffsetMinutes) * 60 + offset * 86400;
          struct tm value {};
          gmtime_r(&local, &value);
          y = value.tm_year + 1900;
          m = value.tm_mon + 1;
          d = value.tm_mday;
        } else {
          d += offset;
        }
        const PrayerTimes daily = calculatePrayerTimes(y, m, d, config.latitude, config.longitude,
                                                       config.utcOffsetMinutes, method, config.customFajrAngle,
                                                       config.customIshaAngle, config.hanafiAsr, config.offsets);
        char row[96];
        std::snprintf(row, sizeof(row), "%s | Suhoor %s | Iftar %s", offset == 0 ? "Today" : ("Day +" + std::to_string(offset)).c_str(),
                      formatClock(daily.fajr, config.clock24h).c_str(),
                      formatClock(daily.maghrib, config.clock24h).c_str());
        labels.emplace_back(row);
      }
      labels.emplace_back("Salaah settings");
      break;
    case Mode::Ai:
      labels.emplace_back(config.aiEndpoint.empty() ? "Provider: not configured" : "Provider: OpenAI-compatible HTTPS");
      labels.emplace_back(config.aiEndpoint.empty() ? "Endpoint: tap to configure" : "Endpoint: configured");
      labels.emplace_back(config.aiModel.empty() ? "Model: tap to configure" : "Model: " + config.aiModel);
      labels.emplace_back(config.aiApiKey.empty() ? "API key: not stored" : "API key: stored locally");
      labels.emplace_back("Ask INK AI (manual prompt)");
      labels.emplace_back(config.aiLastResponse.empty() ? "Last response: none" : "Last response: " + config.aiLastResponse);
      labels.emplace_back("Privacy: manual only; no background book upload");
      labels.emplace_back("Reader handoff requires selected-text mode");
      break;
    case Mode::Connections:
      labels.emplace_back("Weather: Open-Meteo");
      labels.emplace_back(config.notionWebhook.empty() ? "Notion webhook: not configured" : "Notion webhook: configured");
      labels.emplace_back(config.calendarIcs.empty() ? "Calendar ICS: not configured" : "Calendar ICS: configured");
      labels.emplace_back(config.rssFeed.empty() ? "RSS / Atom feed: not configured" : "RSS / Atom feed: configured");
      labels.emplace_back("Notion quick capture");
      labels.emplace_back("Test Notion webhook");
      labels.emplace_back("INK AI settings");
      labels.emplace_back("Phone / PC transfer");
      break;
    case Mode::Maps:
      labels.emplace_back("Map source: " + config.mapProvider);
      for (const auto& place : config.savedPlaces) labels.emplace_back("Saved: " + place);
      labels.emplace_back("Add saved place");
      labels.emplace_back("Offline trip packs use saved places / route groups");
      break;
    case Mode::News:
      labels.emplace_back(config.rssFeed.empty() ? "RSS / Atom feed: not configured" : "RSS / Atom feed: configured");
      for (const auto& headline : config.newsHeadlines) labels.emplace_back(headline);
      labels.emplace_back("Sync headlines now");
      labels.emplace_back("Configure feed URL");
      break;
  }
  rebuildListItems();
}

int X4PlusServiceActivity::listCount() const { return static_cast<int>(rows.size()); }

void X4PlusServiceActivity::buildScreen(UiScreen& screen) {
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

void X4PlusServiceActivity::editStringField(const char* title, std::string initial, const size_t maxLength,
                                            const InputType type,
                                            const std::function<void(std::string)>& onSaved) {
  auto editor = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput, title, std::move(initial), maxLength, type);
  if (!editor) return;
  startActivityForResult(std::move(editor), [this, onSaved](const ActivityResult& result) {
    if (result.isCancelled || !std::holds_alternative<KeyboardResult>(result.data)) return;
    onSaved(std::get<KeyboardResult>(result.data).text);
    saveConfig();
    rebuildRows();
    requestUpdate();
  });
}

void X4PlusServiceActivity::editLocation() {
  char initial[80];
  std::snprintf(initial, sizeof(initial), "%.5f,%.5f,%d", config.latitude, config.longitude, config.utcOffsetMinutes);
  editStringField("latitude,longitude,UTC minutes", initial, 80, InputType::Text, [this](std::string text) {
    double lat = 0.0, lon = 0.0;
    int utc = 0;
    if (std::sscanf(text.c_str(), "%lf,%lf,%d", &lat, &lon, &utc) != 3) return;
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0 || utc < -720 || utc > 840) return;
    config.latitude = lat;
    config.longitude = lon;
    config.utcOffsetMinutes = utc;
    config.prayerConfigured = true;
  });
}

void X4PlusServiceActivity::editOffsets() {
  char initial[80];
  std::snprintf(initial, sizeof(initial), "%d,%d,%d,%d,%d,%d", config.offsets[0], config.offsets[1],
                config.offsets[2], config.offsets[3], config.offsets[4], config.offsets[5]);
  editStringField("Fajr,Sunrise,Dhuhr,Asr,Maghrib,Isha offsets", initial, 80, InputType::Text,
                  [this](std::string text) {
                    std::array<int, 6> values{};
                    if (std::sscanf(text.c_str(), "%d,%d,%d,%d,%d,%d", &values[0], &values[1], &values[2],
                                    &values[3], &values[4], &values[5]) != 6)
                      return;
                    for (const int value : values)
                      if (value < -60 || value > 60) return;
                    config.offsets = values;
                  });
}

void X4PlusServiceActivity::editCustomAngles() {
  char initial[48];
  std::snprintf(initial, sizeof(initial), "%.1f,%.1f", config.customFajrAngle, config.customIshaAngle);
  editStringField("Custom Fajr angle,Isha angle", initial, 48, InputType::Text, [this](std::string text) {
    double fajr = 0.0, isha = 0.0;
    if (std::sscanf(text.c_str(), "%lf,%lf", &fajr, &isha) != 2) return;
    if (fajr < 10.0 || fajr > 25.0 || isha < 10.0 || isha > 25.0) return;
    config.customFajrAngle = fajr;
    config.customIshaAngle = isha;
  });
}

void X4PlusServiceActivity::cyclePrayerMethod() {
  config.prayerMethod = (config.prayerMethod + 1) % kPrayerMethodCount;
  saveConfig();
  rebuildRows();
  requestUpdate();
}

bool X4PlusServiceActivity::ensureNetwork(const char* action) {
  if (WiFi.status() == WL_CONNECTED) return true;
  config.notionStatus = std::string(action) + ": connect Wi-Fi first";
  rebuildRows();
  requestUpdate();
  return false;
}

void X4PlusServiceActivity::syncWeather() {
  if (!config.prayerConfigured) {
    config.weatherSummary = "Set location in Salaah Settings";
    saveConfig();
    rebuildRows();
    requestUpdate();
    return;
  }
  if (!ensureNetwork("Weather")) return;
  char url[384];
  std::snprintf(url, sizeof(url),
                "https://api.open-meteo.com/v1/forecast?latitude=%.5f&longitude=%.5f&current=temperature_2m,weather_code&daily=temperature_2m_max,temperature_2m_min&forecast_days=1&timezone=auto",
                config.latitude, config.longitude);
  freeink::SecureHttpClient http;
  http.setInsecure();
  http.setTimeout(12000);
  if (!http.begin(url)) return;
  http.addHeader("Accept", "application/json");
  const int status = http.GET();
  if (status == 200) {
    JsonDocument doc;
    if (deserializeJson(doc, http.getString()) == DeserializationError::Ok) {
      const double temp = doc["current"]["temperature_2m"] | 0.0;
      const double high = doc["daily"]["temperature_2m_max"][0] | temp;
      const double low = doc["daily"]["temperature_2m_min"][0] | temp;
      const int code = doc["current"]["weather_code"] | -1;
      const char* condition = code == 0 ? "Clear" : (code <= 3 ? "Partly cloudy" : (code <= 67 ? "Rain possible" : "Mixed weather"));
      char summary[96];
      std::snprintf(summary, sizeof(summary), "%.0f C %s | H %.0f / L %.0f", temp, condition, high, low);
      config.weatherSummary = summary;
      config.weatherUpdated = "Open-Meteo sync complete";
    }
  } else {
    config.weatherSummary = "Weather sync failed";
  }
  http.end();
  saveConfig();
  rebuildRows();
  requestUpdate();
}

void X4PlusServiceActivity::testNotion() {
  if (config.notionWebhook.empty()) {
    config.notionStatus = "Not configured";
    rebuildRows();
    requestUpdate();
    return;
  }
  if (!ensureNetwork("Notion")) return;
  JsonDocument payload;
  payload["timestamp"] = static_cast<long long>(time(nullptr));
  payload["note"] = "X4 Pro+ Notion connection test";
  payload["source"] = "x4pro-test";
  std::string body;
  serializeJson(payload, body);
  freeink::SecureHttpClient http;
  http.setInsecure();
  http.setTimeout(12000);
  if (!http.begin(config.notionWebhook)) return;
  http.addHeader("Content-Type", "application/json");
  const int status = http.POST(body);
  config.notionStatus = status >= 200 && status < 300 ? "Synced" : "Failed (HTTP " + std::to_string(status) + ")";
  http.end();
  saveConfig();
  rebuildRows();
  requestUpdate();
}

void X4PlusServiceActivity::captureToNotion() {
  editStringField("Quick Capture -> Notion", "", 512, InputType::Text, [this](std::string note) {
    if (note.empty()) return;
    if (config.notionWebhook.empty()) {
      config.notionStatus = "Configure webhook first";
      return;
    }
    if (WiFi.status() != WL_CONNECTED) {
      config.notionStatus = "Queued locally - connect Wi-Fi";
      return;
    }
    JsonDocument payload;
    payload["timestamp"] = static_cast<long long>(time(nullptr));
    payload["note"] = note;
    payload["source"] = "x4pro-quick-capture";
    std::string body;
    serializeJson(payload, body);
    freeink::SecureHttpClient http;
    http.setInsecure();
    http.setTimeout(12000);
    if (!http.begin(config.notionWebhook)) {
      config.notionStatus = "Invalid webhook URL";
      return;
    }
    http.addHeader("Content-Type", "application/json");
    const int status = http.POST(body);
    config.notionStatus = status >= 200 && status < 300 ? "Synced" : "Failed (HTTP " + std::to_string(status) + ")";
    http.end();
  });
}

void X4PlusServiceActivity::configureAiKey() {
  editStringField("INK AI API key (stored locally)", "", 256, InputType::Password,
                  [this](std::string key) { config.aiApiKey = std::move(key); });
}

void X4PlusServiceActivity::askAi() {
  editStringField("Ask INK AI", "", 768, InputType::Text, [this](std::string prompt) {
    if (prompt.empty()) return;
    if (config.aiEndpoint.empty() || config.aiModel.empty()) {
      config.aiLastResponse = "Configure endpoint and model first";
      return;
    }
    if (WiFi.status() != WL_CONNECTED) {
      config.aiLastResponse = "Connect Wi-Fi first";
      return;
    }
    JsonDocument payload;
    payload["model"] = config.aiModel;
    JsonArray messages = payload["messages"].to<JsonArray>();
    JsonObject system = messages.add<JsonObject>();
    system["role"] = "system";
    system["content"] = "You are INK AI on an e-reader. Be concise, useful, and preserve the user's reading context.";
    JsonObject user = messages.add<JsonObject>();
    user["role"] = "user";
    user["content"] = prompt;
    payload["temperature"] = 0.2;
    std::string body;
    serializeJson(payload, body);

    freeink::SecureHttpClient http;
    http.setInsecure();
    http.setTimeout(20000);
    if (!http.begin(config.aiEndpoint)) {
      config.aiLastResponse = "Invalid AI endpoint";
      return;
    }
    http.addHeader("Content-Type", "application/json");
    if (!config.aiApiKey.empty()) http.addHeader("Authorization", "Bearer " + config.aiApiKey);
    const int status = http.POST(body);
    if (status >= 200 && status < 300) {
      JsonDocument response;
      if (deserializeJson(response, http.getString()) == DeserializationError::Ok) {
        config.aiLastResponse = jsonString(response["choices"][0]["message"]["content"], "No text returned");
        if (config.aiLastResponse.size() > 360) config.aiLastResponse.resize(357), config.aiLastResponse += "...";
      } else {
        config.aiLastResponse = "AI response parse failed";
      }
    } else {
      config.aiLastResponse = "AI request failed (HTTP " + std::to_string(status) + ")";
    }
    http.end();
  });
}

void X4PlusServiceActivity::syncNews() {
  if (config.rssFeed.empty()) {
    rebuildRows();
    requestUpdate();
    return;
  }
  if (!ensureNetwork("News")) return;
  freeink::SecureHttpClient http;
  http.setInsecure();
  http.setTimeout(15000);
  if (!http.begin(config.rssFeed)) return;
  http.addHeader("Accept", "application/rss+xml, application/atom+xml, application/xml, text/xml");
  const int status = http.GET();
  if (status >= 200 && status < 300) {
    const std::string& xml = http.getString();
    config.newsHeadlines.clear();
    size_t pos = 0;
    bool skippedFeedTitle = false;
    while (config.newsHeadlines.size() < 5) {
      const size_t open = xml.find("<title", pos);
      if (open == std::string::npos) break;
      const size_t gt = xml.find('>', open);
      const size_t close = gt == std::string::npos ? std::string::npos : xml.find("</title>", gt + 1);
      if (gt == std::string::npos || close == std::string::npos) break;
      std::string title = trimXml(xml.substr(gt + 1, close - gt - 1));
      if (!title.empty()) {
        if (!skippedFeedTitle)
          skippedFeedTitle = true;
        else
          config.newsHeadlines.push_back(std::move(title));
      }
      pos = close + 8;
    }
  }
  http.end();
  saveConfig();
  rebuildRows();
  requestUpdate();
}

void X4PlusServiceActivity::addSavedPlace() {
  editStringField("Saved place / address", "", 160, InputType::Text, [this](std::string place) {
    if (!place.empty()) config.savedPlaces.push_back(std::move(place));
  });
}

void X4PlusServiceActivity::activateIndex(const int index) {
  app.clearTapFlash();
  if (index < 0 || index >= static_cast<int>(labels.size())) return;

  switch (mode) {
    case Mode::DailyBrief:
      if (index == 5) syncWeather();
      else if (index == 6) activityManager.pushActivity(makeUniqueNoThrow<X4PlusServiceActivity>(renderer, mappedInput, Mode::Connections));
      break;
    case Mode::Prayer: {
      const int settingsIndex = config.prayerConfigured ? 8 : 1;
      const int ramadanIndex = settingsIndex + 1;
      if (index == settingsIndex)
        activityManager.pushActivity(makeUniqueNoThrow<X4PlusServiceActivity>(renderer, mappedInput, Mode::PrayerSettings));
      else if (index == ramadanIndex)
        activityManager.pushActivity(makeUniqueNoThrow<X4PlusServiceActivity>(renderer, mappedInput, Mode::Ramadan));
      break;
    }
    case Mode::PrayerSettings: {
      int row = 0;
      if (index == row++) cyclePrayerMethod();
      else if (index == row++) {
        config.hanafiAsr = !config.hanafiAsr;
        saveConfig();
        rebuildRows();
        requestUpdate();
      } else if (index == row++) {
        editLocation();
      } else if (index == row++) {
        editOffsets();
      } else if (index == row++) {
        config.clock24h = !config.clock24h;
        saveConfig();
        rebuildRows();
        requestUpdate();
      } else if (config.prayerMethod == kPrayerMethodCount - 1 && index == row++) {
        editCustomAngles();
      } else if (index == row) {
        activityManager.pushActivity(makeUniqueNoThrow<X4PlusServiceActivity>(renderer, mappedInput, Mode::Prayer));
      }
      break;
    }
    case Mode::Ramadan:
      if (index == static_cast<int>(labels.size()) - 1)
        activityManager.pushActivity(makeUniqueNoThrow<X4PlusServiceActivity>(renderer, mappedInput, Mode::PrayerSettings));
      break;
    case Mode::Ai:
      if (index == 1)
        editStringField("OpenAI-compatible HTTPS endpoint", config.aiEndpoint, 256, InputType::Url,
                        [this](std::string value) { config.aiEndpoint = std::move(value); });
      else if (index == 2)
        editStringField("Model name", config.aiModel, 128, InputType::Text,
                        [this](std::string value) { config.aiModel = std::move(value); });
      else if (index == 3)
        configureAiKey();
      else if (index == 4)
        askAi();
      break;
    case Mode::Connections:
      if (index == 1)
        editStringField("Notion secure webhook URL", config.notionWebhook, 384, InputType::Url,
                        [this](std::string value) {
                          config.notionWebhook = std::move(value);
                          config.notionStatus = config.notionWebhook.empty() ? "Not configured" : "Configured";
                        });
      else if (index == 2)
        editStringField("Read-only Calendar ICS URL", config.calendarIcs, 384, InputType::Url,
                        [this](std::string value) { config.calendarIcs = std::move(value); });
      else if (index == 3)
        editStringField("RSS / Atom feed URL", config.rssFeed, 384, InputType::Url,
                        [this](std::string value) { config.rssFeed = std::move(value); });
      else if (index == 4)
        captureToNotion();
      else if (index == 5)
        testNotion();
      else if (index == 6)
        activityManager.pushActivity(makeUniqueNoThrow<X4PlusServiceActivity>(renderer, mappedInput, Mode::Ai));
      else if (index == 7)
        activityManager.goToPhoneTransfer();
      break;
    case Mode::Maps:
      if (index == static_cast<int>(labels.size()) - 2) addSavedPlace();
      break;
    case Mode::News:
      if (index == static_cast<int>(labels.size()) - 2)
        syncNews();
      else if (index == static_cast<int>(labels.size()) - 1)
        editStringField("RSS / Atom feed URL", config.rssFeed, 384, InputType::Url,
                        [this](std::string value) { config.rssFeed = std::move(value); });
      break;
  }
}
