#include "src/platform/freeink/FreeInkClock.h"

#ifdef ARDUINO

#include <Rtc.h>

namespace habitink {

namespace {
Rtc g_rtc;
bool g_begun = false;

bool ensureRtc() {
  if (!g_begun) {
    g_begun = g_rtc.begin();
  }
  return g_begun;
}
}  // namespace

habitcore::Date FreeInkClock::today() {
  if (!ensureRtc()) return fallback_;
  Rtc::DateTime dt;
  if (!g_rtc.now(dt)) return fallback_;
  // Apply the UTC offset via day-number arithmetic so day/month/year roll over
  // correctly around midnight.
  habitcore::Date d(static_cast<int16_t>(dt.year), static_cast<uint8_t>(dt.month), static_cast<uint8_t>(dt.day));
  const int totalMinutes = dt.hour * 60 + dt.minute + offsetMinutes_;
  int dayShift = 0;
  int m = totalMinutes;
  while (m < 0) {
    m += 1440;
    --dayShift;
  }
  while (m >= 1440) {
    m -= 1440;
    ++dayShift;
  }
  return d.addDays(dayShift);
}

bool FreeInkClock::timeOfDay(uint8_t& hour, uint8_t& minute) {
  if (!ensureRtc()) return false;
  Rtc::DateTime dt;
  if (!g_rtc.now(dt)) return false;
  int totalMinutes = dt.hour * 60 + dt.minute + offsetMinutes_;
  totalMinutes = ((totalMinutes % 1440) + 1440) % 1440;
  hour = static_cast<uint8_t>(totalMinutes / 60);
  minute = static_cast<uint8_t>(totalMinutes % 60);
  return true;
}

}  // namespace habitink

#endif  // ARDUINO
