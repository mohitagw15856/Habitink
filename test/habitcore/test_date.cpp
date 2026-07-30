#include "check.h"
#include "habitcore/Date.h"

using namespace habitcore;

TEST(date_epoch_day_zero) {
  Date epoch(1970, 1, 1);
  CHECK_EQ(epoch.toDayNumber(), 0);
  CHECK(epoch.weekday() == Weekday::Thu);
}

TEST(date_roundtrip_day_number) {
  for (int32_t dn = -3000; dn <= 60000; dn += 37) {
    Date d = Date::fromDayNumber(dn);
    CHECK_EQ(d.toDayNumber(), dn);
  }
}

TEST(date_known_weekdays) {
  CHECK(Date(2026, 7, 30).weekday() == Weekday::Thu);
  CHECK(Date(2000, 1, 1).weekday() == Weekday::Sat);
  CHECK(Date(2026, 1, 1).weekday() == Weekday::Thu);
  CHECK(Date(2024, 2, 29).weekday() == Weekday::Thu);  // leap day
}

TEST(date_add_days_across_year) {
  Date d(2026, 12, 31);
  CHECK(d.addDays(1) == Date(2027, 1, 1));
  CHECK(Date(2024, 2, 28).addDays(1) == Date(2024, 2, 29));  // leap year
  CHECK(Date(2025, 2, 28).addDays(1) == Date(2025, 3, 1));   // non-leap
}

TEST(date_iso_format_and_parse) {
  CHECK_EQ(Date(2026, 7, 30).toIso(), std::string("2026-07-30"));
  CHECK_EQ(Date(2026, 1, 5).toIso(), std::string("2026-01-05"));
  Date d;
  CHECK(Date::parseIso("2026-07-30", d));
  CHECK(d == Date(2026, 7, 30));
}

TEST(date_parse_rejects_malformed) {
  Date d;
  CHECK(!Date::parseIso("2026/07/30", d));
  CHECK(!Date::parseIso("2026-7-30", d));
  CHECK(!Date::parseIso("not-a-date", d));
  CHECK(!Date::parseIso("2026-13-01", d));
  CHECK(!Date::parseIso("", d));
}

TEST(date_validity) {
  CHECK(Date(2026, 2, 28).isValid());
  CHECK(!Date(2026, 2, 30).isValid());  // normalises away, so invalid
  CHECK(!Date(2026, 13, 1).isValid());
  CHECK(Date(2024, 2, 29).isValid());
}
