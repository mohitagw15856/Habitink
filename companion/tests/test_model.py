import datetime as dt

import pytest

from habitink.model import (
    CompletionLog,
    DataStore,
    FormatError,
    HabitConfig,
    Schedule,
)


def test_schedule_parse_daily():
    assert Schedule.parse("daily").mask == 0x7F
    assert Schedule.parse("DAILY").mask == 0x7F


def test_schedule_parse_mask():
    s = Schedule.parse("1111100")
    assert s.is_due(dt.date(2026, 7, 27))  # Monday
    assert not s.is_due(dt.date(2026, 8, 1))  # Saturday
    assert s.due_days_per_week() == 5
    assert s.to_field() == "1111100"


@pytest.mark.parametrize("bad", ["111110", "11111000", "weekly", "111111x"])
def test_schedule_parse_rejects(bad):
    with pytest.raises(FormatError):
        Schedule.parse(bad)


def test_config_parse_and_lookup():
    cfg = HabitConfig.parse(
        "1\twater\tdaily\tDrink water\n2\trun\t1111100\tMorning run\n"
    )
    assert len(cfg.habits) == 2
    assert cfg.by_id(2).name == "Morning run"
    assert cfg.by_name("drink water").id == 1


def test_config_rejects_bad_lines():
    with pytest.raises(FormatError):
        HabitConfig.parse("1\twater\tdaily\n")  # too few fields
    with pytest.raises(FormatError):
        HabitConfig.parse("99\twater\tdaily\tOut of range\n")
    with pytest.raises(FormatError):
        HabitConfig.parse("1\twater\tdaily\tA\n1\trun\tdaily\tDup\n")


def test_config_caps_at_twelve():
    text = "".join(f"{i}\tcheck\tdaily\tH{i}\n" for i in range(1, 16))
    cfg = HabitConfig.parse(text)
    assert len(cfg.habits) == 12


def test_config_roundtrip():
    cfg = HabitConfig.parse("1\twater\tdaily\tDrink\n2\trun\t1010100\tRun\n")
    reparsed = HabitConfig.parse(cfg.serialise())
    assert reparsed.by_id(2).schedule.to_field() == "1010100"


def test_completion_log_last_write_wins():
    log = CompletionLog()
    log.apply_line("2026-07-30\t1")
    assert log.is_done(dt.date(2026, 7, 30))
    log.apply_line("2026-07-30\t0")
    assert not log.is_done(dt.date(2026, 7, 30))


def test_completion_log_ignores_junk():
    log = CompletionLog()
    assert not log.apply_line("# comment")
    assert not log.apply_line("")
    assert not log.apply_line("nonsense")
    assert not log.apply_line("2026-13-40\t1")
    assert log.apply_line("2026-07-30 1")  # space separator accepted


def test_datastore_locate_variants(tmp_path):
    data = tmp_path / ".habitink"
    data.mkdir()
    assert DataStore.locate(tmp_path).root == data
    assert DataStore.locate(data).root == data


def test_datastore_read_write_roundtrip(tmp_path):
    store = DataStore.locate(tmp_path)
    cfg = HabitConfig.parse("1\twater\tdaily\tDrink\n")
    store.write_config(cfg)
    assert store.read_config().by_id(1).name == "Drink"


def test_datastore_append_creates_header(tmp_path):
    store = DataStore.locate(tmp_path)
    store.append_log(1, [(dt.date(2026, 7, 30), True)])
    text = store.log_path(1).read_text()
    assert text.splitlines()[0].startswith("#")
    assert "2026-07-30\t1" in text
