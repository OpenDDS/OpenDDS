#include <dds/DCPS/TimeDuration.h>

#include <gtest/gtest.h>

using OpenDDS::DCPS::TimeDuration;

TEST(dds_DCPS_TimeDuration, str)
{
  // Fractional precision is controlled by argument.
  const TimeDuration one_sec(1, 0);
  EXPECT_EQ(one_sec.str(), "1.000 s");
  EXPECT_EQ(one_sec.str(0), "1 s");
  EXPECT_EQ(one_sec.str(1), "1.0 s");
  EXPECT_EQ(one_sec.str(2), "1.00 s");
  EXPECT_EQ(one_sec.str(3), "1.000 s");
  EXPECT_EQ(TimeDuration(-1).str(0), "-1 s");
  EXPECT_EQ(TimeDuration(-1, -123000).str(3), "-1.123 s");
  EXPECT_EQ(TimeDuration(0, -123000).str(3), "-0.123 s");

  // Seconds turn into hours and minutes; Fields are padded if they're not
  // first.
  EXPECT_EQ(TimeDuration(61, 10000).str(2), "1:01.01");
  EXPECT_EQ(TimeDuration(60).str(0), "1:00");
  EXPECT_EQ(TimeDuration(61).str(0), "1:01");
  EXPECT_EQ(TimeDuration(119).str(0), "1:59");
  EXPECT_EQ(TimeDuration(60 * 60 + 1).str(0), "1:00:01");
  EXPECT_EQ(TimeDuration(61 * 60 + 1).str(0), "1:01:01");
  EXPECT_EQ(TimeDuration(119 * 60 + 1).str(0), "1:59:01");

  // Rounds fractional seconds depending on fractional precision.
  EXPECT_EQ(TimeDuration(0, 900000).str(0), "1 s");
  EXPECT_EQ(TimeDuration(0, 90000).str(1), "0.1 s");
  EXPECT_EQ(TimeDuration(0, 9000).str(2), "0.01 s");
  EXPECT_EQ(TimeDuration(0, 900000).str(1), "0.9 s");
  EXPECT_EQ(TimeDuration(0, 990000).str(1), "1.0 s");
  EXPECT_EQ(TimeDuration(0, 990000).str(2), "0.99 s");
  EXPECT_EQ(TimeDuration(0, 999000).str(2), "1.00 s");
  EXPECT_EQ(TimeDuration(0, 999000).str(3), "0.999 s");
  EXPECT_EQ(TimeDuration(0, 999000).str(4), "0.9990 s");
  EXPECT_EQ(TimeDuration(0, 100000).str(0), "0 s");
  EXPECT_EQ(TimeDuration(0, 100000).str(1), "0.1 s");

  // Full Carry depending on fractional precision.
  const TimeDuration almost_two_hours(1 * 60 * 60 + 59 * 60 + 59, 900000);
  EXPECT_EQ(almost_two_hours.str(1), "1:59:59.9");
  EXPECT_EQ(almost_two_hours.str(0), "2:00:00");

  // Hours are the highest unit
  EXPECT_EQ(TimeDuration(100 * 60 * 60).str(0), "100:00:00");

  // just_sec and sec_str force just seconds
  EXPECT_EQ(TimeDuration(100).str(0, true), "100 s");
  EXPECT_EQ(TimeDuration(100).sec_str(0), "100 s");
}

TEST(dds_DCPS_TimeDuration, double_ctor)
{
  const TimeDuration td = TimeDuration::from_double(1.1);
  EXPECT_EQ(td.value().sec(), 1);
  EXPECT_EQ(td.value().usec(), 100000);
}
