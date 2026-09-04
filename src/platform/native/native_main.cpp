// Host build entry point (PlatformIO `native` env and manual host builds).
//
// This is NOT the firmware. It compiles the exact portable firmware controller
// against a headless in-RAM Hal so the whole application layer is exercised by
// the toolchain in CI without the ESP32 SDK. The device entry point is
// src/main.cpp (guarded by ARDUINO).
#ifndef ARDUINO

#include <cstdio>
#include <deque>
#include <string>
#include <vector>

#include "habitcore/CompletionLog.h"
#include "src/HabitInkController.h"

namespace {

class HeadlessDisplay : public habitink::HalDisplay {
 public:
  HeadlessDisplay() : buffer_(static_cast<size_t>(100) * 480, 0xFF) {}
  uint8_t* framebuffer() override { return buffer_.data(); }
  int width() const override { return 800; }
  int height() const override { return 480; }
  int stride() const override { return 100; }
  void flush(bool) override { ++flushes; }
  int flushes = 0;

 private:
  std::vector<uint8_t> buffer_;
};

class HeadlessButtons : public habitink::HalButtons {
 public:
  std::deque<habitui::AppButton> queue;
  void poll() override {}
  bool popButton(habitui::AppButton& out) override {
    if (queue.empty()) return false;
    out = queue.front();
    queue.pop_front();
    return true;
  }
  bool powerHeld() const override { return false; }
  bool exitHeld() const override { return false; }
};

class HeadlessPower : public habitink::HalPower {
 public:
  uint32_t now = 0;
  habitink::WakeReason wakeReason() const override { return habitink::WakeReason::ColdBoot; }
  uint32_t millis() const override { return now; }
  void deepSleep() override {}
  bool rebootToReader() override { return false; }
};

class MemStore : public habitui::Store {
 public:
  std::string config = "1\twater\tdaily\tDrink water\n2\trun\tdaily\tRun\n";
  std::vector<std::string> log1;
  bool readConfig(std::string& out) override {
    out = config;
    return true;
  }
  bool writeConfig(const std::string& t) override {
    config = t;
    return true;
  }
  bool readLog(int id, const std::function<void(const std::string&)>& sink) override {
    if (id == 1)
      for (auto& l : log1) sink(l);
    return true;
  }
  bool appendLog(int id, const std::string& line) override {
    if (id == 1) log1.push_back(line);
    return true;
  }
};

class MemClock : public habitui::Clock {
 public:
  habitcore::Date today() override { return {2026, 7, 30}; }
  bool timeOfDay(uint8_t& h, uint8_t& m) override {
    h = 8;
    m = 15;
    return true;
  }
};

}  // namespace

int main() {
  HeadlessDisplay display;
  HeadlessButtons buttons;
  HeadlessPower power;
  MemStore store;
  MemClock clock;

  habitink::Hal hal;
  hal.display = &display;
  hal.buttons = &buttons;
  hal.power = &power;
  hal.store = &store;
  hal.clock = &clock;

  habitink::HabitInkController controller(hal);
  controller.begin();

  // Drive a short scripted session: cycle, toggle, then idle out to sleep.
  buttons.queue.push_back(habitui::AppButton::Next);
  buttons.queue.push_back(habitui::AppButton::Toggle);
  while (controller.tick()) {
    power.now += 25000;  // force the idle timeout so the loop terminates
  }

  std::printf("habitink host build OK: flushes=%d, done2=%d\n", display.flushes, controller.app().doneToday(1) ? 1 : 0);
  return 0;
}

#endif  // ARDUINO
