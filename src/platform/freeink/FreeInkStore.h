// habitui::Store backed by the freeink-sdk SD card (via the HalStorage wrapper
// used across the CrossPoint / freeink ecosystem).
//
// TODO(hardware-test): the exact HalStorage / HalFile method names below are
// modelled on the ecosystem API (docs/ARCHITECTURE.md, docs/HARDWARE_TESTING.md)
// and must be confirmed against the pinned freeink-sdk on device.
#pragma once

#include <functional>
#include <string>

#include "habitui/Env.h"

namespace habitink {

class FreeInkStore : public habitui::Store {
 public:
  // rootDir is the SD path that holds HabitInk's data, e.g. "/.habitink".
  explicit FreeInkStore(const char* rootDir = "/.habitink") : root_(rootDir) {}

  bool readConfig(std::string& outText) override;
  bool writeConfig(const std::string& text) override;
  bool readLog(int habitId, const std::function<void(const std::string&)>& sink) override;
  bool appendLog(int habitId, const std::string& recordLine) override;

 private:
  std::string configPath() const { return root_ + "/habits.tsv"; }
  std::string logPath(int habitId) const;
  void ensureDirs() const;

  std::string root_;
};

}  // namespace habitink
