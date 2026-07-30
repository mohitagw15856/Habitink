#include <vector>

#include "check.h"
#include "habitui/FrameCanvas.h"

using namespace habitui;

static std::vector<uint8_t> makeBuffer(int w, int h) {
  return std::vector<uint8_t>(static_cast<size_t>((w + 7) / 8) * h, 0xFF);
}

TEST(canvas_clear_and_pixel) {
  auto buf = makeBuffer(16, 16);
  FrameCanvas c(buf.data(), 16, 16);
  c.clear(true);
  CHECK(!c.inkAt(5, 5));  // white
  c.setPixel(5, 5, true);
  CHECK(c.inkAt(5, 5));
  c.setPixel(5, 5, false);
  CHECK(!c.inkAt(5, 5));
}

TEST(canvas_pixel_bit_position) {
  // Verify MSB-is-leftmost packing on a known byte.
  auto buf = makeBuffer(8, 1);
  FrameCanvas c(buf.data(), 8, 1);
  c.clear(true);           // 0xFF
  c.setPixel(0, 0, true);  // clear MSB -> 0x7F
  CHECK_EQ(buf[0], 0x7F);
  c.clear(true);
  c.setPixel(7, 0, true);  // clear LSB -> 0xFE
  CHECK_EQ(buf[0], 0xFE);
}

TEST(canvas_out_of_bounds_safe) {
  auto buf = makeBuffer(16, 16);
  FrameCanvas c(buf.data(), 16, 16);
  c.setPixel(-1, 5, true);
  c.setPixel(100, 5, true);
  c.setPixel(5, -1, true);
  c.setPixel(5, 100, true);
  CHECK(!c.inkAt(5, 5));  // nothing scribbled in range
}

TEST(canvas_fill_and_draw_rect) {
  auto buf = makeBuffer(32, 32);
  FrameCanvas c(buf.data(), 32, 32);
  c.clear(true);
  c.fillRect(4, 4, 8, 8, true);
  CHECK(c.inkAt(4, 4));
  CHECK(c.inkAt(11, 11));
  CHECK(!c.inkAt(12, 12));

  c.clear(true);
  c.drawRect(4, 4, 8, 8, true);
  CHECK(c.inkAt(4, 4));   // corner on border
  CHECK(c.inkAt(11, 4));  // top edge
  CHECK(!c.inkAt(7, 7));  // interior empty
}

TEST(canvas_text_width_and_draw) {
  auto buf = makeBuffer(200, 32);
  FrameCanvas c(buf.data(), 200, 32);
  c.clear(true);
  const int w1 = c.textWidth("AB", 1);
  const int w2 = c.textWidth("ABCD", 1);
  CHECK(w2 == 2 * w1);
  c.drawText(2, 2, "A", 2, true);
  // Some ink should be laid down for a visible glyph.
  bool anyInk = false;
  for (int y = 0; y < 32 && !anyInk; ++y)
    for (int x = 0; x < 40 && !anyInk; ++x)
      if (c.inkAt(x, y)) anyInk = true;
  CHECK(anyInk);
}

TEST(canvas_icon_draws_ink) {
  auto buf = makeBuffer(32, 32);
  FrameCanvas c(buf.data(), 32, 32);
  c.clear(true);
  c.drawIcon(habitcore::IconId::Check, 4, 4, true);
  int inkPixels = 0;
  for (int y = 0; y < 32; ++y)
    for (int x = 0; x < 32; ++x)
      if (c.inkAt(x, y)) ++inkPixels;
  CHECK(inkPixels > 0);
}

TEST(canvas_custom_stride) {
  // A wide panel-like stride: 100-byte rows for an 800px width.
  std::vector<uint8_t> buf(static_cast<size_t>(100) * 8, 0xFF);
  FrameCanvas c(buf.data(), 800, 8, 100);
  c.setPixel(799, 7, true);
  CHECK(c.inkAt(799, 7));
  CHECK_EQ(buf[7 * 100 + 99], 0xFE);
}
