#include "src/HabitInkController.h"

#if defined(ARDUINO) && defined(HABITINK_DEBUG_SERIAL)
#include "src/platform/freeink/DebugSerial.h"
#define HABITINK_TRACE(what, arg) habitink::debugserial::trace(what, arg)
#else
#define HABITINK_TRACE(what, arg)
#endif

#include "habitui/FrameCanvas.h"

namespace habitink {

using habitui::AppButton;
using habitui::FrameCanvas;

void HabitInkController::repaint(bool full) {
  FrameCanvas canvas(hal_.display->framebuffer(), hal_.display->width(), hal_.display->height(),
                     hal_.display->stride());
  app_.render(canvas);
  hal_.display->flush(full);
}

void HabitInkController::begin() {
  app_.load();
  lastActivityMs_ = hal_.power->millis();
  // A cold boot needs a clean full refresh; a button wake can use a fast one.
  const bool full = hal_.power->wakeReason() != WakeReason::Button;
  repaint(full);
}

void HabitInkController::goToSleep() {
  // Paint the standby "sleep face" so the desk shows today's unfinished habits,
  // then power down. A full refresh leaves a crisp, ghost-free image.
  FrameCanvas canvas(hal_.display->framebuffer(), hal_.display->width(), hal_.display->height(),
                     hal_.display->stride());
  app_.renderSleepFace(canvas);
  hal_.display->flush(true);
  sleeping_ = true;
  hal_.power->deepSleep();  // does not return on device
}

bool HabitInkController::tick() {
  hal_.buttons->poll();

  bool activity = false;
  bool needRepaint = false;
  AppButton button;
  while (hal_.buttons->popButton(button)) {
    activity = true;
    HABITINK_TRACE("button", static_cast<int>(button));
    if (app_.handleButton(button)) needRepaint = true;
    HABITINK_TRACE("handled", needRepaint ? 1 : 0);
  }

  if (needRepaint) {
    HABITINK_TRACE("repaint-begin", 0);
    repaint(false);
    HABITINK_TRACE("repaint-end", 0);
  }
  if (activity) lastActivityMs_ = hal_.power->millis();

  // Manual sleep: holding power. Idle sleep: no input for idleSleepMs_.
  const uint32_t now = hal_.power->millis();
  const bool idleElapsed = (now - lastActivityMs_) >= idleSleepMs_;
  // Hold Back: hand the device back to the reader firmware in the other slot.
  if (hal_.buttons->exitHeld() && hal_.power->rebootToReader()) return false;

  const bool powerHeld = hal_.buttons->powerHeld();
  if (!powerHeld) powerArmed_ = true;
  if ((powerArmed_ && powerHeld) || idleElapsed) {
    goToSleep();
    return false;
  }
  return true;
}

}  // namespace habitink
