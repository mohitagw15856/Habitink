#include "check.h"

int runAllTests() {
  int passed = 0;
  for (auto& c : tinytest::registry()) {
    const int before = tinytest::failures();
    std::printf("[ RUN  ] %s\n", c.name.c_str());
    try {
      c.fn();
    } catch (const tinytest::AssertionError&) {
      // Already reported by fail(); continue to the next case.
    } catch (const std::exception& e) {
      ++tinytest::failures();
      std::printf("    THREW std::exception: %s\n", e.what());
    }
    if (tinytest::failures() == before) {
      ++passed;
      std::printf("[  OK  ] %s\n", c.name.c_str());
    } else {
      std::printf("[ FAIL ] %s\n", c.name.c_str());
    }
  }
  const int total = static_cast<int>(tinytest::registry().size());
  std::printf("\n%d/%d passed, %d assertion failure(s)\n", passed, total, tinytest::failures());
  return tinytest::failures() == 0 ? 0 : 1;
}

int main() { return runAllTests(); }
