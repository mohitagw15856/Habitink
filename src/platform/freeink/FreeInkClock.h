// habitui::Clock backed by the freeink-sdk RTC.
//
// The ecosystem HalClock surfaces only hour/minute, so for the date HabitInk
// reads the underlying SDK Rtc::DateTime directly.
// TODO(hardware-test): confirm the Rtc::now(DateTime&) shape and field names on
// device (see docs/HARDWARE_TESTING.md).
#pragma once

#include "habitui/Env.h"

namespace habitink {

class FreeInkClock : public habitui::Clock {
 public:
  // utcOffsetMinutes shifts the RTC (kept in UTC) to local civil time.
  explicit FreeInkClock(int utcOffsetMinutes = 0) : offsetMinutes_(utcOffsetMinutes) {}

  habitcore::Date today() override;
  bool timeOfDay(uint8_t& hour, uint8_t& minute) override;

 private:
  int offsetMinutes_;
  // Fallback date if no RTC is present, so a device without a clock still runs
  // (all logging then lands on this date until the RTC is set).
  habitcore::Date fallback_{2026, 1, 1};
};

}  // namespace habitink
