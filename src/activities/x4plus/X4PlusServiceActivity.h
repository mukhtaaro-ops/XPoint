#pragma once

#include <array>
#include <string>
#include <vector>

#include "activities/UiListActivity.h"

class X4PlusServiceActivity final : public UiListActivity {
 public:
  enum class Mode : uint8_t {
    DailyBrief,
    Prayer,
    PrayerSettings,
    Ramadan,
    Ai,
    Connections,
    Maps,
    News
  };

  X4PlusServiceActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, Mode mode);
  void onEnter() override;

 protected:
  int listCount() const override;
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  const char* headerTitle() const override;

 private:
  struct Config {
    bool prayerConfigured = false;
    double latitude = 0.0;
    double longitude = 0.0;
    int utcOffsetMinutes = 0;
    int prayerMethod = 0;
    bool hanafiAsr = false;
    bool clock24h = true;
    std::array<int, 6> offsets{0, 0, 0, 0, 0, 0};
    double customFajrAngle = 18.0;
    double customIshaAngle = 17.0;

    std::string notionWebhook;
    std::string calendarIcs;
    std::string rssFeed;
    std::string aiEndpoint;
    std::string aiModel;
    std::string aiApiKey;
    std::string mapProvider = "Offline / OSM";

    std::string weatherSummary;
    std::string weatherUpdated;
    std::vector<std::string> newsHeadlines;
    std::vector<std::string> savedPlaces;
    std::string aiLastResponse;
    std::string notionStatus = "Not configured";
  };

  Mode mode;
  Config config;
  std::vector<std::string> labels;
  std::vector<freeink::ui::ListItem> rows;

  void loadConfig();
  void saveConfig() const;
  void rebuildRows();
  void rebuildListItems();

  void editLocation();
  void editOffsets();
  void editCustomAngles();
  void editStringField(const char* title, std::string initial, size_t maxLength, InputType type,
                       const std::function<void(std::string)>& onSaved);
  void cyclePrayerMethod();

  bool ensureNetwork(const char* action);
  void syncWeather();
  void testNotion();
  void captureToNotion();
  void configureAiKey();
  void askAi();
  void syncNews();
  void addSavedPlace();
};
