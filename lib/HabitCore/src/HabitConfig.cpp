#include "habitcore/HabitConfig.h"

#include <cstdlib>
#include <sstream>

namespace habitcore {

namespace {
std::string rstrip(const std::string& s) {
  size_t end = s.size();
  while (end > 0 && (s[end - 1] == '\r' || s[end - 1] == '\n' || s[end - 1] == ' ' || s[end - 1] == '\t')) --end;
  return s.substr(0, end);
}

// Splits into at most four tab-separated fields. The name (field four) keeps
// any internal spaces but never a tab, so we stop splitting after three tabs.
bool splitFields(const std::string& line, std::string& id, std::string& icon, std::string& sched,
                 std::string& name) {
  size_t p1 = line.find('\t');
  if (p1 == std::string::npos) return false;
  size_t p2 = line.find('\t', p1 + 1);
  if (p2 == std::string::npos) return false;
  size_t p3 = line.find('\t', p2 + 1);
  if (p3 == std::string::npos) return false;
  id = line.substr(0, p1);
  icon = line.substr(p1 + 1, p2 - p1 - 1);
  sched = line.substr(p2 + 1, p3 - p2 - 1);
  name = line.substr(p3 + 1);
  return true;
}
}  // namespace

const Habit* HabitConfig::byId(int id) const {
  for (const auto& h : habits_)
    if (h.id == id) return &h;
  return nullptr;
}

int HabitConfig::nextFreeId() const {
  for (int id = 1; id <= kMaxHabits; ++id)
    if (byId(id) == nullptr) return id;
  return 0;
}

bool HabitConfig::add(Habit habit) {
  if (static_cast<int>(habits_.size()) >= kMaxHabits) return false;
  if (habit.id == 0) {
    habit.id = nextFreeId();
    if (habit.id == 0) return false;
  }
  if (byId(habit.id) != nullptr) return false;
  habits_.push_back(std::move(habit));
  return true;
}

bool HabitConfig::parse(const std::string& text, HabitConfig& out, std::vector<ParseIssue>* issues) {
  out.habits_.clear();
  std::istringstream stream(text);
  std::string rawLine;
  int lineNumber = 0;
  auto report = [&](int ln, const std::string& msg) {
    if (issues != nullptr) issues->push_back(ParseIssue{ln, msg});
  };

  while (std::getline(stream, rawLine)) {
    ++lineNumber;
    const std::string line = rstrip(rawLine);
    if (line.empty() || line[0] == '#') continue;

    std::string idField, iconField, schedField, nameField;
    if (!splitFields(line, idField, iconField, schedField, nameField)) {
      report(lineNumber, "expected four tab-separated fields (id, icon, schedule, name)");
      continue;
    }

    char* endPtr = nullptr;
    const long id = std::strtol(idField.c_str(), &endPtr, 10);
    if (endPtr == idField.c_str() || *endPtr != '\0' || id < 1 || id > kMaxHabits) {
      report(lineNumber, "id must be a whole number in the range 1 to 12");
      continue;
    }
    if (out.byId(static_cast<int>(id)) != nullptr) {
      report(lineNumber, "duplicate habit id");
      continue;
    }
    if (nameField.empty()) {
      report(lineNumber, "habit name must not be empty");
      continue;
    }
    Schedule schedule;
    if (!Schedule::parseField(schedField, schedule)) {
      report(lineNumber, "schedule must be 'daily' or seven 0/1 characters (Monday first)");
      continue;
    }
    if (static_cast<int>(out.habits_.size()) >= kMaxHabits) {
      report(lineNumber, "more than twelve habits defined; extra lines ignored");
      break;
    }

    Habit habit;
    habit.id = static_cast<int>(id);
    habit.icon = iconField;
    habit.schedule = schedule;
    habit.name = nameField;
    out.habits_.push_back(std::move(habit));
  }

  if (out.habits_.empty() && issues != nullptr && !issues->empty()) return false;
  return true;
}

std::string HabitConfig::serialise() const {
  std::ostringstream out;
  out << "# habitink habits v1\n";
  out << "# id\ticon\tschedule\tname\n";
  for (const auto& h : habits_) {
    out << h.id << '\t' << (h.icon.empty() ? "check" : h.icon) << '\t' << h.schedule.toField() << '\t' << h.name
        << '\n';
  }
  return out.str();
}

}  // namespace habitcore
