// Portable 1-bit software rasteriser.
//
// FrameCanvas draws straight into a caller-owned framebuffer using the same
// convention as the e-ink panel: one bit per pixel, MSB is the leftmost pixel,
// a set bit is white and a cleared bit is black ink. On device the firmware
// hands FrameCanvas the display's framebuffer and calls displayBuffer()
// afterwards, so there is no second buffer and no copy. In tests it draws into
// a small std::vector and asserts on inkAt(). Nothing here allocates.
#pragma once

#include <cstdint>

#include "habitcore/IconSet.h"
#include "habitui/Font.h"

namespace habitui {

class FrameCanvas {
 public:
  // stride defaults to the tightly packed (width + 7) / 8 bytes per row.
  FrameCanvas(uint8_t* buffer, int width, int height, int strideBytes = 0);

  int width() const { return width_; }
  int height() const { return height_; }
  uint8_t* buffer() const { return buffer_; }
  int stride() const { return stride_; }

  void clear(bool white = true);

  // ink == true draws black; ink == false draws white.
  void setPixel(int x, int y, bool ink);
  bool inkAt(int x, int y) const;  // true when the pixel is black

  void fillRect(int x, int y, int w, int h, bool ink);
  void drawRect(int x, int y, int w, int h, bool ink);                // 1px border
  void drawRect(int x, int y, int w, int h, int thickness, bool ink);  // inset border
  void drawRoundedRect(int x, int y, int w, int h, int radius, bool ink);
  void hLine(int x, int y, int len, bool ink);
  void vLine(int x, int y, int len, bool ink);

  // Text. Scale is an integer pixel multiplier (1, 2, 3...). One pixel of
  // spacing is added to the right of each glyph.
  int glyphWidth(int scale) const { return font_.width * scale + scale; }
  int lineHeight(int scale) const { return font_.height * scale + scale; }
  int textWidth(const char* text, int scale) const;
  void drawChar(int x, int y, char c, int scale, bool ink);
  void drawText(int x, int y, const char* text, int scale, bool ink);
  // Centres horizontally around cx.
  void drawTextCentered(int cx, int y, const char* text, int scale, bool ink);

  // 1-bit square icon (see Icons.h). Icon pixels marked as ink are drawn in the
  // requested colour; the rest are left untouched (transparent).
  void drawIcon(habitcore::IconId id, int x, int y, bool ink);

 private:
  uint8_t* buffer_;
  int width_;
  int height_;
  int stride_;
  const Font& font_;
};

}  // namespace habitui
