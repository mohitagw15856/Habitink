#include "habitcore/IconSet.h"

#include <cstring>

namespace habitcore {

namespace {
// Order must match the IconId enum exactly.
const char* const kTokens[] = {
    "check", "water", "run",  "book",  "pill", "meditate", "dumbbell", "sun",
    "moon",  "heart", "food", "pen",   "code", "music",    "leaf",     "star",
};
static_assert(sizeof(kTokens) / sizeof(kTokens[0]) == kIconCount, "token table out of sync with IconId");
}  // namespace

const char* const* iconTokens() { return kTokens; }

IconId iconFromToken(const char* token) {
  if (token != nullptr) {
    for (int i = 0; i < kIconCount; ++i) {
      if (std::strcmp(token, kTokens[i]) == 0) return static_cast<IconId>(i);
    }
  }
  return IconId::Check;
}

const char* tokenFromIcon(IconId id) {
  const int i = static_cast<int>(id);
  if (i < 0 || i >= kIconCount) return kTokens[0];
  return kTokens[i];
}

}  // namespace habitcore
