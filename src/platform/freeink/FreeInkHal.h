// freeink-sdk backed Display, Buttons and Power for the device build.
//
// TODO(hardware-test): every SDK call in the .cpp is modelled on the ecosystem
// HAL (HalDisplay / HalGPIO / HalPowerManager) and must be verified against the
// pinned freeink-sdk. See docs/HARDWARE_TESTING.md.
#pragma once

#include <deque>

#include "src/platform/Hal.h"

#ifdef ARDUINO

namespace habitink {

class FreeInkDisplay : public HalDisplay {
 public:
  void begin();
  uint8_t* framebuffer() override;
  int width() const override;
  int height() const override;
  int stride() const override;
  void flush(bool full) override;
};

class FreeInkButtons : public HalButtons {
 public:
  void poll() override;
  bool popButton(habitui::AppButton& out) override;
  bool powerHeld() const override;
  bool exitHeld() const override;

 private:
  std::deque<habitui::AppButton> queue_;
};

class FreeInkPower : public HalPower {
 public:
  WakeReason wakeReason() const override;
  uint32_t millis() const override;
  void deepSleep() override;
  bool rebootToReader() override;
};

}  // namespace habitink

#endif  // ARDUINO
