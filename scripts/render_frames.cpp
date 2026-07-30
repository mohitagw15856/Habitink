// Dev tool: render a scripted HabitInk session to a numbered sequence of PBM
// frames, which scripts/make_gifs.py assembles into the README demo GIF.
// Not part of the firmware.
//
//   g++ -std=c++17 -Ilib/HabitCore/include -Ilib/HabitUI/include -Itest \
//       lib/HabitCore/src/*.cpp lib/HabitUI/src/*.cpp scripts/render_frames.cpp \
//       -o /tmp/frames && /tmp/frames /tmp/frames_out
#include <cstdio>
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
  for (int off = 0; off < 64; ++off) {
    if (off % 6 == 3) continue;
    habitcore::Date d = habitcore::Date(2026, 7, 30).addDays(-off);
    store.appendLog(1, habitcore::CompletionLog::formatRecord(d, true));
    if (d.weekday() < habitcore::Weekday::Sat && off % 4 != 1)
      store.appendLog(2, habitcore::CompletionLog::formatRecord(d, true));
  }

  HabitApp app(store, clock);
  app.load();

  const int W = 800, H = 480, stride = 100;
  std::vector<uint8_t> buf(static_cast<size_t>(stride) * H, 0xFF);
  FrameCanvas c(buf.data(), W, H, stride);

  int frame = 0;
  auto snap = [&]() {
    char name[64];
    std::snprintf(name, sizeof(name), "%s/frame_%03d.pbm", outDir.c_str(), frame++);
    app.render(c);
    writePbm(name, buf, W, H, stride);
  };
  auto snapSleep = [&]() {
    char name[64];
    std::snprintf(name, sizeof(name), "%s/frame_%03d.pbm", outDir.c_str(), frame++);
    app.renderSleepFace(c);
    writePbm(name, buf, W, H, stride);
  };

  // A little story: land on home, cycle through a few habits, tick one off,
  // admire the year grid, glance at the week, then drift to the sleep face.
  snap();                              // home, water selected
  app.handleButton(AppButton::Next);   // -> run
  snap();
  app.handleButton(AppButton::Next);   // -> read
  snap();
  app.handleButton(AppButton::Toggle);  // tick "Read" off for today
  snap();
  app.handleButton(AppButton::Grid);   // year grid for Read
  snap();
  app.handleButton(AppButton::Back);
  app.handleButton(AppButton::Weekly);  // weekly overview
  snap();
  app.handleButton(AppButton::Back);
  snapSleep();  // standby face

  std::printf("wrote %d frames to %s\n", frame, outDir.c_str());
  return 0;
}
