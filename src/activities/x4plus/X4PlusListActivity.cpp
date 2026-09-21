#include "X4PlusListActivity.h"
#include <ArduinoJson.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>
#include <PersistableStore.h>
#include "activities/ActivityManager.h"
#include "activities/ActivityResult.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
namespace fui = freeink::ui;
X4PlusListActivity::X4PlusListActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const Mode mode)
    : UiListActivity("X4Plus", renderer, mappedInput, true), mode(mode) {}
void X4PlusListActivity::onEnter() { UiListActivity::onEnter(); load(); rebuildRows(); }
const char* X4PlusListActivity::filePath() const {
  switch (mode) {
    case Mode::Calendar: return "/.crosspoint/x4plus-calendar.json";
    case Mode::Tasks: return "/.crosspoint/x4plus-tasks.json";
    case Mode::Notes: return "/.crosspoint/x4plus-notes.json";
    case Mode::Cards: return "/.crosspoint/x4plus-cards.json";
    case Mode::Study: return "/.crosspoint/x4plus-study.json";
    case Mode::Quran: return "/.crosspoint/x4plus-quran.json";
    case Mode::Clippings: return "/.crosspoint/x4plus-clippings.json";
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
  }
  return tr(STR_X4P_ADD_NOTE);
}
void X4PlusListActivity::load() {
  items.clear(); completed.clear(); revealed.clear(); JsonDocument doc;
  if (!PersistableStoreBase::readDocFromFile(filePath(), doc)) return;
  JsonArrayConst values = doc["items"].as<JsonArrayConst>();
  items.reserve(values.size()); completed.reserve(values.size());
  for (JsonVariantConst value : values) {
    const char* text = value["text"] | "";
    if (!text[0]) continue;
    items.emplace_back(text); completed.push_back(value["done"] | false); revealed.push_back(false);
  }
}
void X4PlusListActivity::save() const {
  JsonDocument doc; JsonArray values = doc["items"].to<JsonArray>();
  for (size_t i = 0; i < items.size(); ++i) {
    JsonObject value = values.add<JsonObject>();
    value["text"] = items[i];
    value["done"] = mode == Mode::Tasks && i < completed.size() ? completed[i] : false;
  }
  if (!PersistableStoreBase::writeDocToFile(filePath(), doc)) LOG_ERR("X4P", "Failed to save %s", filePath());
}
void X4PlusListActivity::rebuildRows() {
  rows.clear(); displayLabels.clear(); rows.reserve(items.size() + 1); displayLabels.reserve(items.size());
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
  for (size_t i = 0; i < displayLabels.size(); ++i) {
    fui::ListItem row; row.label = displayLabels[i].c_str(); row.actionValue = static_cast<int16_t>(i);
    rows.push_back(row);
  }
  fui::ListItem add; add.label = addLabel(); add.actionValue = static_cast<int16_t>(items.size());
  rows.push_back(add);
}
int X4PlusListActivity::listCount() const { return static_cast<int>(items.size() + 1); }
void X4PlusListActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect safe = UITheme::getInstance().getScreenSafeArea(renderer, true, false);
  screen.setContentMarginFromScreen(fui::Insets{static_cast<int16_t>(safe.y + metrics.topPadding + metrics.headerHeight),
    static_cast<int16_t>(renderer.getScreenWidth() - (safe.x + safe.width)),
    static_cast<int16_t>(renderer.getScreenHeight() - (safe.y + safe.height) + metrics.buttonHintsHeight),
    static_cast<int16_t>(safe.x)});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));
  fui::ListProps props; props.items = rows.data(); props.count = static_cast<uint16_t>(rows.size());
  props.action = ACTION_ROW; props.inputMask = fui::InputTouch | fui::InputLongPress; syncListViewport(screen, props); screen.list(props);
}
void X4PlusListActivity::openEditor(const int index) {
  const bool creating = index >= static_cast<int>(items.size());
  const std::string initial = creating ? std::string{} : items[index];
  auto editor = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput, addLabel(), initial, 512, InputType::Text);
  if (!editor) { LOG_ERR("X4P", "OOM: keyboard editor"); return; }
  startActivityForResult(std::move(editor), [this, index, creating](const ActivityResult& result) {
    if (result.isCancelled || !std::holds_alternative<KeyboardResult>(result.data)) return;
    std::string text = std::get<KeyboardResult>(result.data).text; if (text.empty()) return;
    if (creating) { items.push_back(std::move(text)); completed.push_back(false); revealed.push_back(false); }
    else if (index >= 0 && index < static_cast<int>(items.size())) items[index] = std::move(text);
    save(); rebuildRows(); requestUpdate();
  });
}
void X4PlusListActivity::activateIndex(const int index) {
  app.clearTapFlash();
  if (index == static_cast<int>(items.size())) { openEditor(index); return; }
  if (index < 0 || index >= static_cast<int>(items.size())) return;
  if (mode == Mode::Tasks) { completed[index] = !completed[index]; save(); rebuildRows(); requestUpdate(); return; }
  if (mode == Mode::Study) {
    if (revealed.size() < items.size()) revealed.resize(items.size(), false);
    revealed[index] = !revealed[index];
    rebuildRows(); requestUpdate(); return;
  }
  openEditor(index);
}
void X4PlusListActivity::onRowLongPress(const int index) {
  if (index < 0 || index >= static_cast<int>(items.size())) return;
  items.erase(items.begin() + index); if (index < static_cast<int>(completed.size())) completed.erase(completed.begin() + index);
  if (index < static_cast<int>(revealed.size())) revealed.erase(revealed.begin() + index);
  save(); rebuildRows(); nav.selected = std::min(index, static_cast<int>(items.size())); requestUpdate();
}
