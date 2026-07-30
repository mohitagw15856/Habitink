#include <vector>

#include "check.h"
#include "habitui/HabitApp.h"
#include "habitui/fakes.h"

using namespace habitui;

static const char* kConfig =
    "# habitink habits v1\n"
    "1\twater\tdaily\tDrink water\n"
    "2\trun\t1111100\tMorning run\n"
    "3\tbook\tdaily\tRead\n";

static std::vector<uint8_t> panel() {
  return std::vector<uint8_t>(static_cast<size_t>(100) * 480, 0xFF);
}

TEST(app_loads_config) {
  FakeStore store;
  FakeClock clock;
  store.config = kConfig;
  HabitApp app(store, clock);
  app.load();
  CHECK(app.loaded());
  CHECK_EQ(app.habitCount(), 3);
  CHECK_EQ(app.selectedIndex(), 0);
  CHECK(app.screen() == Screen::Home);
}

TEST(app_cycle_wraps) {
  FakeStore store;
  FakeClock clock;
  store.config = kConfig;
  HabitApp app(store, clock);
  app.load();
  CHECK(app.handleButton(AppButton::Next));
  CHECK_EQ(app.selectedIndex(), 1);
  CHECK(app.handleButton(AppButton::Next));
  CHECK_EQ(app.selectedIndex(), 2);
  CHECK(app.handleButton(AppButton::Next));
  CHECK_EQ(app.selectedIndex(), 0);  // wrapped
  CHECK(app.handleButton(AppButton::Prev));
  CHECK_EQ(app.selectedIndex(), 2);  // wrapped back
}

TEST(app_toggle_appends_and_flips) {
  FakeStore store;
  FakeClock clock;
  store.config = kConfig;
  HabitApp app(store, clock);
  app.load();
  CHECK(!app.doneToday(0));
  CHECK(app.handleButton(AppButton::Toggle));
  CHECK(app.doneToday(0));
  CHECK_EQ(store.recordCount(1), 1);  // habit id 1

  // Toggling again appends a second (append-only) record and clears state.
  CHECK(app.handleButton(AppButton::Toggle));
  CHECK(!app.doneToday(0));
  CHECK_EQ(store.recordCount(1), 2);

  // The appended lines carry today's date and the right value.
  CHECK_EQ(store.logs[1].back(), std::string("2026-07-30\t0"));
}

TEST(app_toggle_persists_across_reload) {
  FakeStore store;
  FakeClock clock;
  store.config = kConfig;
  HabitApp app(store, clock);
  app.load();
  app.handleButton(AppButton::Toggle);  // mark habit 1 done

  HabitApp reloaded(store, clock);
  reloaded.load();
  CHECK(reloaded.doneToday(0));  // read back from the log
}

TEST(app_navigation_screens) {
  FakeStore store;
  FakeClock clock;
  store.config = kConfig;
  HabitApp app(store, clock);
  app.load();
  CHECK(app.handleButton(AppButton::Grid));
  CHECK(app.screen() == Screen::YearGrid);
  CHECK(app.handleButton(AppButton::Back));
  CHECK(app.screen() == Screen::Home);
  CHECK(app.handleButton(AppButton::Weekly));
  CHECK(app.screen() == Screen::Weekly);
  CHECK(app.handleButton(AppButton::Back));
  CHECK(app.screen() == Screen::Home);
  // Toggle is ignored while not on Home.
  app.handleButton(AppButton::Grid);
  CHECK(!app.handleButton(AppButton::Toggle));
}

TEST(app_renders_all_screens_without_crash) {
  FakeStore store;
  FakeClock clock;
  store.config = kConfig;
  // Seed a couple of completions so the grid has something to draw.
  store.appendLog(1, "2026-07-29\t1");
  store.appendLog(1, "2026-07-30\t1");
  HabitApp app(store, clock);
  app.load();

  auto buf = panel();
  FrameCanvas c(buf.data(), 800, 480, 100);

  auto anyInk = [&]() {
    for (size_t i = 0; i < buf.size(); ++i)
      if (buf[i] != 0xFF) return true;
    return false;
  };

  app.render(c);
  CHECK(anyInk());  // Home drew something

  app.handleButton(AppButton::Grid);
  app.render(c);
  CHECK(anyInk());

  app.handleButton(AppButton::Back);
  app.handleButton(AppButton::Weekly);
  app.render(c);
  CHECK(anyInk());

  app.renderSleepFace(c);
  CHECK(anyInk());
}

TEST(app_empty_config_is_safe) {
  FakeStore store;
  FakeClock clock;  // no config set
  HabitApp app(store, clock);
  app.load();
  CHECK_EQ(app.habitCount(), 0);
  CHECK(!app.handleButton(AppButton::Toggle));  // nothing to toggle
  auto buf = panel();
  FrameCanvas c(buf.data(), 800, 480, 100);
  app.render(c);            // must not crash
  app.renderSleepFace(c);   // must not crash
}

TEST(app_weekly_reads_current_week) {
  FakeStore store;
  FakeClock clock;
  store.config = kConfig;
  store.appendLog(2, "2026-07-27\t1");  // Monday of today's week
  HabitApp app(store, clock);
  app.load();
  app.handleButton(AppButton::Weekly);
  auto buf = panel();
  FrameCanvas c(buf.data(), 800, 480, 100);
  app.render(c);  // exercises buildWeekRow path
  CHECK(app.screen() == Screen::Weekly);
}
