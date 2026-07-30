#include "habitcore/Habit.h"

#include <cctype>

namespace habitcore {

int Schedule::dueDaysPerWeek() const {
  int count = 0;
  for (uint8_t i = 0; i < 7; ++i)
    if ((mask_ >> i) & 1u) ++count;
  return count;
}

std::string Schedule::toField() const {
  if (isDaily()) return "daily";
  std::string out(7, '0');
  for (uint8_t i = 0; i < 7; ++i)
    if ((mask_ >> i) & 1u) out[i] = '1';
  return out;
}

bool Schedule::parseField(const std::string& text, Schedule& out) {
  // Case-insensitive "daily".
  if (text.size() == 5) {
    std::string lower;
    lower.reserve(5);
    for (char c : text) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (lower == "daily") {
      out = Schedule::daily();
      return true;
    }
  }
  if (text.size() != 7) return false;
  uint8_t mask = 0;
  for (uint8_t i = 0; i < 7; ++i) {
    if (text[i] == '1') {
      mask |= static_cast<uint8_t>(1u << i);
    } else if (text[i] != '0') {
      return false;
    }
  }
  out = Schedule(mask);
  return true;
}

}  // namespace habitcore
