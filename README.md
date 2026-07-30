# HabitInk

A habit and streak tracker for the [Xteink](https://en.wikipedia.org/wiki/E-reader)
X4/X3 pocket e-reader that turns the always-on nature of e-ink to your advantage:
the device sleeps almost all the time showing today's unfinished habits, and one
wake, one press and back to sleep takes under ten seconds.

- **One-press logging.** From the home screen, cycle habits with one button and
  toggle today's completion with another.
- **Streak grids.** A GitHub-contributions-style year grid per habit, plus an
  all-habits weekly view, drawn crisply for 1-bit e-ink.
- **A sleep face that nags you.** The standby screen is today's checklist, so
  unfinished habits are literally staring at you from the desk.
- **Tiny state, long battery.** Habits and an append-only log live on the SD
  card; only a few bytes of state stay in RAM between presses.
- **A companion CLI.** Generate monthly reports and shareable streak images, and
  backfill days you forgot, from your computer.

HabitInk is a standalone firmware. It is not a fork of a reader: it shares the
architecture and hardware SDK of the [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)
ecosystem but carries none of its reader code, which is what keeps it small and
battery-friendly. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full
reasoning.

## Screenshots

Rendered from the actual firmware renderer at the panel's 800x480 resolution.

| Home | Year grid |
| --- | --- |
| ![Home screen](docs/images/preview_home.png) | ![Year grid](docs/images/preview_grid.png) |

| Weekly view | Sleep face |
| --- | --- |
| ![Weekly view](docs/images/preview_weekly.png) | ![Sleep face](docs/images/preview_sleep.png) |

_Photographs of the firmware running on real hardware will go here once the
device build is verified (see [docs/HARDWARE_TESTING.md](docs/HARDWARE_TESTING.md))._

## Compatibility

Works with **CrossPoint v1.5.0**-class devices: HabitInk targets the same Xteink
X4/X3 hardware and the same `freeink-sdk` generation that CrossPoint v1.5.0
builds against. It stores its data in its own `/.habitink` directory and never
touches CrossPoint's `/.crosspoint`, so the two can share an SD card. They are
separate firmwares, so you flash one at a time.

## Install and flash

HabitInk builds with [PlatformIO](https://platformio.org/).

```sh
# Host build of the portable application layer (no hardware or SDK needed)
pio run -e native && .pio/build/native/program

# Device build for the Xteink X4/X3 (ESP32-C3)
pio run -e xteink
pio run -e xteink -t upload   # flash over USB
pio device monitor            # serial log at 115200
```

The device (`xteink`) build needs the `freeink-sdk` and the ecosystem HAL wrapper
layer; see [docs/HARDWARE_TESTING.md](docs/HARDWARE_TESTING.md) for how to wire
them in and the on-device verification checklist.

### First run

Create `/.habitink/habits.tsv` on the SD card (or let the companion write it):

```
# habitink habits v1
1	water	daily	Drink 2L water
2	run	1111100	Morning run
3	book	0000011	Weekend reading
```

The full format is in [docs/FORMAT.md](docs/FORMAT.md).

## Companion tool

A Python CLI reads the SD card and helps you off the device.

```sh
cd companion && pip install -e .

# Monthly Markdown report plus a PNG of every habit's year grid
habitink report --sd /path/to/sdcard --month 2026-07 --out report.md --png grid.png

# Backfill a day you forgot (append-only, just like the device)
habitink edit --sd /path/to/sdcard --habit "Morning run" --date 2026-07-28 --done
```

See [companion/README.md](companion/README.md) for details.

## Build and test

```sh
# Native C++ unit tests (core logic, renderer, firmware controller)
cmake -S test -B build/test -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/test
ctest --test-dir build/test --output-on-failure

# Companion tests
cd companion && pip install -e ".[test]" && pytest
```

The embedded font and icons are generated and checked in; regenerate them with
`python3 scripts/gen_font.py` and `python3 scripts/gen_icons.py`.

## Repository layout

```
lib/HabitCore    portable habit logic (dates, config, log, streaks, grids)
lib/HabitUI      portable 1-bit renderer, embedded assets, screens and flow
src/             firmware: controller + freeink-sdk adapters + Arduino entry
companion/       Python CLI (report, edit) with pytest
test/            native C++ tests
docs/            architecture, data format, hardware testing checklist
scripts/         asset generators and the screen preview renderer
```

## Documentation

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - design and the fork/module/standalone decision
- [docs/FORMAT.md](docs/FORMAT.md) - the on-disk data format
- [docs/HARDWARE_TESTING.md](docs/HARDWARE_TESTING.md) - on-device verification checklist
- [CONTRIBUTING.md](CONTRIBUTING.md) - how to contribute

## Licence

MIT. See [LICENSE](LICENSE). HabitInk reuses the CrossPoint / `freeink-sdk`
ecosystem's ideas and MIT-licensed conventions but contains no copied CrossPoint
source.
