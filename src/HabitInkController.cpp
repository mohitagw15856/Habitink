#include "src/HabitInkController.h"

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
    if (app_.handleButton(button)) needRepaint = true;
  }

  if (needRepaint) repaint(false);
  if (activity) lastActivityMs_ = hal_.power->millis();

  // Manual sleep: holding power. Idle sleep: no input for idleSleepMs_.
  const uint32_t now = hal_.power->millis();
  const bool idleElapsed = (now - lastActivityMs_) >= idleSleepMs_;
  if (hal_.buttons->powerHeld() || idleElapsed) {
    goToSleep();
    return false;
  }
  return true;
}

}  // namespace habitink
