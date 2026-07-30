#include "habitcore/Stats.h"

namespace habitcore {

Date startOfWeek(const Date& date) {
  const int wd = static_cast<int>(date.weekday());  // Monday = 0
  return date.addDays(-wd);
}

StreakInfo computeStreak(const Schedule& schedule, const CompletionLog& log, const Date& today) {
  StreakInfo info;
  info.dueToday = schedule.isDue(today);
  info.doneToday = log.isDone(today);

  const std::vector<int32_t> doneDays = log.doneDays();
  info.completed = 0;
  for (int32_t dn : doneDays) {
    if (schedule.isDue(Date::fromDayNumber(dn))) ++info.completed;
  }

  if (schedule.isNever()) return info;

  const int32_t todayDn = today.toDayNumber();

  // Current streak: walk backwards over due days. Today is allowed grace: if it
  // is due but not done, we start counting from the previous due day instead of
  // treating the gap as broken. Any earlier missed due day stops the count.
  {
    int streak = 0;
    int32_t dn = todayDn;
    bool startedGrace = false;
    // Establish a lower bound so a habit with no history terminates quickly.
    int32_t floorDn = todayDn - 366 * 5;  // five years is plenty of headroom
    if (!doneDays.empty() && doneDays.front() < floorDn) floorDn = doneDays.front();

    while (dn >= floorDn) {
      const Date d = Date::fromDayNumber(dn);
      if (schedule.isDue(d)) {
        const bool done = log.isDone(dn);
        if (done) {
          ++streak;
        } else if (dn == todayDn && !startedGrace) {
          // Grace for an unfinished today: skip without breaking.
          startedGrace = true;
        } else {
          break;
        }
      }
      --dn;
    }
    info.current = streak;
  }

  // Longest streak: scan forward across the retained window counting the
  // longest run of consecutive completed due days.
  {
    int best = 0;
    int run = 0;
    if (!doneDays.empty()) {
      const int32_t first = doneDays.front();
      for (int32_t dn = first; dn <= todayDn; ++dn) {
        const Date d = Date::fromDayNumber(dn);
        if (!schedule.isDue(d)) continue;
        if (log.isDone(dn)) {
          ++run;
          if (run > best) best = run;
        } else {
          run = 0;
        }
      }
    }
    info.longest = best;
    if (info.current > info.longest) info.longest = info.current;
  }

  // Due days so far (from first activity to today), for a completion rate.
  {
    int due = 0;
    if (!doneDays.empty()) {
      for (int32_t dn = doneDays.front(); dn <= todayDn; ++dn) {
        if (schedule.isDue(Date::fromDayNumber(dn))) ++due;
      }
    }
    info.dueSoFar = due;
  }

  return info;
}

namespace {
// anchorDn is the first day the habit was ever logged. Scheduled days before it
// render Blank (the habit did not exist yet) rather than as missed.
CellState cellFor(const Schedule& schedule, const CompletionLog& log, const Date& date, int32_t todayDn,
                  int32_t anchorDn, bool hasAnchor) {
  const int32_t dn = date.toDayNumber();
  if (dn > todayDn) return CellState::Future;
  if (log.isDone(dn)) return CellState::Done;
  if (!schedule.isDue(date)) return CellState::NotDue;
  if (!hasAnchor || dn < anchorDn) return CellState::Blank;  // before the habit started
  return CellState::Due;                                     // scheduled, on/after start, not done
}
}  // namespace

YearGrid buildYearGrid(const Schedule& schedule, const CompletionLog& log, const Date& today, int weeks) {
  if (weeks < 1) weeks = 1;
  YearGrid grid;
  grid.weeks = weeks;
  grid.cells.resize(static_cast<size_t>(weeks) * 7);

  const int32_t todayDn = today.toDayNumber();
  int32_t anchorDn = 0;
  const bool hasAnchor = log.firstRecordedDay(anchorDn);
  // The right-most column is the week containing today; walk back `weeks-1`
  // whole weeks to find the first column's Monday.
  const Date thisMonday = startOfWeek(today);
  const Date firstMonday = thisMonday.addDays(-7 * (weeks - 1));

  for (int w = 0; w < weeks; ++w) {
    for (int wd = 0; wd < 7; ++wd) {
      const Date date = firstMonday.addDays(7 * w + wd);
      GridCell cell;
      cell.date = date;
      cell.state = cellFor(schedule, log, date, todayDn, anchorDn, hasAnchor);
      grid.cells[static_cast<size_t>(w) * 7 + wd] = cell;
    }
  }
  return grid;
}

WeekRow buildWeekRow(const Schedule& schedule, const CompletionLog& log, const Date& today) {
  WeekRow row;
  const int32_t todayDn = today.toDayNumber();
  int32_t anchorDn = 0;
  const bool hasAnchor = log.firstRecordedDay(anchorDn);
  const Date monday = startOfWeek(today);
  for (int wd = 0; wd < 7; ++wd) {
    const Date date = monday.addDays(wd);
    const CellState state = cellFor(schedule, log, date, todayDn, anchorDn, hasAnchor);
    row.days[wd] = state;
    if (state == CellState::Due || state == CellState::Done) ++row.dueCount;
    if (state == CellState::Done) ++row.doneCount;
  }
  return row;
}

}  // namespace habitcore
