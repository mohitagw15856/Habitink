// In-memory Store and Clock fakes for HabitUI tests.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "habitui/Env.h"

namespace habitui {

class FakeClock : public Clock {
 public:
  habitcore::Date fixedToday{2026, 7, 30};
  bool hasTime = true;
  uint8_t hour = 8, minute = 15;

  habitcore::Date today() override { return fixedToday; }
  bool timeOfDay(uint8_t& h, uint8_t& m) override {
    if (!hasTime) return false;
    h = hour;
    m = minute;
    return true;
  }
};

class FakeStore : public Store {
 public:
  std::string config;
  std::map<int, std::vector<std::string>> logs;  // habitId -> lines

  bool readConfig(std::string& outText) override {
    if (config.empty()) return false;
    outText = config;
    return true;
  }
  bool writeConfig(const std::string& text) override {
    config = text;
    return true;
  }
  bool readLog(int habitId, const std::function<void(const std::string&)>& sink) override {
    auto it = logs.find(habitId);
    if (it == logs.end()) return false;
    for (const auto& line : it->second) sink(line);
    return true;
  }
  bool appendLog(int habitId, const std::string& recordLine) override {
    auto& lines = logs[habitId];
    if (lines.empty()) lines.push_back(habitcore::CompletionLog::headerLine());
    lines.push_back(recordLine);
    return true;
  }

  int recordCount(int habitId) const {
    auto it = logs.find(habitId);
    if (it == logs.end()) return 0;
    int n = 0;
    for (const auto& l : it->second)
      if (!l.empty() && l[0] != '#') ++n;
    return n;
  }
};

}  // namespace habitui
