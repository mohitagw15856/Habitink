"""Data model shared by the HabitInk companion commands.

This mirrors the on-device format documented in docs/FORMAT.md so the companion
reads exactly what the firmware writes: a habits.tsv definition file and one
append-only log per habit under an .habitink directory on the SD card.
"""
from __future__ import annotations

import datetime as dt
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

MAX_HABITS = 12

# Canonical icon tokens, matching habitcore::IconId order.
ICON_TOKENS = [
    "check", "water", "run", "book", "pill", "meditate", "dumbbell", "sun",
    "moon", "heart", "food", "pen", "code", "music", "leaf", "star",
]

WEEKDAY_INITIALS = ["M", "T", "W", "T", "F", "S", "S"]


class FormatError(ValueError):
    """Raised when a config or log file is malformed beyond recovery."""


@dataclass
class Schedule:
    """Seven-day weekday mask, Monday first (matching Python's date.weekday())."""

    mask: int = 0x7F  # bit i set => due on weekday i (Mon=0 .. Sun=6)

    @classmethod
    def daily(cls) -> "Schedule":
        return cls(0x7F)

    @classmethod
    def parse(cls, text: str) -> "Schedule":
        text = text.strip()
        if text.lower() == "daily":
            return cls(0x7F)
        if len(text) != 7 or any(c not in "01" for c in text):
            raise FormatError(f"invalid schedule {text!r}")
        mask = 0
        for i, c in enumerate(text):
            if c == "1":
                mask |= 1 << i
        return cls(mask)

    def to_field(self) -> str:
        if self.mask & 0x7F == 0x7F:
            return "daily"
        return "".join("1" if (self.mask >> i) & 1 else "0" for i in range(7))

    def is_due(self, day: dt.date) -> bool:
        return bool((self.mask >> day.weekday()) & 1)

    def due_days_per_week(self) -> int:
        return bin(self.mask & 0x7F).count("1")


@dataclass
class Habit:
    id: int
    icon: str
    schedule: Schedule
    name: str


@dataclass
class HabitConfig:
    habits: list[Habit] = field(default_factory=list)

    def by_id(self, habit_id: int) -> Habit | None:
        for h in self.habits:
            if h.id == habit_id:
                return h
        return None

    def by_name(self, name: str) -> Habit | None:
        for h in self.habits:
            if h.name.lower() == name.lower():
                return h
        return None

    @classmethod
    def parse(cls, text: str) -> "HabitConfig":
        cfg = cls()
        seen: set[int] = set()
        for lineno, raw in enumerate(text.splitlines(), start=1):
            line = raw.rstrip()
            if not line or line.startswith("#"):
                continue
            parts = line.split("\t", 3)
            if len(parts) != 4:
                raise FormatError(f"line {lineno}: expected four tab-separated fields")
            id_text, icon, sched_text, name = parts
            try:
                habit_id = int(id_text)
            except ValueError as exc:
                raise FormatError(f"line {lineno}: bad id {id_text!r}") from exc
            if not (1 <= habit_id <= MAX_HABITS):
                raise FormatError(f"line {lineno}: id {habit_id} out of range 1..{MAX_HABITS}")
            if habit_id in seen:
                raise FormatError(f"line {lineno}: duplicate id {habit_id}")
            if not name:
                raise FormatError(f"line {lineno}: empty name")
            seen.add(habit_id)
            cfg.habits.append(Habit(habit_id, icon, Schedule.parse(sched_text), name))
            if len(cfg.habits) >= MAX_HABITS:
                break
        return cfg

    def serialise(self) -> str:
        out = ["# habitink habits v1", "# id\ticon\tschedule\tname"]
        for h in self.habits:
            out.append(f"{h.id}\t{h.icon or 'check'}\t{h.schedule.to_field()}\t{h.name}")
        return "\n".join(out) + "\n"


class CompletionLog:
    """Effective per-day completion state from an append-only log."""

    def __init__(self) -> None:
        self._state: dict[dt.date, bool] = {}

    def apply_line(self, line: str) -> bool:
        line = line.strip()
        if not line or line.startswith("#"):
            return False
        if "\t" in line:
            date_text, _, value_text = line.partition("\t")
        else:
            date_text, _, value_text = line.partition(" ")
        date_text, value_text = date_text.strip(), value_text.strip()
        if not value_text or value_text[0] not in "01":
            return False
        try:
            date = dt.date.fromisoformat(date_text)
        except ValueError:
            return False
        self._state[date] = value_text[0] == "1"
        return True

    def apply_record(self, date: dt.date, done: bool) -> None:
        self._state[date] = done

    def is_done(self, date: dt.date) -> bool:
        return self._state.get(date, False)

    def done_dates(self) -> list[dt.date]:
        return sorted(d for d, done in self._state.items() if done)

    def first_recorded(self) -> dt.date | None:
        return min(self._state) if self._state else None

    @staticmethod
    def format_record(date: dt.date, done: bool) -> str:
        return f"{date.isoformat()}\t{'1' if done else '0'}"

    @staticmethod
    def header() -> str:
        return "# habitink log v1"


@dataclass
class DataStore:
    """Locates and reads/writes the .habitink directory on an SD card root."""

    root: Path  # the .habitink directory itself

    @classmethod
    def locate(cls, path: str | Path) -> "DataStore":
        p = Path(path)
        if p.name == ".habitink":
            return cls(p)
        candidate = p / ".habitink"
        if candidate.exists():
            return cls(candidate)
        # Fall back to treating the given path as the data dir directly.
        return cls(candidate)

    @property
    def config_path(self) -> Path:
        return self.root / "habits.tsv"

    def log_path(self, habit_id: int) -> Path:
        return self.root / "logs" / f"{habit_id:02d}.log"

    def read_config(self) -> HabitConfig:
        if not self.config_path.exists():
            raise FileNotFoundError(f"no habits.tsv at {self.config_path}")
        return HabitConfig.parse(self.config_path.read_text(encoding="utf-8"))

    def write_config(self, config: HabitConfig) -> None:
        self.config_path.parent.mkdir(parents=True, exist_ok=True)
        self.config_path.write_text(config.serialise(), encoding="utf-8")

    def read_log(self, habit_id: int) -> CompletionLog:
        log = CompletionLog()
        path = self.log_path(habit_id)
        if path.exists():
            for line in path.read_text(encoding="utf-8").splitlines():
                log.apply_line(line)
        return log

    def append_log(self, habit_id: int, records: Iterable[tuple[dt.date, bool]]) -> None:
        path = self.log_path(habit_id)
        path.parent.mkdir(parents=True, exist_ok=True)
        new_file = not path.exists()
        with path.open("a", encoding="utf-8") as fh:
            if new_file:
                fh.write(CompletionLog.header() + "\n")
            for date, done in records:
                fh.write(CompletionLog.format_record(date, done) + "\n")
