// In-memory Hal implementation for exercising HabitInkController natively.
#pragma once

#include <deque>
#include <vector>

#include "habitui/fakes.h"
#include "src/platform/Hal.h"

namespace habitink {

class MockDisplay : public HalDisplay {
 public:
  MockDisplay(int w = 800, int h = 480) : width_(w), height_(h), stride_((w + 7) / 8) {
    buffer_.assign(static_cast<size_t>(stride_) * height_, 0xFF);
  }
  uint8_t* framebuffer() override { return buffer_.data(); }
  int width() const override { return width_; }
  int height() const override { return height_; }
  int stride() const override { return stride_; }
  void flush(bool full) override {
    ++flushes;
    lastFull = full;
  }

  bool anyInk() const {
    for (uint8_t b : buffer_)
      if (b != 0xFF) return true;
    return false;
  }

  int flushes = 0;
  bool lastFull = false;

 private:
  int width_, height_, stride_;
  std::vector<uint8_t> buffer_;
};

class MockButtons : public HalButtons {
 public:
  std::deque<habitui::AppButton> queued;
  bool holdPower = false;

  void poll() override {}
  bool popButton(habitui::AppButton& out) override {
    if (queued.empty()) return false;
    out = queued.front();
    queued.pop_front();
    return true;
  }
  bool powerHeld() const override { return holdPower; }
};

class MockPower : public HalPower {
 public:
  WakeReason reason = WakeReason::ColdBoot;
  uint32_t now = 0;

  WakeReason wakeReason() const override { return reason; }
  uint32_t millis() const override { return now; }
  void deepSleep() override { ++deepSleeps; }

  int deepSleeps = 0;
};

}  // namespace habitink
