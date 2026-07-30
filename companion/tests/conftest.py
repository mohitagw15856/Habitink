import datetime as dt
from pathlib import Path

import pytest

from habitink.model import CompletionLog, DataStore, HabitConfig


@pytest.fixture
def sd(tmp_path: Path) -> Path:
    """A temporary SD card root with a populated .habitink directory."""
    data = tmp_path / ".habitink"
    (data / "logs").mkdir(parents=True)
    (data / "habits.tsv").write_text(
        "# habitink habits v1\n"
        "1\twater\tdaily\tDrink water\n"
        "2\trun\t1111100\tMorning run\n"
        "3\tbook\tdaily\tRead\n",
        encoding="utf-8",
    )
    # Habit 1: done for the five days ending 2026-07-30.
    lines = [CompletionLog.header()]
    for day in range(26, 31):
        lines.append(CompletionLog.format_record(dt.date(2026, 7, day), True))
    (data / "logs" / "01.log").write_text("\n".join(lines) + "\n", encoding="utf-8")
    return tmp_path


@pytest.fixture
def store(sd: Path) -> DataStore:
    return DataStore.locate(sd)


@pytest.fixture
def config(store: DataStore) -> HabitConfig:
    return store.read_config()
