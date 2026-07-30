// The battery-minded wake/log/sleep controller.
//
// HabitInk's whole reason to exist is to wake, let you log in one press, and get
// back to sleep in seconds. This controller encodes that loop against the Hal
// interfaces, so it is exercised by native tests exactly as it runs on device.
#pragma once

#include "habitui/HabitApp.h"
#include "src/platform/Hal.h"

namespace habitink {

class HabitInkController {
 public:
  explicit HabitInkController(const Hal& hal, uint32_t idleSleepMs = 20000)
      : hal_(hal), app_(*hal.store, *hal.clock), idleSleepMs_(idleSleepMs) {}

  // Boot: load state and paint the first frame. Returns quickly so the device
  // is interactive fast.
  void begin();

  // One iteration of the main loop. Returns false when the controller has
  // decided to sleep (the caller then stops; on device deepSleep() has already
  // been requested and does not return).
  bool tick();

  // Test/debug accessors.
  habitui::HabitApp& app() { return app_; }
  bool sleeping() const { return sleeping_; }

 private:
  void repaint(bool full);
  void goToSleep();

  Hal hal_;
  habitui::HabitApp app_;
  uint32_t idleSleepMs_;
  uint32_t lastActivityMs_ = 0;
  bool sleeping_ = false;
};

}  // namespace habitink
