// Portable proleptic-Gregorian date maths for HabitInk.
//
// No hardware, no allocations, no exceptions. A Date is a plain year/month/day
// triple; conversions to and from a day number use Howard Hinnant's civil
// algorithms so weekday and streak arithmetic are exact and branch cheap. The
// day number is days since 1970-01-01 (the Unix epoch day), which keeps it
// compatible with the companion tool and any RTC that reports civil time.
#pragma once

#include <cstdint>
#include <string>

namespace habitcore {

// Weekday index used throughout HabitInk: Monday is 0, Sunday is 6. This
// matches the schedule mask layout documented in docs/FORMAT.md.
enum class Weekday : uint8_t { Mon = 0, Tue, Wed, Thu, Fri, Sat, Sun };

struct Date {
  int16_t year = 1970;
  uint8_t month = 1;  // 1..12
  uint8_t day = 1;    // 1..31

  constexpr Date() = default;
  constexpr Date(int16_t y, uint8_t m, uint8_t d) : year(y), month(m), day(d) {}

  bool operator==(const Date& o) const { return year == o.year && month == o.month && day == o.day; }
  bool operator!=(const Date& o) const { return !(*this == o); }
  bool operator<(const Date& o) const { return toDayNumber() < o.toDayNumber(); }
  bool operator<=(const Date& o) const { return toDayNumber() <= o.toDayNumber(); }
  bool operator>(const Date& o) const { return toDayNumber() > o.toDayNumber(); }
  bool operator>=(const Date& o) const { return toDayNumber() >= o.toDayNumber(); }

  // Days since 1970-01-01 (may be negative for earlier dates).
  int32_t toDayNumber() const;
  static Date fromDayNumber(int32_t dayNumber);

  Weekday weekday() const;

  // Returns a new date offset by the given number of days.
  Date addDays(int32_t delta) const { return fromDayNumber(toDayNumber() + delta); }

  // ISO 8601 "YYYY-MM-DD". Always zero padded.
  std::string toIso() const;
  // Parses "YYYY-MM-DD". Returns false and leaves out untouched on malformed
  // input. Does not validate that the day exists in the month beyond range
  // checks, because the append-only log is the source of truth, not a calendar.
  static bool parseIso(const std::string& text, Date& out);

  bool isValid() const;
};

// True if the given day number is a valid civil date within HabitInk's range.
inline bool isReasonableYear(int16_t year) { return year >= 1970 && year <= 2200; }

}  // namespace habitcore
