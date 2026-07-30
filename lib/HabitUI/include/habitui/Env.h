// Abstract environment the app runs against.
//
// HabitApp never touches the SD card, RTC or buttons directly. It talks to these
// three tiny interfaces so the whole app (including the wake/log/sleep flow) can
// be exercised natively with in-memory fakes, and the firmware supplies thin
// freeink-sdk backed implementations. Log reading is streamed line by line so a
// device implementation never buffers a whole file.
#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "habitcore/Date.h"

namespace habitui {

// Logical buttons, decoupled from any physical layout. The firmware maps
// hardware buttons onto these (see docs/ARCHITECTURE.md).
enum class AppButton : uint8_t {
  Next,    // cycle to the next habit
  Prev,    // cycle to the previous habit
  Toggle,  // toggle today's completion for the selected habit
  Grid,    // open the year grid for the selected habit
  Weekly,  // open the all-habits weekly view
  Back,    // leave a sub-view / return home
};

class Clock {
 public:
  virtual ~Clock() = default;
  // Today's civil date in the user's local time.
  virtual habitcore::Date today() = 0;
  // Local wall-clock time. Returns false if no RTC is present.
  virtual bool timeOfDay(uint8_t& hour, uint8_t& minute) = 0;
};

class Store {
 public:
  virtual ~Store() = default;

  // Reads habits.tsv into outText. Returns false if the file is missing.
  virtual bool readConfig(std::string& outText) = 0;
  // Overwrites habits.tsv. Returns true on success.
  virtual bool writeConfig(const std::string& text) = 0;

  // Streams each line of the log for habitId to sink (without trailing
  // newline). Returns false if there is no log yet. The callback must not
  // retain references to the passed string.
  virtual bool readLog(int habitId, const std::function<void(const std::string&)>& sink) = 0;

  // Appends one record line to the log for habitId, creating the file with a
  // header if needed. recordLine has no trailing newline. Returns true on
  // success.
  virtual bool appendLog(int habitId, const std::string& recordLine) = 0;
};

}  // namespace habitui
