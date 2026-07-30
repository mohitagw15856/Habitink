#include "check.h"
#include "mock_hal.h"
#include "src/HabitInkController.h"

using namespace habitink;
using habitui::AppButton;

namespace {
Hal makeHal(MockDisplay& d, MockButtons& b, MockPower& p, habitui::FakeStore& s, habitui::FakeClock& c) {
  Hal hal;
  hal.display = &d;
  hal.buttons = &b;
  hal.power = &p;
  hal.store = &s;
  hal.clock = &c;
  return hal;
}
const char* kConfig = "1\twater\tdaily\tDrink water\n2\trun\tdaily\tRun\n";
}  // namespace

TEST(controller_begin_paints_full_on_cold_boot) {
  MockDisplay d;
  MockButtons b;
  MockPower p;
  habitui::FakeStore s;
  habitui::FakeClock c;
  s.config = kConfig;
  p.reason = WakeReason::ColdBoot;
  HabitInkController ctrl(makeHal(d, b, p, s, c));
  ctrl.begin();
  CHECK(d.anyInk());
  CHECK(d.lastFull);           // cold boot = full refresh
  CHECK_EQ(d.flushes, 1);
}

TEST(controller_button_wake_uses_fast_refresh) {
  MockDisplay d;
  MockButtons b;
  MockPower p;
  habitui::FakeStore s;
  habitui::FakeClock c;
  s.config = kConfig;
  p.reason = WakeReason::Button;
  HabitInkController ctrl(makeHal(d, b, p, s, c));
  ctrl.begin();
  CHECK(!d.lastFull);          // button wake = fast refresh
}

TEST(controller_toggle_logs_and_repaints) {
  MockDisplay d;
  MockButtons b;
  MockPower p;
  habitui::FakeStore s;
  habitui::FakeClock c;
  s.config = kConfig;
  HabitInkController ctrl(makeHal(d, b, p, s, c));
  ctrl.begin();
  const int flushesAfterBegin = d.flushes;

  b.queued.push_back(AppButton::Toggle);
  CHECK(ctrl.tick());
  CHECK(ctrl.app().doneToday(0));            // logged
  CHECK_EQ(s.recordCount(1), 1);             // appended to SD
  CHECK(d.flushes > flushesAfterBegin);      // repainted
}

TEST(controller_cycle_then_toggle_second_habit) {
  MockDisplay d;
  MockButtons b;
  MockPower p;
  habitui::FakeStore s;
  habitui::FakeClock c;
  s.config = kConfig;
  HabitInkController ctrl(makeHal(d, b, p, s, c));
  ctrl.begin();
  b.queued.push_back(AppButton::Next);
  b.queued.push_back(AppButton::Toggle);
  CHECK(ctrl.tick());
  CHECK_EQ(ctrl.app().selectedIndex(), 1);
  CHECK(ctrl.app().doneToday(1));
  CHECK_EQ(s.recordCount(2), 1);
}

TEST(controller_sleeps_on_power_hold) {
  MockDisplay d;
  MockButtons b;
  MockPower p;
  habitui::FakeStore s;
  habitui::FakeClock c;
  s.config = kConfig;
  HabitInkController ctrl(makeHal(d, b, p, s, c));
  ctrl.begin();
  b.holdPower = true;
  CHECK(!ctrl.tick());          // decided to sleep
  CHECK(ctrl.sleeping());
  CHECK_EQ(p.deepSleeps, 1);
  CHECK(d.lastFull);            // sleep face uses a full refresh
}

TEST(controller_sleeps_on_idle_timeout) {
  MockDisplay d;
  MockButtons b;
  MockPower p;
  habitui::FakeStore s;
  habitui::FakeClock c;
  s.config = kConfig;
  HabitInkController ctrl(makeHal(d, b, p, s, c), /*idleSleepMs=*/10000);
  ctrl.begin();
  // No input; advance the clock past the idle threshold.
  p.now = 15000;
  CHECK(!ctrl.tick());
  CHECK_EQ(p.deepSleeps, 1);
}

TEST(controller_activity_resets_idle_timer) {
  MockDisplay d;
  MockButtons b;
  MockPower p;
  habitui::FakeStore s;
  habitui::FakeClock c;
  s.config = kConfig;
  HabitInkController ctrl(makeHal(d, b, p, s, c), /*idleSleepMs=*/10000);
  ctrl.begin();
  p.now = 8000;
  b.queued.push_back(AppButton::Next);
  CHECK(ctrl.tick());           // activity at t=8000 resets timer
  p.now = 15000;                // 7000ms since activity: below threshold
  CHECK(ctrl.tick());
  CHECK_EQ(p.deepSleeps, 0);
}
