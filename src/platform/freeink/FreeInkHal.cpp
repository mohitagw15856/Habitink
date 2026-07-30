#include "src/platform/freeink/FreeInkHal.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <HalDisplay.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>

namespace habitink {

// ---- Display -------------------------------------------------------------
// The panel is a single 1-bit framebuffer (800x480, 100-byte stride) in the
// SDK's convention: set bit = white, cleared bit = black. FrameCanvas already
// matches that, so we draw straight into it and flush.

void FreeInkDisplay::begin() { display.begin(); }

uint8_t* FreeInkDisplay::framebuffer() { return display.getFrameBuffer(); }
int FreeInkDisplay::width() const { return HalDisplay::DISPLAY_WIDTH; }
int FreeInkDisplay::height() const { return HalDisplay::DISPLAY_HEIGHT; }
int FreeInkDisplay::stride() const { return HalDisplay::DISPLAY_WIDTH_BYTES; }

void FreeInkDisplay::flush(bool full) {
  display.displayBuffer(full ? HalDisplay::FULL_REFRESH : HalDisplay::FAST_REFRESH);
}

// ---- Buttons -------------------------------------------------------------
// One-press logging: Down cycles habits, Confirm toggles today. Left/Right open
// the weekly and year views, Back returns.

void FreeInkButtons::poll() {
  gpio.update();
  if (gpio.wasReleased(HalGPIO::BTN_DOWN)) queue_.push_back(habitui::AppButton::Next);
  if (gpio.wasReleased(HalGPIO::BTN_UP)) queue_.push_back(habitui::AppButton::Prev);
  if (gpio.wasReleased(HalGPIO::BTN_CONFIRM)) queue_.push_back(habitui::AppButton::Toggle);
  if (gpio.wasReleased(HalGPIO::BTN_RIGHT)) queue_.push_back(habitui::AppButton::Grid);
  if (gpio.wasReleased(HalGPIO::BTN_LEFT)) queue_.push_back(habitui::AppButton::Weekly);
  if (gpio.wasReleased(HalGPIO::BTN_BACK)) queue_.push_back(habitui::AppButton::Back);
}

bool FreeInkButtons::popButton(habitui::AppButton& out) {
  if (queue_.empty()) return false;
  out = queue_.front();
  queue_.pop_front();
  return true;
}

bool FreeInkButtons::powerHeld() const {
  // A deliberate hold, not the wake tap.
  return gpio.getPowerButtonHeldTime() > 350;
}

// ---- Power ---------------------------------------------------------------

WakeReason FreeInkPower::wakeReason() const {
  switch (gpio.getWakeupReason()) {
    case HalGPIO::WakeupReason::PowerButton:
      return WakeReason::Button;
    case HalGPIO::WakeupReason::AfterFlash:
    case HalGPIO::WakeupReason::AfterUSBPower:
    case HalGPIO::WakeupReason::Other:
    default:
      return WakeReason::ColdBoot;
  }
}

uint32_t FreeInkPower::millis() const { return ::millis(); }

void FreeInkPower::deepSleep() {
  display.deepSleep();
  powerManager.startDeepSleep(gpio);  // does not return
}

}  // namespace habitink

#endif  // ARDUINO
