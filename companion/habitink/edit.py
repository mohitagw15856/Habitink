"""`habitink edit`: backfill or correct completion records.

Edits are append-only, exactly like the device: marking a past day done or
cleared writes a new record whose value wins over any earlier one for that date.
Nothing is rewritten, so the log stays a faithful audit trail.
"""
from __future__ import annotations

import datetime as dt

from .model import DataStore, Habit, HabitConfig


def resolve_habit(config: HabitConfig, selector: str) -> Habit:
    """Finds a habit by numeric id or by name."""
    if selector.isdigit():
        habit = config.by_id(int(selector))
        if habit:
            return habit
    habit = config.by_name(selector)
    if habit:
        return habit
    raise KeyError(f"no habit matching {selector!r}")


def parse_dates(date_spec: str) -> list[dt.date]:
    """Parses a single date or an inclusive DATE..DATE range."""
    if ".." in date_spec:
        start_s, _, end_s = date_spec.partition("..")
        start = dt.date.fromisoformat(start_s.strip())
        end = dt.date.fromisoformat(end_s.strip())
        if end < start:
            start, end = end, start
        out = []
        d = start
        while d <= end:
            out.append(d)
            d += dt.timedelta(days=1)
        return out
    return [dt.date.fromisoformat(date_spec.strip())]


def run_edit(store: DataStore, selector: str, date_spec: str, done: bool) -> list[dt.date]:
    """Appends records marking the given dates done/cleared for a habit.

    Returns the list of dates written.
    """
    config = store.read_config()
    habit = resolve_habit(config, selector)
    dates = parse_dates(date_spec)
    store.append_log(habit.id, ((d, done) for d in dates))
    return dates
