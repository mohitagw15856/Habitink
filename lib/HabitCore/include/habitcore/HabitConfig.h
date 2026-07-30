// Parsing and serialisation of habits.tsv (the habit definition file).
//
// The format is deliberately line oriented and tab separated so it can be hand
// edited and diffed. See docs/FORMAT.md for the specification. Parsing is
// lenient about comments and blank lines but strict about field structure, and
// it enforces the twelve habit ceiling and unique ids.
#pragma once

#include <string>
#include <vector>

#include "habitcore/Habit.h"

namespace habitcore {

struct ParseIssue {
  int lineNumber = 0;   // 1-based line in the source, 0 for whole-file issues
  std::string message;  // human readable, British English
};

class HabitConfig {
 public:
  const std::vector<Habit>& habits() const { return habits_; }
  std::vector<Habit>& habits() { return habits_; }
  int size() const { return static_cast<int>(habits_.size()); }
  bool empty() const { return habits_.empty(); }

  // Finds a habit by id; returns nullptr if absent.
  const Habit* byId(int id) const;

  // Returns the first unused id in 1..kMaxHabits, or 0 if full.
  int nextFreeId() const;

  // Adds a habit, assigning nextFreeId() when habit.id is 0. Returns false if
  // the config is full or the id clashes.
  bool add(Habit habit);

  // Parses a whole habits.tsv document. Recoverable problems are collected in
  // issues; the returned config contains every well-formed habit. Returns false
  // only when nothing usable could be parsed and issues is non-empty.
  static bool parse(const std::string& text, HabitConfig& out, std::vector<ParseIssue>* issues = nullptr);

  // Serialises back to habits.tsv text, including the header and column legend.
  std::string serialise() const;

 private:
  std::vector<Habit> habits_;
};

}  // namespace habitcore
