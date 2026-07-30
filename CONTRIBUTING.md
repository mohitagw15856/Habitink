# Contributing to HabitInk

Thanks for your interest. HabitInk is a small, single-purpose firmware, and the
aim is to keep it that way: fast to wake, tiny in RAM, easy to reason about.

## Principles

- **Keep logic portable.** New behaviour belongs in `lib/HabitCore` (pure logic)
  or `lib/HabitUI` (rendering and flow), behind the `Store` / `Clock` /
  `AppButton` and `Hal` interfaces, so it can be unit tested on the host. Only
  genuinely hardware-specific code goes in `src/platform/freeink`.
- **Mind the memory.** The device has about 380KB of RAM. Stream from the SD
  card, bound what you retain (see the day window in `CompletionLog`), avoid
  large heap allocations, and never buffer a whole log.
- **Do heavy work off-device.** Anything expensive (reports, images) belongs in
  the Python companion, not the firmware.
- **Match the two implementations.** The streak and grid rules exist in both C++
  (`lib/HabitCore`) and Python (`companion/habitink`). If you change one, change
  and test the other so they keep reporting the same numbers.

## Building and testing

```sh
# C++ unit tests
cmake -S test -B build/test -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/test
ctest --test-dir build/test --output-on-failure

# Host build of the firmware application layer
pio run -e native && .pio/build/native/program

# Companion tests
cd companion && pip install -e ".[test]" && pytest
```

Please add tests with any behaviour change. The C++ harness is the tiny one in
`test/check.h` (no external dependencies); register a case with `TEST(name)`.

## Generated assets

The embedded font (`lib/HabitUI/src/Font8x13.cpp`) and icons
(`lib/HabitUI/src/Icons.cpp`) are generated and checked in. Do not edit them by
hand; change the generator and rerun it:

```sh
python3 scripts/gen_font.py
python3 scripts/gen_icons.py
```

## Style

- C++17. Two-space indent, 120-column lines, matching `.clang-format` (run
  `clang-format` before committing).
- Python formatted for readability; keep functions small and typed.
- Documentation uses British English and avoids em dashes.

## Hardware-dependent work

Code that cannot be verified without the device is implemented as a best guess,
marked with a `TODO(hardware-test)` comment, and listed in
[docs/HARDWARE_TESTING.md](docs/HARDWARE_TESTING.md). If you verify or change one
of these, update that checklist.

## Commits and pull requests

Keep commits focused and messages descriptive. Explain the reasoning behind a
change, not just the what. By contributing you agree your work is licensed under
the project's MIT licence.
