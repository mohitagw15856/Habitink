// Streak and grid computations over a habit's completion log.
//
// All functions are pure: they take a Schedule plus a CompletionLog and a
// reference "today", and return value types the UI and companion lay out
// however they like. Non-due days never break a streak; only a missed *due*
// day does. The current day is given grace: if today is due but not yet logged,
// the running streak is not considered broken.
#pragma once

#include <cstdint>
#include <vector>

#include "habitcore/CompletionLog.h"
#include "habitcore/Date.h"
#include "habitcore/Habit.h"

namespace habitcore {

struct StreakInfo {
  int current = 0;    // consecutive completed due days ending at/near today
  int longest = 0;    // longest run of completed due days ever
  int completed = 0;  // total completed due days in the log window
  int dueSoFar = 0;   // due days from first activity up to and including today
  bool doneToday = false;
  bool dueToday = false;
};

// Computes streaks. Requires that the log retains enough history to be
// meaningful (the firmware loads a ~1 year window; the companion loads all).
StreakInfo computeStreak(const Schedule& schedule, const CompletionLog& log, const Date& today);

// State of one cell in a rendered grid.
enum class CellState : uint8_t {
  NotDue = 0,  // outside the schedule: draw faint or empty
  Due,         // scheduled but not completed
  Done,        // scheduled and completed
  Future,      // after today: not yet actionable
  Blank        // padding before the log/window starts
};

struct GridCell {
  Date date;
  CellState state = CellState::Blank;
};

// GitHub-contributions style grid: 7 rows (Monday..Sunday) by `weeks` columns,
// the right-most column ending in the week containing `today`. Column major:
// cells[week * 7 + weekdayIndex]. Cells after today are Future.
struct YearGrid {
  int weeks = 0;
  std::vector<GridCell> cells;  // size == weeks * 7
  const GridCell& at(int week, int weekdayIndex) const { return cells[week * 7 + weekdayIndex]; }
};

YearGrid buildYearGrid(const Schedule& schedule, const CompletionLog& log, const Date& today, int weeks = 53);

// A single habit's status for the current week (Monday..Sunday of today's
// week), used by the weekly all-habits face.
struct WeekRow {
  CellState days[7] = {CellState::Blank, CellState::Blank, CellState::Blank, CellState::Blank,
                       CellState::Blank, CellState::Blank, CellState::Blank};
  int doneCount = 0;
  int dueCount = 0;
};

WeekRow buildWeekRow(const Schedule& schedule, const CompletionLog& log, const Date& today);

// Monday of the week containing `date`.
Date startOfWeek(const Date& date);

}  // namespace habitcore
