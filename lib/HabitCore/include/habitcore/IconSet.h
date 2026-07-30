// The built-in 1-bit icon set, referenced by token.
//
// HabitCore only knows icons as tokens (stable strings stored in habits.tsv)
// mapped to an IconId enum. The actual 1-bit bitmaps live in the firmware
// (src/icons) and the companion renders its own equivalents, but both sides
// agree on this token list so a config file is portable between them.
#pragma once

#include <cstddef>
#include <cstdint>

namespace habitcore {

enum class IconId : uint8_t {
  Check = 0,
  Water,
  Run,
  Book,
  Pill,
  Meditate,
  Dumbbell,
  Sun,
  Moon,
  Heart,
  Food,
  Pen,
  Code,
  Music,
  Leaf,
  Star,
  Count_  // sentinel, keep last
};

constexpr int kIconCount = static_cast<int>(IconId::Count_);

// Canonical token for each icon, index matches IconId. Kept as a free function
// returning a table so both firmware and tests share one definition.
const char* const* iconTokens();

// Maps a token to its IconId. Unknown tokens fall back to IconId::Check so a
// hand-edited config never leaves a habit iconless.
IconId iconFromToken(const char* token);
const char* tokenFromIcon(IconId id);

}  // namespace habitcore
