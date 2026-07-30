# Architecture

HabitInk is a habit and streak tracker for the Xteink X4/X3 pocket e-reader
(ESP32-C3, roughly 380KB of usable RAM, a 4.2 inch 1-bit e-ink panel, SD card,
physical buttons). It is designed around one idea: because e-ink holds its image
with no power, the device can spend almost all of its life asleep showing you
today's unfinished habits, and a single wake, a single press and a return to
sleep should take under ten seconds.

## The big decision: fork, module or standalone?

Before writing any code we cloned and studied the dominant community firmware,
[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)
(MIT licensed, built on the separate `freeink-sdk` hardware SDK). CrossPoint is a
mature, memory-disciplined e-reader with an Android-style Activity stack, an
SD-first cache under `/.crosspoint`, a dedicated render task, and a careful deep
sleep and quick-resume flow. Three options were on the table:

- **(a) Fork CrossPoint.** Inherit the whole e-reader and bolt on habit
  screens. This drags in EPUB parsing, a TLS/Wi-Fi stack, font systems and much
  more, none of which a habit tracker needs. It works against the battery and
  tiny-state goals and leaves us maintaining a large tree we did not write.
- **(b) A module or app for upstream.** Ship HabitInk as an Activity that lives
  inside CrossPoint, or inside the CrossMux fork's "Apps hub". Cleaner than a
  fork, but it still boots the full reader firmware, and the reader's scope
  guardrails may not want a habit tracker upstream.
- **(c) A standalone firmware.** A single-purpose firmware for the same
  hardware, built on the same public `freeink-sdk`, sharing CrossPoint's
  architectural patterns (Activity-style flow, SD-first storage, deep-sleep
  wake/log/sleep loop) but none of its reader code.

**We chose (c), a standalone firmware.** The deciding factor is the product
brief: "keep total on-device state tiny; the most battery-friendly app possible;
wake, log, sleep in under ten seconds." A single-purpose firmware boots faster,
holds far less state, and has a far smaller attack and failure surface than a
reader carrying features HabitInk will never use. It reuses CrossPoint's ideas,
not its code, so there is no derived-work licensing burden; and where we do
depend on shared components (the `freeink-sdk` HAL, and the ecosystem's
`GfxRenderer`-style conventions) they are MIT, matching our own MIT licence.

The design keeps option (b) open at low cost: all screen logic is expressed as
plain functions over abstract interfaces (see below), so the same code could be
repackaged as a CrossMux app later with no change to the core.

### Licence compatibility

CrossPoint and `freeink-sdk` are MIT. HabitInk is MIT. No CrossPoint source is
copied into this repository. The embedded font is generated from Pillow's
built-in bitmap font (a permissive HPND-style licence) and checked in as our own
generated data. Everything is therefore MIT-compatible.

## Layered design

The firmware is split so that almost all of it is ordinary, portable C++17 that
compiles and is unit tested on a laptop, with only a thin adapter touching the
SDK. From the bottom up:

```
+--------------------------------------------------------------+
| src/main.cpp (Arduino entry, ARDUINO-guarded)                |
|   wires the freeink adapters to the controller               |
+--------------------------------------------------------------+
| src/platform/freeink/*  (device adapters, TODO(hardware-test))|
|   FreeInkStore / FreeInkClock / FreeInkDisplay / Buttons /...  |
+----------------------------+---------------------------------+
| src/HabitInkController      |  <- portable, unit tested        |
|   wake -> render -> log -> sleep, via the Hal interfaces      |
+----------------------------+---------------------------------+
| lib/HabitUI  (portable)     |  FrameCanvas 1-bit rasteriser,   |
|   HabitApp screens + flow   |  embedded font + icons, Env      |
+----------------------------+---------------------------------+
| lib/HabitCore (portable)    |  Date, Habit/Schedule, config,   |
|   pure logic, no I/O        |  append-only log, streaks, grids |
+--------------------------------------------------------------+
```

- **`lib/HabitCore`** is the hardware-free heart: date maths, the habit and
  schedule model, the `habits.tsv` parser, the append-only log folder, and all
  streak and grid computations. No allocations beyond small containers, no I/O.
  It is reused verbatim by the firmware and mirrored (not linked) by the Python
  companion so both report identical numbers.

