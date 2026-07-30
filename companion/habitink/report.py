"""`habitink report`: a monthly Markdown report plus a shareable PNG grid."""
from __future__ import annotations

import calendar
import datetime as dt
from pathlib import Path

from . import stats
from .model import DataStore, Habit, HabitConfig
from .stats import BLANK, DONE, DUE, FUTURE, NOT_DUE

MONTH_NAMES = [
    "", "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December",
]


def _month_bounds(month: str) -> tuple[int, int]:
    try:
        year_s, mon_s = month.split("-")
        year, mon = int(year_s), int(mon_s)
    except ValueError as exc:
        raise ValueError(f"month must be YYYY-MM, got {month!r}") from exc
    if not (1 <= mon <= 12):
        raise ValueError(f"month out of range: {month!r}")
    return year, mon


def render_grid_png(config: HabitConfig, store: DataStore, today: dt.date, out_path: Path,
                    weeks: int = 53) -> None:
    """Renders every habit's year grid, stacked, into a single PNG."""
    from PIL import Image, ImageDraw  # local import so the CLI loads without Pillow

    cell = 10
    gap = 2
    step = cell + gap
    label_h = 18
    grid_w = weeks * step
    left = 12
    block_h = label_h + 7 * step + 14
    width = left + grid_w + 12
    height = 14 + block_h * max(1, len(config.habits))

    shades = {
        DONE: 0,          # black
        DUE: 205,         # light grey outline fill
        NOT_DUE: 240,     # barely there
        FUTURE: 255,      # white
        BLANK: 255,       # white
    }

    img = Image.new("L", (width, height), 255)
    draw = ImageDraw.Draw(img)

    y0 = 14
    for habit in config.habits:
        log = store.read_log(habit.id)
        grid = stats.build_year_grid(habit.schedule, log, today, weeks)
        streak = stats.compute_streak(habit.schedule, log, today)
        label = f"{habit.name}  -  streak {streak.current}, best {streak.longest}"
        draw.text((left, y0), label, fill=0)
        gy = y0 + label_h
        for w, col in enumerate(grid):
            for wd, state in enumerate(col):
                x = left + w * step
                y = gy + wd * step
                shade = shades[state]
                if state == DUE:
                    draw.rectangle([x, y, x + cell - 1, y + cell - 1], outline=150, fill=shade)
                elif state in (DONE,):
                    draw.rectangle([x, y, x + cell - 1, y + cell - 1], fill=shade)
                elif state == NOT_DUE:
                    draw.point((x + cell // 2, y + cell // 2), fill=180)
        y0 += block_h

    out_path.parent.mkdir(parents=True, exist_ok=True)
    img.save(out_path)


def build_markdown(config: HabitConfig, store: DataStore, year: int, month: int, today: dt.date,
                   png_name: str | None) -> str:
    days = stats.month_days(year, month)
    lines: list[str] = []
    lines.append(f"# HabitInk report: {MONTH_NAMES[month]} {year}")
    lines.append("")
    lines.append(f"Generated on {today.isoformat()}.")
    lines.append("")

    if png_name:
        lines.append(f"![Streak grids]({png_name})")
        lines.append("")

    lines.append("## Summary")
    lines.append("")
    lines.append("| Habit | Schedule | Done this month | Due this month | Rate | Current streak | Best streak |")
    lines.append("| --- | --- | --- | --- | --- | --- | --- |")
    for habit in config.habits:
        log = store.read_log(habit.id)
        due = [d for d in days if habit.schedule.is_due(d)]
        done = [d for d in due if log.is_done(d)]
        rate = (len(done) * 100 // len(due)) if due else 0
        streak = stats.compute_streak(habit.schedule, log, today)
        lines.append(
            f"| {habit.name} | {habit.schedule.to_field()} | {len(done)} | {len(due)} | "
            f"{rate}% | {streak.current} | {streak.longest} |"
        )
    lines.append("")

    lines.append("## Calendars")
    lines.append("")
    for habit in config.habits:
        log = store.read_log(habit.id)
        lines.append(f"### {habit.name}")
        lines.append("")
        lines.append(_month_calendar(habit, log, year, month, today))
        lines.append("")
    return "\n".join(lines)


def _month_calendar(habit: Habit, log, year: int, month: int, today: dt.date) -> str:
    """A small text calendar: x = done, . = missed, space = not due/future."""
    cal = calendar.Calendar(firstweekday=0)  # Monday
    rows = ["```", "Mo Tu We Th Fr Sa Su"]
    for week in cal.monthdatescalendar(year, month):
        cells = []
        for day in week:
            if day.month != month:
                cells.append("  ")
                continue
            if not habit.schedule.is_due(day):
                mark = " -"
            elif log.is_done(day):
                mark = " x"
            elif day > today:
                mark = "  "
            else:
                mark = " ."
            cells.append(mark)
        rows.append(" ".join(c.strip().rjust(2) for c in cells))
    rows.append("```")
    return "\n".join(rows)


def run_report(store: DataStore, month: str | None, out_md: Path, out_png: Path | None,
               today: dt.date | None = None) -> Path:
    today = today or dt.date.today()
    if month is None:
        month = f"{today.year:04d}-{today.month:02d}"
    year, mon = _month_bounds(month)
    config = store.read_config()

    png_name = None
    if out_png is not None:
        render_grid_png(config, store, today, out_png)
        # Reference the PNG relatively if it sits beside the report.
        try:
            png_name = str(out_png.relative_to(out_md.parent))
        except ValueError:
            png_name = out_png.name

    markdown = build_markdown(config, store, year, mon, today, png_name)
    out_md.parent.mkdir(parents=True, exist_ok=True)
    out_md.write_text(markdown, encoding="utf-8")
    return out_md
