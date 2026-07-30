#include "habitui/HabitApp.h"

#include <cstdio>

namespace habitui {

using habitcore::CellState;
using habitcore::CompletionLog;
using habitcore::Date;
using habitcore::Habit;
using habitcore::iconFromToken;

namespace {
constexpr int kHeaderH = 30;
constexpr bool INK = true;
constexpr bool BG = false;
const char* const kWeekdayInitials[7] = {"M", "T", "W", "T", "F", "S", "S"};

// Draws a checkbox with an optional tick.
void checkbox(FrameCanvas& c, int x, int y, int size, bool ticked) {
  c.drawRect(x, y, size, size, 2, INK);
  if (ticked) c.fillRect(x + 4, y + 4, size - 8, size - 8, INK);
}
}  // namespace

void HabitApp::load() {
  config_ = habitcore::HabitConfig();
  runtime_.clear();
  today_ = clock_.today();

  std::string text;
  if (store_.readConfig(text)) {
    habitcore::HabitConfig::parse(text, config_);
  }
  for (const auto& h : config_.habits()) {
    Runtime rt;
    rt.habit = h;
    runtime_.push_back(rt);
  }
  if (selected_ >= static_cast<int>(runtime_.size())) selected_ = 0;
  refreshDoneToday();
  loadedLogHabitId_ = -1;
  loaded_ = true;
}

bool HabitApp::loadTodayFor(int habitId) {
  CompletionLog log;
  log.setWindow(today_, today_);
  store_.readLog(habitId, [&](const std::string& line) { log.applyLine(line); });
  return log.isDone(today_);
}

void HabitApp::refreshDoneToday() {
  for (auto& rt : runtime_) rt.doneToday = loadTodayFor(rt.habit.id);
}

const Habit* HabitApp::selectedHabit() const {
  if (selected_ < 0 || selected_ >= static_cast<int>(runtime_.size())) return nullptr;
  return &runtime_[selected_].habit;
}

bool HabitApp::doneToday(int index) const {
  if (index < 0 || index >= static_cast<int>(runtime_.size())) return false;
  return runtime_[index].doneToday;
}

bool HabitApp::toggleSelected() {
  if (runtime_.empty()) return false;
  Runtime& rt = runtime_[selected_];
  const bool newValue = !rt.doneToday;
  if (!store_.appendLog(rt.habit.id, CompletionLog::formatRecord(today_, newValue))) return false;
  rt.doneToday = newValue;
  // Keep a loaded year log in sync so a subsequent grid view is correct.
  if (loadedLogHabitId_ == rt.habit.id) {
    selectedLog_.applyRecord(today_, newValue);
    selectedStreak_ = habitcore::computeStreak(rt.habit.schedule, selectedLog_, today_);
  }
  return true;
}

void HabitApp::ensureSelectedLog() {
  const Habit* h = selectedHabit();
  if (h == nullptr) return;
  if (loadedLogHabitId_ == h->id) return;
  selectedLog_ = CompletionLog();
  // A 53 week window is all the year grid and streak need.
  const Date windowStart = habitcore::startOfWeek(today_).addDays(-7 * 52);
  selectedLog_.setWindow(windowStart, today_);
  store_.readLog(h->id, [&](const std::string& line) { selectedLog_.applyLine(line); });
  selectedStreak_ = habitcore::computeStreak(h->schedule, selectedLog_, today_);
  loadedLogHabitId_ = h->id;
}

bool HabitApp::handleButton(AppButton button) {
  if (runtime_.empty() && button != AppButton::Back) {
    return false;  // nothing configured; only Back does anything
  }
  switch (button) {
    case AppButton::Next:
      if (screen_ != Screen::Home) return false;
      selected_ = (selected_ + 1) % static_cast<int>(runtime_.size());
      return true;
    case AppButton::Prev:
      if (screen_ != Screen::Home) return false;
      selected_ = (selected_ - 1 + static_cast<int>(runtime_.size())) % static_cast<int>(runtime_.size());
      return true;
    case AppButton::Toggle:
      if (screen_ != Screen::Home) return false;
      return toggleSelected();
    case AppButton::Grid:
      if (screen_ == Screen::Home) {
        ensureSelectedLog();
        screen_ = Screen::YearGrid;
        return true;
      }
      return false;
    case AppButton::Weekly:
      if (screen_ == Screen::Home) {
        screen_ = Screen::Weekly;
        return true;
      }
      return false;
    case AppButton::Back:
      if (screen_ != Screen::Home) {
        screen_ = Screen::Home;
        return true;
      }
      return false;
  }
  return false;
}

void HabitApp::renderHeader(FrameCanvas& c, const char* title) {
  c.fillRect(0, 0, c.width(), kHeaderH, INK);
  c.drawText(6, 6, title, 2, BG);  // white on black bar
  char right[32];
  uint8_t hh, mm;
  const std::string iso = today_.toIso();
  if (clock_.timeOfDay(hh, mm)) {
    std::snprintf(right, sizeof(right), "%s  %02u:%02u", iso.c_str(), hh, mm);
  } else {
    std::snprintf(right, sizeof(right), "%s", iso.c_str());
  }
  c.drawText(c.width() - c.textWidth(right, 1) - 6, 10, right, 1, BG);
}

void HabitApp::render(FrameCanvas& c) {
  c.clear(true);
  switch (screen_) {
    case Screen::Home:
      renderHome(c);
      break;
    case Screen::YearGrid:
      renderYearGrid(c);
      break;
    case Screen::Weekly:
      renderWeekly(c);
      break;
  }
}

void HabitApp::renderHome(FrameCanvas& c) {
  renderHeader(c, "HabitInk");
  if (runtime_.empty()) {
    c.drawTextCentered(c.width() / 2, c.height() / 2 - 10, "No habits yet.", 2, INK);
    c.drawTextCentered(c.width() / 2, c.height() / 2 + 20, "Add them in habits.tsv on the SD card.", 1, INK);
    return;
  }

  // Left: vertical list of all habits with today's checkbox.
  const int listX = 8;
  const int listTop = kHeaderH + 10;
  const int rowH = 34;
  const int listW = 372;
  for (int i = 0; i < static_cast<int>(runtime_.size()); ++i) {
    const Runtime& rt = runtime_[i];
    const int y = listTop + i * rowH;
    if (i == selected_) c.drawRect(listX - 2, y - 2, listW, rowH - 2, 2, INK);
    checkbox(c, listX + 4, y + 2, 24, rt.doneToday);
    c.drawIcon(iconFromToken(rt.habit.icon.c_str()), listX + 36, y + 3, INK);
    c.drawText(listX + 66, y + 6, rt.habit.name.c_str(), 2, INK);
  }

  // Right: focus panel for the selected habit.
  const int panelX = 400;
  c.vLine(panelX - 8, kHeaderH + 6, c.height() - kHeaderH - 12, INK);
  const Runtime& sel = runtime_[selected_];
  c.drawIcon(iconFromToken(sel.habit.icon.c_str()), panelX, kHeaderH + 20, INK);
  c.drawText(panelX + 34, kHeaderH + 24, sel.habit.name.c_str(), 2, INK);

  // Streak headline for the selected habit (loads its log lazily).
  ensureSelectedLog();
  char line[48];
  std::snprintf(line, sizeof(line), "%d", selectedStreak_.current);
  c.drawText(panelX, kHeaderH + 70, line, 6, INK);
  const int numW = c.textWidth(line, 6);
  c.drawText(panelX + numW + 12, kHeaderH + 100, "day", 2, INK);
  c.drawText(panelX + numW + 12, kHeaderH + 124, "streak", 2, INK);

  std::snprintf(line, sizeof(line), "Best: %d   Done: %d", selectedStreak_.longest, selectedStreak_.completed);
  c.drawText(panelX, kHeaderH + 170, line, 1, INK);

  const bool due = sel.habit.schedule.isDue(today_);
  if (!due) {
    c.drawText(panelX, kHeaderH + 200, "Not scheduled today", 2, INK);
  } else if (sel.doneToday) {
    c.drawText(panelX, kHeaderH + 200, "Done today", 2, INK);
    c.drawIcon(habitcore::IconId::Check, panelX + c.textWidth("Done today", 2) + 12, kHeaderH + 196, INK);
  } else {
    c.drawText(panelX, kHeaderH + 200, "Not done yet", 2, INK);
  }

  // Button hints along the bottom.
  const int hintY = c.height() - 22;
  c.hLine(0, hintY - 6, c.width(), INK);
  c.drawText(8, hintY, "Cycle", 1, INK);
  c.drawText(160, hintY, "Toggle", 1, INK);
  c.drawText(320, hintY, "Grid", 1, INK);
  c.drawText(460, hintY, "Week", 1, INK);
}

void HabitApp::renderYearGrid(FrameCanvas& c) {
  const Habit* h = selectedHabit();
  if (h == nullptr) {
    screen_ = Screen::Home;
    renderHome(c);
    return;
  }
  ensureSelectedLog();
  char title[64];
  std::snprintf(title, sizeof(title), "%s  -  year", h->name.c_str());
  renderHeader(c, title);

  const habitcore::YearGrid grid = habitcore::buildYearGrid(h->schedule, selectedLog_, today_, 53);
  const int cell = 13;
  const int gap = 1;
  const int step = cell + gap;
  const int gridX = 30;
  const int gridY = kHeaderH + 24;

  // Weekday row labels (Mon, Wed, Fri).
  for (int wd = 0; wd < 7; wd += 2) {
    c.drawText(6, gridY + wd * step + 2, kWeekdayInitials[wd], 1, INK);
  }

  for (int w = 0; w < grid.weeks; ++w) {
    for (int wd = 0; wd < 7; ++wd) {
      const habitcore::GridCell& gc = grid.at(w, wd);
      const int x = gridX + w * step;
      const int y = gridY + wd * step;
      switch (gc.state) {
        case CellState::Done:
          c.fillRect(x, y, cell, cell, INK);
          break;
        case CellState::Due:
          c.drawRect(x, y, cell, cell, INK);
          break;
        case CellState::NotDue:
          c.setPixel(x + cell / 2, y + cell / 2, INK);  // faint marker
          break;
        case CellState::Future:
        case CellState::Blank:
          break;  // leave empty
      }
    }
  }

  // Stats line beneath the grid.
  const int statsY = gridY + 7 * step + 16;
  char stats[96];
  int rate = selectedStreak_.dueSoFar > 0 ? (selectedStreak_.completed * 100) / selectedStreak_.dueSoFar : 0;
  std::snprintf(stats, sizeof(stats), "Streak %d   Best %d   Done %d   %d%%", selectedStreak_.current,
                selectedStreak_.longest, selectedStreak_.completed, rate);
  c.drawText(gridX, statsY, stats, 2, INK);

  // Legend.
  const int legY = statsY + 34;
  c.fillRect(gridX, legY, cell, cell, INK);
  c.drawText(gridX + cell + 6, legY + 2, "done", 1, INK);
  c.drawRect(gridX + 120, legY, cell, cell, INK);
  c.drawText(gridX + 120 + cell + 6, legY + 2, "missed", 1, INK);

  c.drawText(gridX, c.height() - 20, "Back to return", 1, INK);
}

void HabitApp::renderWeekly(FrameCanvas& c) {
  renderHeader(c, "This week");
  if (runtime_.empty()) {
    c.drawTextCentered(c.width() / 2, c.height() / 2, "No habits yet.", 2, INK);
    return;
  }
  const int nameW = 300;
  const int colW = 60;
  const int gridX = nameW + 10;
  const int top = kHeaderH + 30;
  const int rowH = 32;
  const int todayWd = static_cast<int>(today_.weekday());

  // Column headers with today highlighted.
  for (int wd = 0; wd < 7; ++wd) {
    const int x = gridX + wd * colW;
    if (wd == todayWd) c.fillRect(x - 2, top - 22, colW - 4, 18, INK);
    c.drawText(x + colW / 2 - 4, top - 20, kWeekdayInitials[wd], 2, wd == todayWd ? BG : INK);
  }

  for (int i = 0; i < static_cast<int>(runtime_.size()); ++i) {
    const Runtime& rt = runtime_[i];
    const int y = top + i * rowH;
    c.drawIcon(iconFromToken(rt.habit.icon.c_str()), 6, y, INK);
    c.drawText(36, y + 3, rt.habit.name.c_str(), 2, INK);

    CompletionLog log;
    log.setWindow(habitcore::startOfWeek(today_), habitcore::startOfWeek(today_).addDays(6));
    store_.readLog(rt.habit.id, [&](const std::string& line) { log.applyLine(line); });
    const habitcore::WeekRow week = habitcore::buildWeekRow(rt.habit.schedule, log, today_);

    for (int wd = 0; wd < 7; ++wd) {
      const int x = gridX + wd * colW + 8;
      switch (week.days[wd]) {
        case CellState::Done:
          checkbox(c, x, y, 22, true);
          break;
        case CellState::Due:
          checkbox(c, x, y, 22, false);
          break;
        case CellState::NotDue:
          c.hLine(x + 6, y + 11, 10, INK);  // dash: not scheduled
          break;
        case CellState::Future:
        case CellState::Blank:
          break;
      }
    }
  }
  c.drawText(6, c.height() - 20, "Back to return", 1, INK);
}

void HabitApp::renderSleepFace(FrameCanvas& c) {
  c.clear(true);
  // High-contrast standby face: today's checklist so unfinished habits stare
  // back from the desk.
  c.fillRect(0, 0, c.width(), 46, INK);
  c.drawText(10, 12, "Today", 3, BG);
  const std::string iso = today_.toIso();
  c.drawText(c.width() - c.textWidth(iso.c_str(), 2) - 10, 14, iso.c_str(), 2, BG);

  int dueCount = 0, doneCount = 0;
  for (const auto& rt : runtime_) {
    if (rt.habit.schedule.isDue(today_)) {
      ++dueCount;
      if (rt.doneToday) ++doneCount;
    }
  }

  int y = 66;
  const int rowH = 34;
  for (const auto& rt : runtime_) {
    if (!rt.habit.schedule.isDue(today_)) continue;
    checkbox(c, 12, y, 26, rt.doneToday);
    c.drawIcon(iconFromToken(rt.habit.icon.c_str()), 48, y + 1, INK);
    c.drawText(80, y + 4, rt.habit.name.c_str(), 2, INK);
    if (rt.doneToday) {
      // Strike through completed items.
      c.hLine(80, y + 14, c.textWidth(rt.habit.name.c_str(), 2), INK);
    }
    y += rowH;
    if (y > c.height() - 40) break;
  }

  char footer[48];
  std::snprintf(footer, sizeof(footer), "%d of %d done", doneCount, dueCount);
  c.fillRect(0, c.height() - 30, c.width(), 30, INK);
  c.drawText(10, c.height() - 24, footer, 2, BG);
}

}  // namespace habitui
