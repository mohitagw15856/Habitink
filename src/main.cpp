// HabitInk firmware entry point for the Xteink X4/X3 (ESP32-C3).
//
// This is a deliberately thin Arduino shim: it wires the freeink-sdk backed Hal
// to the portable HabitInkController (tested natively) and runs the wake, log,
// sleep loop. All real behaviour lives in lib/HabitCore, lib/HabitUI and
// src/HabitInkController.cpp.
//
// TODO(hardware-test): boot ordering and the SDK init calls below follow the
// ecosystem pattern (see docs/ARCHITECTURE.md) and must be verified on device.
#ifdef ARDUINO

#include <Arduino.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <HalStorage.h>

#include "src/HabitInkController.h"
#include "src/platform/freeink/FreeInkClock.h"
#include "src/platform/freeink/FreeInkHal.h"
#include "src/platform/freeink/FreeInkStore.h"

namespace {
habitink::FreeInkDisplay g_display;
habitink::FreeInkButtons g_buttons;
habitink::FreeInkPower g_power;
habitink::FreeInkStore g_store("/.habitink");
// UTC offset in minutes; adjust to your timezone or wire to a settings file.
habitink::FreeInkClock g_clock(0);

habitink::HabitInkController* g_controller = nullptr;
bool g_running = true;
}  // namespace

void setup() {
  // Bring up the board, storage and display, then hand off to the controller.
  // gpio.begin() runs SPI/button setup and X4-vs-X3 detection; it must precede
  // storage and display bring-up (CrossPoint boot order).
  // TODO(hardware-test): verify the boot order on device.
  gpio.begin();
  Storage.begin();
  g_display.begin();
  powerManager.begin();

  habitink::Hal hal;
  hal.display = &g_display;
  hal.buttons = &g_buttons;
  hal.power = &g_power;
  hal.store = &g_store;
  hal.clock = &g_clock;

  static habitink::HabitInkController controller(hal);
  g_controller = &controller;
  g_controller->begin();
}

void loop() {
  if (!g_running || g_controller == nullptr) {
    delay(50);
    return;
  }
  g_running = g_controller->tick();
  // Light idle delay keeps input responsive while sipping power; the controller
  // itself decides when to deep sleep (which does not return).
  delay(20);
}

#endif  // ARDUINO
