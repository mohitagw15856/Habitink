// The whole on-device application: state, input handling and rendering.
//
// HabitApp owns the tiny bit of RAM state HabitInk keeps between button presses
// (the loaded config, one boolean per habit for "done today", and the selected
// habit). Everything heavier is streamed from the Store on demand and dropped
// again. It is deliberately hardware-free so the native tests drive the exact
// same code the firmware runs.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "habitcore/CompletionLog.h"
#include "habitcore/HabitConfig.h"
#include "habitcore/Stats.h"
#include "habitui/Env.h"
#include "habitui/FrameCanvas.h"

namespace habitui {

enum class Screen : uint8_t { Home, YearGrid, Weekly };

class HabitApp {
 public:
  HabitApp(Store& store, Clock& clock) : store_(store), clock_(clock) {}

  // Reads the config and today's completion for every habit. Call once on wake.
  void load();

  // Handles a logical button. Returns true when the display needs repainting.
  bool handleButton(AppButton button);

  // Paints the current screen. Caller flushes the canvas to the panel after.
  void render(FrameCanvas& canvas);

  // Paints the standby "sleep face": today's checklist, drawn so unfinished
  // habits are visible on the desk. Independent of the current screen.
  void renderSleepFace(FrameCanvas& canvas);

  // --- accessors, mainly for tests and the firmware glue ---
  Screen screen() const { return screen_; }
  int habitCount() const { return config_.size(); }
  int selectedIndex() const { return selected_; }
  const habitcore::Habit* selectedHabit() const;
  bool doneToday(int index) const;
  habitcore::Date today() const { return today_; }
  bool loaded() const { return loaded_; }

 private:
  struct Runtime {
    habitcore::Habit habit;
    bool doneToday = false;
  };

  void refreshDoneToday();
  bool toggleSelected();
  void ensureSelectedLog();  // load the year window log for the selected habit
  void renderHome(FrameCanvas& canvas);
  void renderYearGrid(FrameCanvas& canvas);
  void renderWeekly(FrameCanvas& canvas);
  void renderHeader(FrameCanvas& canvas, const char* title);
  bool loadTodayFor(int habitId);

  Store& store_;
  Clock& clock_;
  habitcore::HabitConfig config_;
  std::vector<Runtime> runtime_;
  habitcore::Date today_;
  int selected_ = 0;
  Screen screen_ = Screen::Home;
  bool loaded_ = false;

  // Lazily loaded full-window log + stats for the selected habit (year grid /
  // streak). Kept for one habit at a time to bound RAM.
  int loadedLogHabitId_ = -1;
  habitcore::CompletionLog selectedLog_;
  habitcore::StreakInfo selectedStreak_;
};

}  // namespace habitui
