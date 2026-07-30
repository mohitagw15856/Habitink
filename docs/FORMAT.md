# On-disk data format

All HabitInk data lives in a single directory, `.habitink`, at the root of the
SD card. The format is deliberately plain text: line oriented, tab separated,
and safe to read, diff and hand edit. The companion tool reads and writes
exactly these files, so nothing has to be exported.

```
/.habitink/
  habits.tsv        habit definitions
  logs/
    01.log          append-only completion log for habit id 1
    02.log          ... one file per habit id, zero padded to two digits
```

Text encoding is UTF-8. Line endings may be LF or CRLF on read; the device
writes LF. Lines beginning with `#` are comments; blank lines are ignored.

## habits.tsv

Defines up to twelve habits, one per line, with four tab-separated fields.

```
# habitink habits v1
# id  icon  schedule  name
1	water	daily	Drink 2L water
2	run	1111100	Morning run
3	book	0000011	Weekend reading
```

| Field | Meaning |
| --- | --- |
| `id` | Whole number 1 to 12. Stable identity; it names the log file. |
| `icon` | A token from the built-in icon set (see below). Unknown tokens fall back to `check`. |
| `schedule` | `daily`, or seven characters of `0`/`1`, Monday first (see below). |
| `name` | Display name. May contain spaces but not tab characters. |

Rules enforced on parse: ids must be unique and in range; the name must not be
empty; at most twelve habits are kept (extra lines are ignored). A malformed
line is skipped and reported rather than aborting the whole file, so one bad
edit never loses your other habits.

### Schedule field

`daily` is shorthand for "due every day". Otherwise the field is exactly seven
characters, one per weekday starting on **Monday**, where `1` means the habit is
due that day and `0` means it is not:

```
Mon Tue Wed Thu Fri Sat Sun
 1   1   1   1   1   0   0     -> "1111100"  (weekdays only)
 0   0   0   0   0   1   1     -> "0000011"  (weekends only)
 1   0   1   0   1   0   0     -> "1010100"  (Mon, Wed, Fri)
```

### Icon tokens

`check`, `water`, `run`, `book`, `pill`, `meditate`, `dumbbell`, `sun`, `moon`,
`heart`, `food`, `pen`, `code`, `music`, `leaf`, `star`.

## logs/NN.log

One append-only log per habit, named by the zero-padded habit id (`01.log` ..
`12.log`). Each data line is a date and a value, tab separated:

```
# habitink log v1
2026-07-28	1
2026-07-29	1
2026-07-30	1
2026-07-30	0
2026-07-30	1
```

- The date is ISO 8601 `YYYY-MM-DD`.
- The value is `1` (completed) or `0` (cleared).

### Append-only semantics

The log is **never rewritten**. Toggling a day, or correcting it later, appends
a new line. The effective state of a date is the value of the **last** line
naming that date (last write wins). In the example above, 2026-07-30 was marked
done, then cleared, then done again, so its effective state is done.

This is what makes logging cheap and crash-safe on device: a toggle is a single
short append with no read-modify-write of existing data, and a power loss
mid-write at worst drops the last line. It also keeps a faithful history: you can
see not just the final state but when it changed.

A day with no line at all is "not completed". Days before a habit's first
recorded line are treated as "before the habit existed" and render blank in the
grid (not as missed), so a new habit does not start life looking like a wall of
failures.

### Growth and compaction

A line is about 13 bytes, and in normal use you write one or two per habit per
day, so a year is a few kilobytes: small enough to stream and fold on device
within the wake budget. If a log ever grows large through heavy re-toggling, the
companion can rewrite it to one line per day without changing any effective
state; the device never needs to.

## Streak and grid rules

These are defined once in `lib/HabitCore` and mirrored in the companion.

- **Due day**: a day whose weekday is in the habit's schedule (`daily` means
  every day).
- **Current streak**: consecutive completed due days counting back from today.
  Non-due days are skipped and never break a streak. Today is given grace: if
  today is due but not yet logged, the streak is not considered broken; missing
  any earlier due day ends it.
- **Best streak**: the longest run of consecutive completed due days in history.
- **Completion rate**: completed due days divided by due days since the habit's
  first record.
