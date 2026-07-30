#include "habitcore/Date.h"

#include <cstdio>

namespace habitcore {

// days_from_civil / civil_from_days: Howard Hinnant, "chrono-Compatible
// Low-Level Date Algorithms" (public domain). Valid for the full range of
// years we care about and correct across leap years without a lookup table.
int32_t Date::toDayNumber() const {
  const int y = (month <= 2) ? year - 1 : year;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = (153u * (month > 2 ? month - 3u : month + 9u) + 2u) / 5u + day - 1u;
  const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
  return era * 146097 + static_cast<int>(doe) - 719468;
}

Date Date::fromDayNumber(int32_t dayNumber) {
  int32_t z = dayNumber + 719468;
  const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = static_cast<unsigned>(z - era * 146097);
  const unsigned yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u;
  const int y = static_cast<int>(yoe) + era * 400;
  const unsigned doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
  const unsigned mp = (5u * doy + 2u) / 153u;
  const unsigned d = doy - (153u * mp + 2u) / 5u + 1u;
  const unsigned m = mp < 10u ? mp + 3u : mp - 9u;
  Date out;
  out.year = static_cast<int16_t>(m <= 2 ? y + 1 : y);
  out.month = static_cast<uint8_t>(m);
  out.day = static_cast<uint8_t>(d);
  return out;
}

Weekday Date::weekday() const {
  // 1970-01-01 was a Thursday. Shift so Monday is 0.
  int32_t dn = toDayNumber();
  int32_t idx = ((dn % 7) + 7) % 7;  // 0 = Thursday
  // Thursday(0) -> Weekday::Thu(3). Rotate: Monday-based = (idx + 3) % 7.
  int32_t mondayBased = (idx + 3) % 7;
  return static_cast<Weekday>(mondayBased);
}

std::string Date::toIso() const {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%04d-%02u-%02u", static_cast<int>(year), static_cast<unsigned>(month),
                static_cast<unsigned>(day));
  return std::string(buf);
}

bool Date::parseIso(const std::string& text, Date& out) {
  if (text.size() < 10) return false;
  // Expect exactly YYYY-MM-DD in the first 10 characters.
  for (size_t i = 0; i < 10; ++i) {
    const char c = text[i];
    const bool isDash = (i == 4 || i == 7);
    if (isDash) {
      if (c != '-') return false;
    } else if (c < '0' || c > '9') {
      return false;
    }
  }
  int y = 0, m = 0, d = 0;
  if (std::sscanf(text.c_str(), "%4d-%2d-%2d", &y, &m, &d) != 3) return false;
  if (m < 1 || m > 12 || d < 1 || d > 31) return false;
  if (!isReasonableYear(static_cast<int16_t>(y))) return false;
  out.year = static_cast<int16_t>(y);
  out.month = static_cast<uint8_t>(m);
  out.day = static_cast<uint8_t>(d);
  return true;
}

bool Date::isValid() const {
  if (!isReasonableYear(year)) return false;
  if (month < 1 || month > 12) return false;
  if (day < 1 || day > 31) return false;
  // Round trip through the day-number conversion: an out-of-range day such as
  // "2026-02-31" normalises to a different date, so a mismatch means invalid.
  return fromDayNumber(toDayNumber()) == *this;
}

}  // namespace habitcore
