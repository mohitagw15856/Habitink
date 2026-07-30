// Dev tool: render each HabitInk screen to a PBM image for docs and layout
// checks. Not part of the firmware. Build with the native tests' include paths.
//
//   g++ -std=c++17 -Ilib/HabitCore/include -Ilib/HabitUI/include \
//       lib/HabitCore/src/*.cpp lib/HabitUI/src/*.cpp scripts/render_preview.cpp \
//       -o /tmp/preview && /tmp/preview docs/images
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "habitui/HabitApp.h"
#include "habitui/fakes.h"

using namespace habitui;

static void writePbm(const std::string& path, const std::vector<uint8_t>& buf, int w, int h, int stride) {
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return;
  std::fprintf(f, "P1\n%d %d\n", w, h);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const uint8_t byte = buf[y * stride + (x >> 3)];
      const bool ink = (byte & (0x80u >> (x & 7))) == 0;
      std::fputc(ink ? '1' : '0', f);
      std::fputc(x + 1 < w ? ' ' : '\n', f);
    }
  }
  std::fclose(f);
}

int main(int argc, char** argv) {
  const std::string outDir = argc > 1 ? argv[1] : ".";
  FakeStore store;
  FakeClock clock;
  store.config =
      "# habitink habits v1\n"
      "1\twater\tdaily\tDrink water\n"
      "2\trun\t1111100\tMorning run\n"
      "3\tbook\tdaily\tRead 20 pages\n"
      "4\tmeditate\t1010100\tMeditate\n"
      "5\tcode\tdaily\tShip code\n";
  // Seed a realistic-looking history for the first habit.
  for (int off = 0; off < 60; ++off) {
    if (off % 5 == 2) continue;  // a few misses
    habitcore::Date d = habitcore::Date(2026, 7, 30).addDays(-off);
    store.appendLog(1, habitcore::CompletionLog::formatRecord(d, true));
  }
  store.appendLog(2, "2026-07-27\t1");
  store.appendLog(2, "2026-07-28\t1");
  store.appendLog(3, "2026-07-30\t1");

  HabitApp app(store, clock);
  app.load();

  const int W = 800, H = 480, stride = 100;
  std::vector<uint8_t> buf(static_cast<size_t>(stride) * H, 0xFF);
  FrameCanvas c(buf.data(), W, H, stride);

  app.render(c);
  writePbm(outDir + "/preview_home.pbm", buf, W, H, stride);

  app.handleButton(AppButton::Grid);
  app.render(c);
  writePbm(outDir + "/preview_grid.pbm", buf, W, H, stride);

  app.handleButton(AppButton::Back);
  app.handleButton(AppButton::Weekly);
  app.render(c);
  writePbm(outDir + "/preview_weekly.pbm", buf, W, H, stride);

  app.renderSleepFace(c);
  writePbm(outDir + "/preview_sleep.pbm", buf, W, H, stride);

  std::printf("wrote previews to %s\n", outDir.c_str());
  return 0;
}
