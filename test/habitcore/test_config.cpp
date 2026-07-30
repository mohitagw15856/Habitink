#include "check.h"
#include "habitcore/HabitConfig.h"

using namespace habitcore;

TEST(schedule_parse_and_serialise) {
  Schedule s;
  CHECK(Schedule::parseField("daily", s));
  CHECK(s.isDaily());
  CHECK_EQ(s.toField(), std::string("daily"));

  CHECK(Schedule::parseField("1111100", s));
  CHECK(s.isDue(Weekday::Mon));
  CHECK(s.isDue(Weekday::Fri));
  CHECK(!s.isDue(Weekday::Sat));
  CHECK(!s.isDue(Weekday::Sun));
  CHECK_EQ(s.dueDaysPerWeek(), 5);
  CHECK_EQ(s.toField(), std::string("1111100"));

  CHECK(Schedule::parseField("DAILY", s));  // case-insensitive
  CHECK(s.isDaily());
}

TEST(schedule_parse_rejects_bad) {
  Schedule s;
  CHECK(!Schedule::parseField("111110", s));   // too short
  CHECK(!Schedule::parseField("11111000", s));  // too long
  CHECK(!Schedule::parseField("111110x", s));   // bad char
  CHECK(!Schedule::parseField("weekly", s));
}

TEST(config_parse_basic) {
  const std::string text =
      "# habitink habits v1\n"
      "# id\ticon\tschedule\tname\n"
      "1\twater\tdaily\tDrink 2L water\n"
      "2\trun\t1111100\tMorning run\n"
      "\n"
      "3\tbook\t0000011\tWeekend reading\n";
  HabitConfig cfg;
  std::vector<ParseIssue> issues;
  CHECK(HabitConfig::parse(text, cfg, &issues));
  CHECK_EQ(cfg.size(), 3);
  CHECK(issues.empty());

  const Habit* run = cfg.byId(2);
  CHECK(run != nullptr);
  CHECK_EQ(run->name, std::string("Morning run"));
  CHECK_EQ(run->icon, std::string("run"));
  CHECK(run->schedule.isDue(Weekday::Mon));
  CHECK(!run->schedule.isDue(Weekday::Sun));

  const Habit* weekend = cfg.byId(3);
  CHECK(weekend != nullptr);
  CHECK(weekend->schedule.isDue(Weekday::Sat));
  CHECK(weekend->schedule.isDue(Weekday::Sun));
  CHECK(!weekend->schedule.isDue(Weekday::Mon));
}

TEST(config_reports_issues_but_keeps_good_lines) {
  const std::string text =
      "1\twater\tdaily\tGood\n"
      "99\trun\tdaily\tBadId\n"        // id out of range
      "2\trun\tnope\tBadSchedule\n"    // bad schedule
      "3\tbook\tdaily\t\n"             // empty name
      "4\tpen\tdaily\tAlsoGood\n";
  HabitConfig cfg;
  std::vector<ParseIssue> issues;
  CHECK(HabitConfig::parse(text, cfg, &issues));
  CHECK_EQ(cfg.size(), 2);
  CHECK_EQ(issues.size(), 3u);
  CHECK(cfg.byId(1) != nullptr);
  CHECK(cfg.byId(4) != nullptr);
}

TEST(config_rejects_duplicate_ids) {
  const std::string text =
      "1\twater\tdaily\tFirst\n"
      "1\trun\tdaily\tDuplicate\n";
  HabitConfig cfg;
  std::vector<ParseIssue> issues;
  HabitConfig::parse(text, cfg, &issues);
  CHECK_EQ(cfg.size(), 1);
  CHECK_EQ(issues.size(), 1u);
}

TEST(config_enforces_twelve_cap) {
  std::string text;
  for (int i = 1; i <= 15; ++i) {
    text += std::to_string(i) + "\tcheck\tdaily\tHabit " + std::to_string(i) + "\n";
  }
  HabitConfig cfg;
  std::vector<ParseIssue> issues;
  HabitConfig::parse(text, cfg, &issues);
  CHECK_EQ(cfg.size(), kMaxHabits);
}

TEST(config_roundtrip_serialise_parse) {
  HabitConfig cfg;
  Habit a;
  a.icon = "water";
  a.name = "Drink water";
  a.schedule = Schedule::daily();
  CHECK(cfg.add(a));
  Habit b;
  b.icon = "run";
  b.name = "Run 5k";
  Schedule s;
  Schedule::parseField("1010100", s);
  b.schedule = s;
  CHECK(cfg.add(b));

  CHECK_EQ(cfg.habits()[0].id, 1);
  CHECK_EQ(cfg.habits()[1].id, 2);

  const std::string text = cfg.serialise();
  HabitConfig reparsed;
  CHECK(HabitConfig::parse(text, reparsed));
  CHECK_EQ(reparsed.size(), 2);
  CHECK_EQ(reparsed.byId(2)->schedule.toField(), std::string("1010100"));
}

TEST(config_next_free_id) {
  HabitConfig cfg;
  Habit a;
  a.id = 1;
  a.name = "One";
  cfg.add(a);
  Habit c;
  c.id = 3;
  c.name = "Three";
  cfg.add(c);
  CHECK_EQ(cfg.nextFreeId(), 2);
}
