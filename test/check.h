// Tiny zero-dependency test harness for HabitInk's native tests.
//
// Deliberately not GoogleTest: the CI runner builds this with nothing but a
// C++17 compiler and CMake, so there is no network fetch to flake on. Each test
// is a function registered with TEST(); main() runs them all and reports.
#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace tinytest {

struct Case {
  std::string name;
  std::function<void()> fn;
};

inline std::vector<Case>& registry() {
  static std::vector<Case> cases;
  return cases;
}

inline int& failures() {
  static int f = 0;
  return f;
}

struct Registrar {
  Registrar(const std::string& name, std::function<void()> fn) { registry().push_back({name, std::move(fn)}); }
};

struct AssertionError {
  std::string message;
};

inline void fail(const std::string& file, int line, const std::string& expr) {
  ++failures();
  std::printf("    ASSERT FAILED: %s\n      at %s:%d\n", expr.c_str(), file.c_str(), line);
  throw AssertionError{expr};
}

}  // namespace tinytest

#define TEST(name)                                            \
  static void name();                                         \
  static ::tinytest::Registrar registrar_##name(#name, name); \
  static void name()

#define CHECK(cond)                                           \
  do {                                                        \
    if (!(cond)) ::tinytest::fail(__FILE__, __LINE__, #cond); \
  } while (0)

#define CHECK_EQ(a, b)                                                     \
  do {                                                                     \
    if (!((a) == (b))) ::tinytest::fail(__FILE__, __LINE__, #a " == " #b); \
  } while (0)

#define CHECK_MSG(cond, msg)                                                                    \
  do {                                                                                          \
    if (!(cond)) ::tinytest::fail(__FILE__, __LINE__, std::string(#cond) + " (" + (msg) + ")"); \
  } while (0)

// Provided by test/test_main.cpp
int runAllTests();
