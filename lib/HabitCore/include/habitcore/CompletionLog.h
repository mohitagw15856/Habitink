// Effective completion state derived from an append-only log for one habit.
//
// The on-disk log is append only: toggling a day writes a new line rather than
// rewriting the file (see docs/FORMAT.md). The effective state of a date is the
// value of the last record naming that date. CompletionLog folds a stream of
// lines into that effective state.
//
// Memory discipline: the firmware never needs the whole history at once. A
// window [minDay, maxDay] (inclusive day numbers) bounds what is retained, so
// the home "is today done" check keeps a single day, the weekly face keeps
// seven, and the year grid keeps a year, regardless of how many times a habit
// has been toggled over its lifetime. Lines are fed one at a time so the caller
// can stream straight from the SD card without buffering the file.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "habitcore/Date.h"

namespace habitcore {

class CompletionLog {
 public:
  CompletionLog() = default;

  // Restrict retained records to the inclusive day-number window. Records
  // outside it are ignored as they are fed. Call before feeding lines.
  void setWindow(int32_t minDay, int32_t maxDay) {
    hasWindow_ = true;
    minDay_ = minDay;
    maxDay_ = maxDay;
  }
  void setWindow(const Date& min, const Date& max) { setWindow(min.toDayNumber(), max.toDayNumber()); }

  // Feed one raw log line. Comment (#...) and blank lines are ignored. A valid
  // data line is "YYYY-MM-DD\tV" where V is '1' or '0'. Malformed lines are
  // skipped rather than throwing, so a partially corrupt log still yields the
  // records it can. Returns true if the line was a valid data record.
  bool applyLine(const std::string& line);

  // Record a single completion state directly (last write wins).
  void applyRecord(const Date& date, bool done);

  // Effective completion for a date; false if never logged (or logged then
  // cleared).
  bool isDone(const Date& date) const;
  bool isDone(int32_t dayNumber) const;

  // Number of retained "done" days.
  size_t doneCount() const;

  bool empty() const { return effective_.empty(); }

  // Sorted list of day numbers currently marked done, ascending.
  std::vector<int32_t> doneDays() const;

  // Day number of the earliest retained record (done or cleared). Returns false
  // if the log has no records. Used to anchor grids so days before a habit
  // existed render blank rather than as missed.
  bool firstRecordedDay(int32_t& out) const;

  // Formats a single append line for a record (no trailing newline).
  static std::string formatRecord(const Date& date, bool done);
  // The header line written at the top of a fresh log file.
  static const char* headerLine();

 private:
  bool inWindow(int32_t dayNumber) const { return !hasWindow_ || (dayNumber >= minDay_ && dayNumber <= maxDay_); }

  std::map<int32_t, bool> effective_;  // dayNumber -> done, last write wins
  bool hasWindow_ = false;
  int32_t minDay_ = 0;
  int32_t maxDay_ = 0;
};

}  // namespace habitcore
