// Hardware abstraction the firmware controller runs against.
//
// These interfaces are the only seam between HabitInk's portable logic and the
// freeink-sdk. Keeping them abstract means the whole wake/log/sleep controller
// compiles and is unit tested on the host against a mock, and the on-device
// implementation (src/platform/freeink) is a thin, reviewable adapter. Nothing
// here includes Arduino or the SDK.
#pragma once

#include <cstdint>

#include "habitui/Env.h"

namespace habitink {

// How the device came out of sleep, so the controller can decide whether to
// repaint fully or trust the retained e-ink frame.
enum class WakeReason : uint8_t {
  ColdBoot,     // power on / flash: full repaint
  Button,       // woken by a button: interactive session
  Timer,        // periodic wake (unused today, reserved)
};

// The panel: a 1-bit framebuffer the controller draws into via FrameCanvas.
class HalDisplay {
 public:
  virtual ~HalDisplay() = default;
  virtual uint8_t* framebuffer() = 0;
  virtual int width() const = 0;
  virtual int height() const = 0;
  virtual int stride() const = 0;
  // Push the framebuffer to the panel. full == true requests a clean full
  // refresh (used on entry and for the sleep face); otherwise a fast refresh.
  virtual void flush(bool full) = 0;
};

// Physical buttons, already debounced by the SDK, delivered as logical
// AppButtons. popButton drains one queued edge per call.
class HalButtons {
 public:
  virtual ~HalButtons() = default;
  virtual void poll() = 0;  // sample hardware once per loop
  virtual bool popButton(habitui::AppButton& out) = 0;
  // True while the power button is being held (for manual sleep).
  virtual bool powerHeld() const = 0;
};

// Power / sleep control.
class HalPower {
 public:
  virtual ~HalPower() = default;
  virtual WakeReason wakeReason() const = 0;
  // Milliseconds since boot; used for the idle sleep timeout.
  virtual uint32_t millis() const = 0;
  // Persist nothing here; the controller has already flushed the sleep face.
  // Does not return: the device powers down until the next wake.
  virtual void deepSleep() = 0;
};

// Bundle passed to the controller.
struct Hal {
  HalDisplay* display;
  HalButtons* buttons;
  HalPower* power;
  habitui::Store* store;
  habitui::Clock* clock;
};

}  // namespace habitink
