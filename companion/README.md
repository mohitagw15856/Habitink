# HabitInk companion

A small Python CLI that reads the HabitInk data directory from your SD card and
helps you off the device: it produces shareable monthly reports and lets you
backfill days you forgot to log.

It reads exactly the format the firmware writes (see
[`../docs/FORMAT.md`](../docs/FORMAT.md)), so nothing has to be exported first.
Point it at the SD card root (or directly at the `.habitink` directory).

## Install

```sh
cd companion
pip install -e .
```

This installs a `habitink` command. Pillow is the only runtime dependency (used
to draw the PNG streak grid).

## `habitink report`

Produces a monthly Markdown report and a PNG of every habit's year grid.

```sh
habitink report --sd /Volumes/XTEINK --month 2026-07 \
    --out report.md --png grid.png
```

- `--sd` path to the SD card root or the `.habitink` directory.
- `--month` report month as `YYYY-MM` (defaults to the current month).
- `--out` Markdown output path.
- `--png` PNG output path, or `none` to skip the image.
- `--today` override today's date (`YYYY-MM-DD`), handy for reproducible output.

The report includes a per-habit summary table (completions, due days,
completion rate, current and best streak) and a text calendar per habit.

## `habitink edit`

Backfills or corrects a day. Edits are append-only, just like on the device, so
your log stays a faithful history.

```sh
# Mark a day you forgot
habitink edit --sd /Volumes/XTEINK --habit "Morning run" --date 2026-07-28 --done

# Clear a day logged by mistake
habitink edit --sd /Volumes/XTEINK --habit 1 --date 2026-07-30 --clear

# Backfill an inclusive range
habitink edit --sd /Volumes/XTEINK --habit 3 --date 2026-07-01..2026-07-07 --done
```

`--habit` accepts a numeric id or a habit name.

## Tests

```sh
pip install -e ".[test]"
pytest
```
