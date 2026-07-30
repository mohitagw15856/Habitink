#include "check.h"
#include "habitcore/CompletionLog.h"
#include "habitcore/IconSet.h"
#include "habitcore/Stats.h"

using namespace habitcore;

TEST(log_last_write_wins) {
  CompletionLog log;
  CHECK(log.applyLine("2026-07-30\t1"));
  CHECK(log.isDone(Date(2026, 7, 30)));
  CHECK(log.applyLine("2026-07-30\t0"));  // cleared
  CHECK(!log.isDone(Date(2026, 7, 30)));
  CHECK(log.applyLine("2026-07-30\t1"));  // re-done
  CHECK(log.isDone(Date(2026, 7, 30)));
}

TEST(log_ignores_comments_and_junk) {
  CompletionLog log;
  CHECK(!log.applyLine("# habitink log v1"));
  CHECK(!log.applyLine(""));
  CHECK(!log.applyLine("garbage line"));
  CHECK(!log.applyLine("2026-13-40\t1"));  // invalid date
  CHECK(log.applyLine("2026-07-30\t1"));
  CHECK_EQ(log.doneCount(), 1u);
}

TEST(log_accepts_crlf_and_spaces) {
  CompletionLog log;
  CHECK(log.applyLine("2026-07-30\t1\r"));
  CHECK(log.applyLine("2026-07-31 1"));  // space separator
  CHECK(log.isDone(Date(2026, 7, 30)));
  CHECK(log.isDone(Date(2026, 7, 31)));
}

TEST(log_window_bounds_memory) {
  CompletionLog log;
  log.setWindow(Date(2026, 7, 1), Date(2026, 7, 31));
  CHECK(log.applyLine("2026-06-15\t1"));   // before window: dropped
  CHECK(log.applyLine("2026-07-10\t1"));   // inside
  CHECK(log.applyLine("2026-08-01\t1"));   // after window: dropped
  CHECK_EQ(log.doneCount(), 1u);
  CHECK(log.isDone(Date(2026, 7, 10)));
  CHECK(!log.isDone(Date(2026, 6, 15)));
}

TEST(log_format_record) {
  CHECK_EQ(CompletionLog::formatRecord(Date(2026, 7, 30), true), std::string("2026-07-30\t1"));
  CHECK_EQ(CompletionLog::formatRecord(Date(2026, 7, 30), false), std::string("2026-07-30\t0"));
}

TEST(streak_daily_consecutive) {
  Schedule daily = Schedule::daily();
  CompletionLog log;
  // Done for the four days ending today (2026-07-30).
  for (int d = 27; d <= 30; ++d) log.applyRecord(Date(2026, 7, d), true);
  StreakInfo s = computeStreak(daily, log, Date(2026, 7, 30));
  CHECK_EQ(s.current, 4);
  CHECK_EQ(s.longest, 4);
  CHECK(s.doneToday);
  CHECK(s.dueToday);
}

TEST(streak_grace_for_unfinished_today) {
  Schedule daily = Schedule::daily();
  CompletionLog log;
  // Done through yesterday, today not yet logged.
  for (int d = 25; d <= 29; ++d) log.applyRecord(Date(2026, 7, d), true);
  StreakInfo s = computeStreak(daily, log, Date(2026, 7, 30));
  CHECK_EQ(s.current, 5);  // grace: unfinished today does not break it
  CHECK(!s.doneToday);
  CHECK(s.dueToday);
}

TEST(streak_broken_by_missed_earlier_day) {
  Schedule daily = Schedule::daily();
  CompletionLog log;
  log.applyRecord(Date(2026, 7, 26), true);
  // 2026-07-27 missed
  log.applyRecord(Date(2026, 7, 28), true);
  log.applyRecord(Date(2026, 7, 29), true);
  log.applyRecord(Date(2026, 7, 30), true);
  StreakInfo s = computeStreak(daily, log, Date(2026, 7, 30));
  CHECK_EQ(s.current, 3);   // 28, 29, 30
  CHECK_EQ(s.longest, 3);
}

TEST(streak_weekday_schedule_skips_non_due) {
  // Mon-Fri schedule; weekends are not due and must not break the streak.
  Schedule s;
  Schedule::parseField("1111100", s);
  CompletionLog log;
  // 2026-07-24 is a Friday. Log Mon-Fri that week and the prior Friday, then
  // today = Monday 2026-07-27.
  log.applyRecord(Date(2026, 7, 24), true);  // Fri
  log.applyRecord(Date(2026, 7, 27), true);  // Mon (today)
  StreakInfo info = computeStreak(s, log, Date(2026, 7, 27));
  // Fri then Mon are consecutive *due* days (Sat/Sun skipped): streak of 2.
  CHECK_EQ(info.current, 2);
}

