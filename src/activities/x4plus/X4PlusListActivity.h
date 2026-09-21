#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "activities/UiListActivity.h"

class X4PlusListActivity final : public UiListActivity {
 public:
  enum class Mode : uint8_t { Calendar, Tasks, Notes, Cards, Study, Quran, Clippings, Prayer, Focus, Wallet, Calculator };
  X4PlusListActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, Mode mode);
  void onEnter() override;

 private:
  int listCount() const override;
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  void onRowLongPress(int index) override;
  bool handleCustomInput() override;
  const char* headerTitle() const override;
  void load();
  void save() const;
  void rebuildRows();
  void openEditor(int index);
  void openPrayerConfigEditor();
  void createStudyCardFromClipping(int index);
  void startFocus(uint32_t seconds, bool isBreak);
  void pauseFocus();
  uint32_t focusRemainingSeconds() const;
  const char* filePath() const;
  const char* addLabel() const;

  Mode mode;
  std::vector<std::string> items;
  std::vector<bool> completed;
  std::vector<bool> revealed;
  std::vector<freeink::ui::ListItem> rows;
  std::vector<std::string> displayLabels;

  bool focusRunning = false;
  bool focusBreak = false;
  uint32_t focusEndMs = 0;
  uint32_t focusPausedSeconds = 25 * 60;
  uint32_t focusSessions = 0;
  uint32_t focusLastBucket = UINT32_MAX;

  double prayerLatitude = -26.2041;
  double prayerLongitude = 28.0473;
  int prayerUtcOffsetMinutes = 120;
  double prayerFajrAngle = 18.0;
  double prayerIshaAngle = 17.0;
  double prayerAsrShadowFactor = 1.0;  // 1 = standard, 2 = Hanafi
};
