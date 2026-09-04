// Bench-only USB serial diagnostics, compiled in by the xteink_x3_debug env
// (-DHABITINK_DEBUG_SERIAL). Prints the board/input profile inkkit detected
// at boot, then raw button state and ADC-ladder readings whenever they change,
// so a hardware session can tell "wrong board profile" from "ladder bands off"
// from "buttons fine, UI ignoring them". No-ops in production builds.
#pragma once

namespace habitink {
namespace debugserial {
void begin();  // call first thing in setup(); waits briefly for the host
void banner(); // call after gpio.begin()
void tick();   // call every loop()
void trace(const char* what, int arg);  // flushed breadcrumb around risky steps
}  // namespace debugserial
}  // namespace habitink