TEST(streak_none_when_never_scheduled) {
  Schedule never(0);
  CompletionLog log;
  log.applyRecord(Date(2026, 7, 30), true);
  StreakInfo s = computeStreak(never, log, Date(2026, 7, 30));
  CHECK_EQ(s.current, 0);
}

TEST(grid_dimensions_and_today_column) {
  Schedule daily = Schedule::daily();
  CompletionLog log;
  log.applyRecord(Date(2026, 7, 30), true);
  YearGrid g = buildYearGrid(daily, log, Date(2026, 7, 30), 53);
  CHECK_EQ(g.weeks, 53);
  CHECK_EQ(g.cells.size(), static_cast<size_t>(53 * 7));

  // Today is Thursday 2026-07-30 -> weekday index 3, last column.
  const GridCell& todayCell = g.at(52, 3);
  CHECK(todayCell.date == Date(2026, 7, 30));
  CHECK(todayCell.state == CellState::Done);

  // The day after today in the last column is Future.
  const GridCell& tomorrow = g.at(52, 4);
  CHECK(tomorrow.state == CellState::Future);
}

TEST(grid_marks_due_vs_notdue) {
  Schedule weekdays;
  Schedule::parseField("1111100", weekdays);
  CompletionLog log;
  // Anchor the habit two weeks back so weekdays since then that are unlogged
  // count as missed (Due) and the weekend stays NotDue.
  log.applyRecord(Date(2026, 7, 20), true);  // a Monday
  YearGrid g = buildYearGrid(weekdays, log, Date(2026, 7, 30), 2);
  bool sawNotDue = false, sawDue = false;
  for (const auto& c : g.cells) {
    if (c.state == CellState::NotDue) sawNotDue = true;
    if (c.state == CellState::Due) sawDue = true;
  }
  CHECK(sawNotDue);
  CHECK(sawDue);
}

TEST(grid_blank_before_first_record) {
  Schedule daily = Schedule::daily();
  CompletionLog log;
  log.applyRecord(Date(2026, 7, 28), true);  // habit started three days ago
  YearGrid g = buildYearGrid(daily, log, Date(2026, 7, 30), 4);
  // A day well before the first record must be Blank, not Due (missed).
  bool sawBlankBeforeStart = false;
  for (const auto& c : g.cells) {
    if (c.date < Date(2026, 7, 28) && c.date <= Date(2026, 7, 30)) {
      if (c.state == CellState::Blank) sawBlankBeforeStart = true;
      CHECK(c.state != CellState::Due);
    }
  }
  CHECK(sawBlankBeforeStart);
}

TEST(log_first_recorded_day) {
  CompletionLog log;
  int32_t first = 0;
  CHECK(!log.firstRecordedDay(first));
  log.applyRecord(Date(2026, 7, 30), false);  // even a cleared record anchors
  log.applyRecord(Date(2026, 7, 20), true);
  CHECK(log.firstRecordedDay(first));
  CHECK_EQ(first, Date(2026, 7, 20).toDayNumber());
}

TEST(week_row_counts) {
  Schedule daily = Schedule::daily();
  CompletionLog log;
  // Week of Monday 2026-07-27 .. Sunday 2026-08-02. Today = Thu 07-30.
  log.applyRecord(Date(2026, 7, 27), true);
  log.applyRecord(Date(2026, 7, 28), true);
  WeekRow row = buildWeekRow(daily, log, Date(2026, 7, 30));
  CHECK_EQ(row.doneCount, 2);
  CHECK(row.days[0] == CellState::Done);  // Mon
  CHECK(row.days[1] == CellState::Done);  // Tue
  CHECK(row.days[2] == CellState::Due);   // Wed, missed
  CHECK(row.days[3] == CellState::Due);   // Thu today, not done
  CHECK(row.days[4] == CellState::Future);  // Fri
}

TEST(icon_token_roundtrip) {
  CHECK(iconFromToken("water") == IconId::Water);
  CHECK(iconFromToken("run") == IconId::Run);
  CHECK(iconFromToken("unknown-token") == IconId::Check);  // fallback
  CHECK_EQ(std::string(tokenFromIcon(IconId::Book)), std::string("book"));
}
