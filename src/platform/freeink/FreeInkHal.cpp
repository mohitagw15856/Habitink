#include "src/platform/freeink/FreeInkHal.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <HalDisplay.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <inkkit/Buttons.h>
#include <inkkit/Display.h>
#include <inkkit/Power.h>

namespace habitink {

// The SDK singletons (display, gpio, powerManager) are declared by the headers
// above; inkkit's thin wrappers hold references to them, so we construct a
// wrapper around the relevant singleton per call. All the raw SDK calls now live
// once in inkkit rather than being re-derived here and in InkCards.

// ---- Display -------------------------------------------------------------
// The panel is a single 1-bit framebuffer (800x480, 100-byte stride) in the
// SDK's convention: set bit = white, cleared bit = black. FrameCanvas already
// matches that, so we draw straight into it and flush.

void FreeInkDisplay::begin() { inkkit::Display(display).begin(); }

uint8_t* FreeInkDisplay::framebuffer() { return inkkit::Display(display).framebuffer(); }
int FreeInkDisplay::width() const { return inkkit::Display(display).width(); }
int FreeInkDisplay::height() const { return inkkit::Display(display).height(); }
int FreeInkDisplay::stride() const { return inkkit::Display(display).stride(); }

void FreeInkDisplay::flush(bool full) { inkkit::Display(display).flush(full); }

// ---- Buttons -------------------------------------------------------------
// One-press logging: Down cycles habits, Confirm toggles today. Left/Right open
// the weekly and year views, Back returns.

void FreeInkButtons::poll() {
  inkkit::Buttons btn(gpio);
  btn.update();
  if (btn.wasReleased(HalGPIO::BTN_DOWN)) queue_.push_back(habitui::AppButton::Next);
  if (btn.wasReleased(HalGPIO::BTN_UP)) queue_.push_back(habitui::AppButton::Prev);
  if (btn.wasReleased(HalGPIO::BTN_CONFIRM)) queue_.push_back(habitui::AppButton::Toggle);
  if (btn.wasReleased(HalGPIO::BTN_RIGHT)) queue_.push_back(habitui::AppButton::Grid);
  if (btn.wasReleased(HalGPIO::BTN_LEFT)) queue_.push_back(habitui::AppButton::Weekly);
  if (btn.wasReleased(HalGPIO::BTN_BACK)) queue_.push_back(habitui::AppButton::Back);
}

bool FreeInkButtons::popButton(habitui::AppButton& out) {
  if (queue_.empty()) return false;
  out = queue_.front();
  queue_.pop_front();
  return true;
}

bool FreeInkButtons::exitHeld() const {
  return gpio.isPressed(HalGPIO::BTN_BACK) && gpio.getHeldTime() > 1500;
}

bool FreeInkButtons::powerHeld() const {
  // A deliberate hold, not the wake tap.
  return inkkit::Buttons(gpio).powerHeldMs() > 350;
}

// ---- Power ---------------------------------------------------------------

WakeReason FreeInkPower::wakeReason() const {
  switch (inkkit::Buttons(gpio).wakeCause()) {
    case inkkit::WakeCause::Button:
      return WakeReason::Button;
    case inkkit::WakeCause::Timer:
      return WakeReason::Timer;
    case inkkit::WakeCause::ColdBoot:
    default:
      return WakeReason::ColdBoot;
  }
}

uint32_t FreeInkPower::millis() const { return inkkit::Power(powerManager, display, gpio).millis(); }

void FreeInkPower::deepSleep() {
  inkkit::Power(powerManager, display, gpio).deepSleep();  // does not return
}

}  // namespace habitink

#endif  // ARDUINO

namespace habitink {
bool FreeInkPower::rebootToReader() {
  return inkkit::Power(powerManager, display, gpio).rebootIntoOtherFirmware();
}
}  // namespace habitink