- **`lib/HabitUI`** turns that data into pixels. `FrameCanvas` is a 1-bit
  software rasteriser that writes directly into the panel's framebuffer
  convention (a set bit is white, a cleared bit is black ink), so on device
  there is no second buffer and no copy. It ships an embedded 8x13 font and a
  16-icon set (both generated and checked in). `HabitApp` holds the tiny runtime
  state and renders the home, year-grid, weekly and sleep-face screens. It reads
  the world through three small interfaces in `Env.h` (`Store`, `Clock`,
  `AppButton`), so it can be driven by in-memory fakes in tests.

- **`src/HabitInkController`** is the wake/log/sleep loop, expressed against the
  `Hal` interfaces in `src/platform/Hal.h`. Because it never touches the SDK it
  is compiled and unit tested on the host exactly as it runs on device.

- **`src/platform/freeink`** is the only SDK-facing code: SD storage (with
  streamed, memory-bounded log reads), the RTC clock, the panel, the buttons and
  deep sleep. These adapters are small and are the items listed in
  [HARDWARE_TESTING.md](HARDWARE_TESTING.md). The raw freeink-sdk calls they make
  (Storage helpers, `HalFile` streaming, HalGPIO edges, panel flush, deep sleep)
  live in the shared [`inkkit`](https://github.com/mohitagw15856/inkkit) library,
  which HabitInk and InkCards both consume; the freeink adapters here are the thin
  HabitInk-specific layer (logical `AppButton` map, `/.habitink` paths, log
  framing) over it. inkkit is added only to the `xteink` (device) build via
  `lib_deps`, so the CI `native` build and host tests never pull it in.

## Runtime flow

```
wake (button or cold boot)
  -> Storage + display init
  -> HabitApp.load(): read habits.tsv, stream each log for "done today"
  -> paint home (full refresh on cold boot, fast on button wake)
loop:
  -> poll buttons -> AppButton events
  -> Down cycles habits, Confirm toggles today (append one log line)
  -> Right opens the year grid, Left the weekly view, Back returns
  -> repaint on change (fast refresh)
  -> power held, or idle timeout -> paint sleep face (full refresh) -> deep sleep
```

The only state kept in RAM between presses is the loaded config (at most twelve
short habits) plus one boolean per habit for "done today", and, lazily, a single
year-window log for the habit whose grid you are viewing. Everything else is
streamed from the SD card and dropped again. This is what keeps the wake-log-
sleep cycle fast and the battery cost negligible.

## Memory discipline

- Logs are read line by line through a callback sink, never buffered whole
  (`FreeInkStore::readLog`). `CompletionLog` keeps only records inside a day
  window, so history size does not affect RAM: the home check keeps one day, the
  weekly face seven, and the year grid a year.
- The framebuffer is drawn in place; there is no off-screen buffer.
- Only one habit's full log is ever resident at a time.
- Anything heavy (report generation, PNG rendering) is done on the companion,
  never on device.

## Rendering for e-ink

Everything is 1-bit. Filled cells, outlined cells, checkboxes and a monospaced
bitmap font give crisp, ghost-free images. A full refresh is used on entry and
for the sleep face (clean, no ghosting); fast refresh is used for interactive
repaints. The layout targets the panel's native 800x480 landscape orientation,
so `FrameCanvas` writes physical pixels with no rotation.

## Testing strategy

- `lib/HabitCore` and `lib/HabitUI` and `src/HabitInkController` are compiled
  and unit tested natively (CMake + a tiny zero-dependency harness, so CI needs
  no network fetch). See `test/`.
- The same firmware controller is also compiled by PlatformIO's `native`
  environment against a headless Hal, proving the on-device application layer
  builds and links with that toolchain too.
- The Python companion has full pytest coverage, including tests that mirror the
  C++ streak and grid scenarios so the two implementations stay in lockstep.
- The device (`xteink`) build and everything behind a `TODO(hardware-test)`
  marker is verified on real hardware, tracked in
  [HARDWARE_TESTING.md](HARDWARE_TESTING.md).
