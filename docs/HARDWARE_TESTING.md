# Hardware testing checklist

Most of HabitInk is portable and is tested automatically on the host (see
[ARCHITECTURE.md](ARCHITECTURE.md)). The parts that genuinely cannot be verified
without an Xteink X4/X3 are implemented against the inkkit device layer and
marked in the source with `TODO(hardware-test)`. This page lists every one so
they can be confirmed on device.

## How to build and flash for hardware

The device firmware is the `xteink_x4` / `xteink_x3` PlatformIO environments
(identical firmware; the device is detected at runtime). The complete device
layer (`HalDisplay`, `HalStorage`, `HalGPIO`, `Rtc`, `BoardConfig` and the SDK
hardware libraries) comes from
[inkkit](https://github.com/mohitagw15856/inkkit), already pinned in
`platformio.ini`. Then:

```sh
pio run -e xteink_x4            # build
pio run -e xteink_x4 -t upload  # flash over USB
pio device monitor           # serial log at 115200
```

The `native` environment (the default, and what CI builds) needs none of that
and compiles the whole portable application layer on any host:

```sh
pio run -e native && .pio/build/native/program
```

## Items to verify on device

Each item is a `TODO(hardware-test)` in the source.

### Boot and bring-up (`src/main.cpp`)
- Confirm the bring-up ordering. We call `gpio.begin()` (SPI/button setup and
  X4-vs-X3 detection), then `Storage.begin()`, `display.begin()` and
  `powerManager.begin()`, following the CrossPoint boot order; verify this is
  right for a from-cold and a from-deep-sleep wake.
- Confirm the wake path shows the home screen quickly and that a button wake
  uses a fast refresh while a cold boot uses a full refresh.

### SD storage (`src/platform/freeink/FreeInkStore.cpp`)
- Confirm `Storage.ensureDirectoryExists` creates parent directories (so
  `/.habitink/logs` is made in one call chain).
- Confirm `Storage.open(path, O_WRONLY | O_CREAT | O_APPEND)` appends rather
  than truncates; if not, adjust to the correct append flag or helper.
- Confirm streamed reads through `HalFile::read` return the whole file and that
  the line splitter handles the final line without a trailing newline.

### RTC clock (`src/platform/freeink/FreeInkClock.cpp`)
- Confirm the `Rtc` API: `begin()`, `now(Rtc::DateTime&)`, and the field names
  `year`, `month`, `day`, `hour`, `minute`. Verify the RTC is read in UTC and
  that the configured UTC offset produces the correct local date around
  midnight.
- Decide how the UTC offset is configured (currently a constructor argument in
  `src/main.cpp`); a small settings file on the SD card is the likely home.

### Display (`src/platform/freeink/FreeInkHal.cpp`)
- Confirm `display.getFrameBuffer()` returns a 800x480, 100-byte-stride, 1-bit
  buffer in the "set bit = white" convention `FrameCanvas` assumes. If the panel
  is wired in the rotated portrait orientation instead, either set the SDK
  orientation to native landscape or adjust the canvas dimensions.
- Confirm `FULL_REFRESH` and `FAST_REFRESH` behave as expected (no ghosting on
  the sleep face; fast, clean interactive repaints).

### Buttons (`src/platform/freeink/FreeInkHal.cpp`)
- Confirm the physical button constants (`BTN_DOWN`, `BTN_UP`, `BTN_CONFIRM`,
  `BTN_LEFT`, `BTN_RIGHT`, `BTN_BACK`) and that `wasReleased` gives one event
  per press. Confirm the mapping feels right in the hand (Down cycles, Confirm
  toggles) and adjust to taste.
- Confirm `getPowerButtonHeldTime()` and the 350ms hold threshold distinguish a
  deliberate sleep hold from the wake tap.

### Power and sleep (`src/platform/freeink/FreeInkHal.cpp`)
- Confirm `powerManager.startDeepSleep(gpio)` powers down and that a button wake
  returns through `getWakeupReason()` as `PowerButton`.
- Measure the wake-to-interactive and total wake-log-sleep times against the
  under-ten-seconds goal, and measure sleep current draw.

## Acceptance targets
- Wake, cycle to a habit, toggle it done, and sleep in under ten seconds.
- The sleep face shows today's checklist with completed items struck through and
  an "N of M done" footer.
- A toggle survives a reboot (the log is read back correctly).
- The year grid and weekly view match the companion's report for the same data.
