"""Mirror the C++ streak/grid tests so both implementations stay in lockstep."""
import datetime as dt

from habitink.model import CompletionLog, Schedule
from habitink.stats import (
    BLANK,
    DONE,
    DUE,
    FUTURE,
    NOT_DUE,
    build_year_grid,
    compute_streak,
    start_of_week,
)

TODAY = dt.date(2026, 7, 30)  # a Thursday


def _log(*records):
    log = CompletionLog()
    for date, done in records:
        log.apply_record(date, done)
    return log


def test_streak_daily_consecutive():
    log = _log(*((dt.date(2026, 7, d), True) for d in range(27, 31)))
    s = compute_streak(Schedule.daily(), log, TODAY)
    assert s.current == 4
    assert s.longest == 4
    assert s.done_today


def test_streak_grace_for_unfinished_today():
    log = _log(*((dt.date(2026, 7, d), True) for d in range(25, 30)))
    s = compute_streak(Schedule.daily(), log, TODAY)
    assert s.current == 5
    assert not s.done_today


def test_streak_broken_by_missed_day():
    log = _log(
        (dt.date(2026, 7, 26), True),
        (dt.date(2026, 7, 28), True),
        (dt.date(2026, 7, 29), True),
        (dt.date(2026, 7, 30), True),
    )
    s = compute_streak(Schedule.daily(), log, TODAY)
    assert s.current == 3


def test_streak_weekday_schedule_skips_weekend():
    s = Schedule.parse("1111100")
    log = _log((dt.date(2026, 7, 24), True), (dt.date(2026, 7, 27), True))
    info = compute_streak(s, log, dt.date(2026, 7, 27))
    assert info.current == 2  # Fri then Mon, weekend skipped


def test_streak_rate():
    log = _log((dt.date(2026, 7, 28), True), (dt.date(2026, 7, 30), True))
    s = compute_streak(Schedule.daily(), log, TODAY)
    # First activity 28th; due days 28,29,30 = 3; completed 28,30 = 2.
    assert s.due_so_far == 3
    assert s.completed == 2
    assert s.rate == 66


def test_grid_dimensions_and_today():
    log = _log((dt.date(2026, 7, 30), True))
    grid = build_year_grid(Schedule.daily(), log, TODAY, weeks=53)
    assert len(grid) == 53
    assert all(len(col) == 7 for col in grid)
    # Today is Thursday -> weekday index 3, last column.
    assert grid[52][3] == DONE
    assert grid[52][4] == FUTURE


def test_grid_blank_before_first_record():
    log = _log((dt.date(2026, 7, 28), True))
    grid = build_year_grid(Schedule.daily(), log, TODAY, weeks=4)
    states = {state for col in grid for state in col}
    assert BLANK in states
    # No day before the anchor should be marked DUE (missed).
    for w, col in enumerate(grid):
        for wd, state in enumerate(col):
            monday = start_of_week(TODAY) - dt.timedelta(days=7 * (4 - 1))
            date = monday + dt.timedelta(days=7 * w + wd)
            if date < dt.date(2026, 7, 28) and date <= TODAY:
                assert state != DUE


def test_grid_weekend_not_due():
    s = Schedule.parse("1111100")
    log = _log((dt.date(2026, 7, 20), True))
    grid = build_year_grid(s, log, TODAY, weeks=2)
    states = {state for col in grid for state in col}
    assert NOT_DUE in states
    assert DUE in states
