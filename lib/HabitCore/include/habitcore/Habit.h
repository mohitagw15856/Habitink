// Habit definition and its weekly schedule.
//
// A Schedule is a seven bit mask, one bit per weekday (Monday first, matching
// habitcore::Weekday). "daily" is simply all seven bits set. Everything here is
// value-typed and allocation free so it is cheap to hold the whole config
// (at most twelve habits) in RAM on device.
#pragma once

#include <cstdint>
#include <string>

#include "habitcore/Date.h"

namespace habitcore {

constexpr int kMaxHabits = 12;

class Schedule {
 public:
  constexpr Schedule() = default;
  explicit constexpr Schedule(uint8_t mask) : mask_(mask & 0x7F) {}

  static constexpr Schedule daily() { return Schedule(0x7F); }

  bool isDue(Weekday day) const { return (mask_ >> static_cast<uint8_t>(day)) & 1u; }
  bool isDue(const Date& date) const { return isDue(date.weekday()); }
  bool isDaily() const { return (mask_ & 0x7F) == 0x7F; }
  bool isNever() const { return (mask_ & 0x7F) == 0; }
  uint8_t mask() const { return mask_; }
  int dueDaysPerWeek() const;

  // Serialises to the schedule field used in habits.tsv: "daily" for an
  // all-days schedule, otherwise seven characters of '1'/'0', Monday first.
  std::string toField() const;
  // Parses a schedule field. Accepts "daily" (any case) or exactly seven
  // '0'/'1' characters. Returns false on anything else.
  static bool parseField(const std::string& text, Schedule& out);

 private:
  uint8_t mask_ = 0x7F;
};

struct Habit {
  int id = 0;        // 1..kMaxHabits, stable identity used for the log filename
  std::string icon;  // token from the built-in icon set (see IconSet.h)
  std::string name;  // display name, UTF-8, no tab characters
  Schedule schedule = Schedule::daily();

  bool isValid() const { return id >= 1 && id <= kMaxHabits && !name.empty(); }
};

}  // namespace habitcore
