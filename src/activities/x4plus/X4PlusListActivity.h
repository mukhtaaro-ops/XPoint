#pragma once
#include <string>
#include <vector>
#include "activities/UiListActivity.h"
class X4PlusListActivity final : public UiListActivity {
 public:
  enum class Mode : uint8_t { Calendar, Tasks, Notes, Cards, Study, Quran, Clippings, Prayer, Focus, Wallet };
  X4PlusListActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, Mode mode);
  void onEnter() override;
 private:
  int listCount() const override;
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  void onRowLongPress(int index) override;
  const char* headerTitle() const override;
  void load();
  void save() const;
  void rebuildRows();
  void openEditor(int index);
  void createStudyCardFromClipping(int index);
  const char* filePath() const;
  const char* addLabel() const;
  Mode mode;
  std::vector<std::string> items;
  std::vector<bool> completed;
  std::vector<bool> revealed;
  std::vector<freeink::ui::ListItem> rows;
  std::vector<std::string> displayLabels;
};
