#include "src/platform/freeink/FreeInkStore.h"

// This translation unit only compiles for the device build; the native tests
// use a mock Store instead.
#ifdef ARDUINO

#include <HalStorage.h>

#include <cstdio>

namespace habitink {

std::string FreeInkStore::logPath(int habitId) const {
  char name[32];
  std::snprintf(name, sizeof(name), "/logs/%02d.log", habitId);
  return root_ + name;
}

void FreeInkStore::ensureDirs() const {
  // TODO(hardware-test): confirm ensureDirectoryExists creates parents.
  Storage.ensureDirectoryExists(root_.c_str());
  Storage.ensureDirectoryExists((root_ + "/logs").c_str());
}

bool FreeInkStore::readConfig(std::string& outText) {
  if (!Storage.exists(configPath().c_str())) return false;
  // habits.tsv is tiny (<=12 short lines); reading it whole is fine.
  String content = Storage.readFile(configPath().c_str());
  outText.assign(content.c_str(), content.length());
  return !outText.empty();
}

bool FreeInkStore::writeConfig(const std::string& text) {
  ensureDirs();
  return Storage.writeFile(configPath().c_str(), String(text.c_str()));
}

bool FreeInkStore::readLog(int habitId, const std::function<void(const std::string&)>& sink) {
  const std::string path = logPath(habitId);
  if (!Storage.exists(path.c_str())) return false;

  HalFile file;
  if (!Storage.openFileForRead("HAB", path.c_str(), file)) return false;

  // Stream line by line so a long history never buffers on the ~380KB heap.
  std::string line;
  line.reserve(24);
  char chunk[64];
  int n;
  while ((n = file.read(chunk, sizeof(chunk))) > 0) {
    for (int i = 0; i < n; ++i) {
      const char c = chunk[i];
      if (c == '\n') {
        sink(line);
        line.clear();
      } else if (c != '\r') {
        line.push_back(c);
      }
    }
  }
  if (!line.empty()) sink(line);
  return true;
}

bool FreeInkStore::appendLog(int habitId, const std::string& recordLine) {
  ensureDirs();
  const std::string path = logPath(habitId);
  const bool fresh = !Storage.exists(path.c_str());

  // TODO(hardware-test): confirm openFileForWrite opens in append mode, or
  // switch to Storage.open(path, O_WRONLY | O_CREAT | O_APPEND).
  HalFile file = Storage.open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND);
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
