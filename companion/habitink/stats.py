"""Streak and grid statistics, mirroring habitcore/Stats.cpp exactly.

Keeping the rules identical on both sides means the companion reports the same
numbers the device shows.
"""
from __future__ import annotations

import datetime as dt
from dataclasses import dataclass

from .model import CompletionLog, Schedule

ONE_DAY = dt.timedelta(days=1)


@dataclass
class StreakInfo:
    current: int = 0
    longest: int = 0
    completed: int = 0
    due_so_far: int = 0
    done_today: bool = False
    due_today: bool = False

    @property
    def rate(self) -> int:
        return (self.completed * 100) // self.due_so_far if self.due_so_far else 0


def start_of_week(date: dt.date) -> dt.date:
    return date - dt.timedelta(days=date.weekday())


def compute_streak(schedule: Schedule, log: CompletionLog, today: dt.date) -> StreakInfo:
    info = StreakInfo()
    info.due_today = schedule.is_due(today)
    info.done_today = log.is_done(today)

    done_dates = log.done_dates()
    info.completed = sum(1 for d in done_dates if schedule.is_due(d))

    if schedule.mask & 0x7F == 0:
        return info

    # Current streak with same-day grace.
    streak = 0
    day = today
    started_grace = False
    floor = today - dt.timedelta(days=366 * 5)
    if done_dates and done_dates[0] < floor:
        floor = done_dates[0]
    while day >= floor:
        if schedule.is_due(day):
            if log.is_done(day):
                streak += 1
            elif day == today and not started_grace:
                started_grace = True
            else:
                break
        day -= ONE_DAY
    info.current = streak

    # Longest run of consecutive completed due days.
    best = run = 0
    if done_dates:
        day = done_dates[0]
        while day <= today:
            if schedule.is_due(day):
                if log.is_done(day):
                    run += 1
                    best = max(best, run)
                else:
                    run = 0
            day += ONE_DAY
    info.longest = max(best, info.current)

    # Due days from first activity to today.
    due = 0
    if done_dates:
        day = done_dates[0]
        while day <= today:
            if schedule.is_due(day):
                due += 1
            day += ONE_DAY
    info.due_so_far = due
    return info


# Grid cell states mirror habitcore::CellState.
NOT_DUE = "not_due"
DUE = "due"
DONE = "done"
FUTURE = "future"
BLANK = "blank"


def cell_state(schedule: Schedule, log: CompletionLog, date: dt.date, today: dt.date,
               anchor: dt.date | None) -> str:
    if date > today:
        return FUTURE
    if log.is_done(date):
        return DONE
    if not schedule.is_due(date):
        return NOT_DUE
    if anchor is None or date < anchor:
        return BLANK
    return DUE


def build_year_grid(schedule: Schedule, log: CompletionLog, today: dt.date, weeks: int = 53) -> list[list[str]]:
    """Returns weeks columns, each a list of 7 cell states (Mon..Sun)."""
    anchor = log.first_recorded()
    this_monday = start_of_week(today)
    first_monday = this_monday - dt.timedelta(days=7 * (weeks - 1))
    grid = []
    for w in range(weeks):
        col = []
        for wd in range(7):
            date = first_monday + dt.timedelta(days=7 * w + wd)
            col.append(cell_state(schedule, log, date, today, anchor))
        grid.append(col)
    return grid


def month_days(year: int, month: int) -> list[dt.date]:
    first = dt.date(year, month, 1)
    if month == 12:
        nxt = dt.date(year + 1, 1, 1)
    else:
        nxt = dt.date(year, month + 1, 1)
    days = []
    d = first
    while d < nxt:
        days.append(d)
        d += ONE_DAY
    return days
