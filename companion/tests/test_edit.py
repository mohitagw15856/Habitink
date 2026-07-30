import datetime as dt

import pytest

from habitink.edit import parse_dates, resolve_habit, run_edit
from habitink.model import DataStore, HabitConfig


def test_parse_single_date():
    assert parse_dates("2026-07-30") == [dt.date(2026, 7, 30)]


def test_parse_date_range():
    dates = parse_dates("2026-07-28..2026-07-30")
    assert dates == [dt.date(2026, 7, 28), dt.date(2026, 7, 29), dt.date(2026, 7, 30)]


def test_parse_date_range_reversed():
    dates = parse_dates("2026-07-30..2026-07-28")
    assert dates[0] == dt.date(2026, 7, 28)  # normalised ascending


def test_resolve_habit_by_id_and_name():
    cfg = HabitConfig.parse("1\twater\tdaily\tDrink water\n2\trun\tdaily\tRun\n")
    assert resolve_habit(cfg, "2").name == "Run"
    assert resolve_habit(cfg, "Drink water").id == 1
    with pytest.raises(KeyError):
        resolve_habit(cfg, "nope")


def test_run_edit_appends_backfill(store):
    # Backfill a missed day for habit 2 (which had no log yet).
    dates = run_edit(store, "2", "2026-07-28", done=True)
    assert dates == [dt.date(2026, 7, 28)]
    log = store.read_log(2)
    assert log.is_done(dt.date(2026, 7, 28))


def test_run_edit_clear_wins(store):
    run_edit(store, "1", "2026-07-30", done=True)
    run_edit(store, "1", "2026-07-30", done=False)
    log = store.read_log(1)
    assert not log.is_done(dt.date(2026, 7, 30))  # last write wins
    # Append-only: both records are still on disk.
    text = store.log_path(1).read_text()
    assert text.count("2026-07-30") >= 2


def test_run_edit_range(store):
    dates = run_edit(store, "3", "2026-07-01..2026-07-03", done=True)
    assert len(dates) == 3
    log = store.read_log(3)
    assert all(log.is_done(d) for d in dates)
