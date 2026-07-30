#include "habitui/FrameCanvas.h"

#include <cstring>

#include "habitui/Icons.h"

namespace habitui {

FrameCanvas::FrameCanvas(uint8_t* buffer, int width, int height, int strideBytes)
    : buffer_(buffer),
      width_(width),
      height_(height),
      stride_(strideBytes > 0 ? strideBytes : (width + 7) / 8),
      font_(defaultFont()) {}

void FrameCanvas::clear(bool white) {
  std::memset(buffer_, white ? 0xFF : 0x00, static_cast<size_t>(stride_) * height_);
}

void FrameCanvas::setPixel(int x, int y, bool ink) {
  if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
  const int idx = y * stride_ + (x >> 3);
  const uint8_t bit = static_cast<uint8_t>(0x80u >> (x & 7));
  if (ink) {
    buffer_[idx] &= static_cast<uint8_t>(~bit);  // clear bit -> black
  } else {
    buffer_[idx] |= bit;  // set bit -> white
  }
}

bool FrameCanvas::inkAt(int x, int y) const {
  if (x < 0 || y < 0 || x >= width_ || y >= height_) return false;
  const int idx = y * stride_ + (x >> 3);
  const uint8_t bit = static_cast<uint8_t>(0x80u >> (x & 7));
  return (buffer_[idx] & bit) == 0;  // cleared bit == black == ink
}

void FrameCanvas::fillRect(int x, int y, int w, int h, bool ink) {
  if (w <= 0 || h <= 0) return;
  for (int yy = y; yy < y + h; ++yy)
    for (int xx = x; xx < x + w; ++xx) setPixel(xx, yy, ink);
}

void FrameCanvas::hLine(int x, int y, int len, bool ink) {
  for (int i = 0; i < len; ++i) setPixel(x + i, y, ink);
}

void FrameCanvas::vLine(int x, int y, int len, bool ink) {
  for (int i = 0; i < len; ++i) setPixel(x, y + i, ink);
}

void FrameCanvas::drawRect(int x, int y, int w, int h, bool ink) {
  if (w <= 0 || h <= 0) return;
  hLine(x, y, w, ink);
  hLine(x, y + h - 1, w, ink);
  vLine(x, y, h, ink);
  vLine(x + w - 1, y, h, ink);
}

void FrameCanvas::drawRect(int x, int y, int w, int h, int thickness, bool ink) {
  for (int t = 0; t < thickness; ++t) drawRect(x + t, y + t, w - 2 * t, h - 2 * t, ink);
}

void FrameCanvas::drawRoundedRect(int x, int y, int w, int h, int radius, bool ink) {
  if (w <= 0 || h <= 0) return;
  if (radius * 2 > w) radius = w / 2;
  if (radius * 2 > h) radius = h / 2;
  // Straight edges.
  hLine(x + radius, y, w - 2 * radius, ink);
  hLine(x + radius, y + h - 1, w - 2 * radius, ink);
  vLine(x, y + radius, h - 2 * radius, ink);
  vLine(x + w - 1, y + radius, h - 2 * radius, ink);
  // Corners via a simple midpoint circle over one quadrant, mirrored.
  int cxL = x + radius, cxR = x + w - 1 - radius;
  int cyT = y + radius, cyB = y + h - 1 - radius;
  int px = radius, py = 0, err = 1 - radius;
  while (px >= py) {
    setPixel(cxR + px, cyB + py, ink);
    setPixel(cxL - px, cyB + py, ink);
    setPixel(cxR + px, cyT - py, ink);
    setPixel(cxL - px, cyT - py, ink);
    setPixel(cxR + py, cyB + px, ink);
    setPixel(cxL - py, cyB + px, ink);
    setPixel(cxR + py, cyT - px, ink);
    setPixel(cxL - py, cyT - px, ink);
    ++py;
    if (err < 0) {
      err += 2 * py + 1;
    } else {
      --px;
      err += 2 * (py - px) + 1;
    }
  }
}

int FrameCanvas::textWidth(const char* text, int scale) const {
  int n = 0;
  for (const char* p = text; *p; ++p) ++n;
  return n * glyphWidth(scale);
}

void FrameCanvas::drawChar(int x, int y, char c, int scale, bool ink) {
  const uint8_t* glyph = font_.glyph(c);
  for (int row = 0; row < font_.height; ++row) {
    const uint8_t bits = glyph[row];
    for (int col = 0; col < font_.width; ++col) {
      if (bits & (0x80u >> col)) {
        if (scale == 1) {
          setPixel(x + col, y + row, ink);
        } else {
          fillRect(x + col * scale, y + row * scale, scale, scale, ink);
        }
      }
    }
  }
}

void FrameCanvas::drawText(int x, int y, const char* text, int scale, bool ink) {
  int cursor = x;
  for (const char* p = text; *p; ++p) {
    drawChar(cursor, y, *p, scale, ink);
    cursor += glyphWidth(scale);
  }
}

void FrameCanvas::drawTextCentered(int cx, int y, const char* text, int scale, bool ink) {
  drawText(cx - textWidth(text, scale) / 2, y, text, scale, ink);
}

void FrameCanvas::drawIcon(habitcore::IconId id, int x, int y, bool ink) {
  const IconBitmap& icon = iconBitmap(id);
  for (int row = 0; row < icon.size; ++row) {
    const uint8_t* rowBits = icon.data + row * ((icon.size + 7) / 8);
    for (int col = 0; col < icon.size; ++col) {
      if (rowBits[col >> 3] & (0x80u >> (col & 7))) setPixel(x + col, y + row, ink);
    }
  }
}

}  // namespace habitui
