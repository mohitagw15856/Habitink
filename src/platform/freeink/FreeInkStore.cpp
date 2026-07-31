#include "src/platform/freeink/FreeInkStore.h"

// This translation unit only compiles for the device build; the native tests
// use a mock Store instead.
#ifdef ARDUINO

#include <inkkit/SdStream.h>
#include <inkkit/Storage.h>

#include <cstdio>

namespace habitink {

std::string FreeInkStore::logPath(int habitId) const {
  char name[32];
  std::snprintf(name, sizeof(name), "/logs/%02d.log", habitId);
  return root_ + name;
}

void FreeInkStore::ensureDirs() const {
  inkkit::sd::ensureDir(root_.c_str());
  inkkit::sd::ensureDir((root_ + "/logs").c_str());
}

bool FreeInkStore::readConfig(std::string& outText) {
  // habits.tsv is tiny (<=12 short lines); reading it whole is fine.
  return inkkit::sd::readWholeFile(configPath().c_str(), outText);
}

bool FreeInkStore::writeConfig(const std::string& text) {
  ensureDirs();
  return inkkit::sd::writeWholeFile(configPath().c_str(), text);
}

bool FreeInkStore::readLog(int habitId, const std::function<void(const std::string&)>& sink) {
  const std::string path = logPath(habitId);
  if (!inkkit::sd::exists(path.c_str())) return false;

  HalFile file;
  if (!inkkit::sd::openRead("HAB", path.c_str(), file)) return false;

  // Stream line by line so a long history never buffers on the ~380KB heap.
  inkkit::readLines(file, sink);
  file.close();
  return true;
}

bool FreeInkStore::appendLog(int habitId, const std::string& recordLine) {
  ensureDirs();
  const std::string path = logPath(habitId);
  const bool fresh = !inkkit::sd::exists(path.c_str());

  HalFile file = inkkit::sd::openAppend(path.c_str());
  if (!file) return false;
  if (fresh) {
    file.write("# habitink log v1\n", 18);
  }
  const std::string out = recordLine + "\n";
  file.write(out.data(), out.size());
  return true;
}

}  // namespace habitink

#endif  // ARDUINO
