#include "habitcore/CompletionLog.h"

namespace habitcore {

namespace {
// Trim trailing CR (logs written on a host may carry CRLF) and surrounding
// whitespace without pulling in <algorithm>.
std::string trimmed(const std::string& in) {
  size_t start = 0;
  size_t end = in.size();
  while (start < end && (in[start] == ' ' || in[start] == '\t')) ++start;
  while (end > start && (in[end - 1] == ' ' || in[end - 1] == '\t' || in[end - 1] == '\r' || in[end - 1] == '\n'))
    --end;
  return in.substr(start, end - start);
}
}  // namespace

const char* CompletionLog::headerLine() { return "# habitink log v1"; }

std::string CompletionLog::formatRecord(const Date& date, bool done) { return date.toIso() + "\t" + (done ? "1" : "0"); }

bool CompletionLog::applyLine(const std::string& rawLine) {
  const std::string line = trimmed(rawLine);
  if (line.empty() || line[0] == '#') return false;

  // Split on the first tab (or run of whitespace) into date and value.
  size_t sep = line.find('\t');
  if (sep == std::string::npos) sep = line.find(' ');
  if (sep == std::string::npos) return false;

  const std::string dateText = trimmed(line.substr(0, sep));
  const std::string valueText = trimmed(line.substr(sep + 1));
  if (valueText.empty()) return false;

  Date date;
  if (!Date::parseIso(dateText, date)) return false;

  bool done;
  if (valueText[0] == '1') {
    done = true;
  } else if (valueText[0] == '0') {
    done = false;
  } else {
    return false;
  }

  applyRecord(date, done);
  return true;
}

void CompletionLog::applyRecord(const Date& date, bool done) {
  const int32_t dn = date.toDayNumber();
  if (!inWindow(dn)) return;
  effective_[dn] = done;  // last write wins
}

bool CompletionLog::isDone(int32_t dayNumber) const {
  auto it = effective_.find(dayNumber);
  return it != effective_.end() && it->second;
}

bool CompletionLog::isDone(const Date& date) const { return isDone(date.toDayNumber()); }

size_t CompletionLog::doneCount() const {
  size_t n = 0;
  for (const auto& kv : effective_)
    if (kv.second) ++n;
  return n;
}

bool CompletionLog::firstRecordedDay(int32_t& out) const {
  if (effective_.empty()) return false;
  out = effective_.begin()->first;  // std::map keeps keys sorted ascending
  return true;
}

std::vector<int32_t> CompletionLog::doneDays() const {
  std::vector<int32_t> out;
  out.reserve(effective_.size());
  for (const auto& kv : effective_)
    if (kv.second) out.push_back(kv.first);
  return out;  // std::map iterates in ascending key order
}

}  // namespace habitcore
