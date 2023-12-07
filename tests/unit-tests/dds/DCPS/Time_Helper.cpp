/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include "ace/Time_Value.h"

#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TimeTypes.h"

#include <iostream>

using namespace OpenDDS::DCPS;

const DDS::Duration_t infinite_duration = { ::DDS::DURATION_INFINITE_SEC, ::DDS::DURATION_INFINITE_NSEC };

TEST(dds_DCPS_Time_Helper, infinite_duration_to_time_value)
{
  const ACE_Time_Value tv = duration_to_time_value(infinite_duration);
  // see value.
  //time_t sec = tv.sec ();
  //suseconds_t usec = tv.usec ();
  //unsigned long msec = tv.msec ();

  //std::cout << "infinite sec and nsec convert to time value: sec="
  //          << sec << " usec=" << usec << " msec=" << msec << std::endl;

  EXPECT_TRUE(tv.sec() == ACE_Time_Value::max_time.sec()
              || tv.sec() == (time_t)(infinite_duration.sec  + infinite_duration.nanosec/1000/ACE_ONE_SECOND_IN_USECS));
  EXPECT_TRUE(tv.usec() == ACE_Time_Value::max_time.usec()
              || tv.usec() == (suseconds_t)(infinite_duration.nanosec/1000%ACE_ONE_SECOND_IN_USECS));
}

TEST(dds_DCPS_Time_Helper, infinite_duration_to_absolute_time_value)
{
  const ACE_Time_Value now = SystemTimePoint::now().value();
  const ACE_Time_Value tv = duration_to_absolute_time_value(infinite_duration, now);
  // see value.
  //time_t sec = tv.sec ();
  //suseconds_t usec = tv.usec ();
  //unsigned long msec = tv.msec ();

  //std::cout << "infinite sec and nsec convert to absolute time value: sec="
  //          << sec << " usec=" << usec << " msec=" << msec << std::endl;

  EXPECT_TRUE(tv.sec() == ACE_Time_Value::max_time.sec()
              || tv.sec() == infinite_duration.sec  + now.sec() + (time_t)((infinite_duration.nanosec/1000 + now.usec ())/ACE_ONE_SECOND_IN_USECS));
  EXPECT_TRUE(tv.usec() == ACE_Time_Value::max_time.usec()
              || tv.usec() == (suseconds_t)(infinite_duration.nanosec/1000 + now.usec ())%ACE_ONE_SECOND_IN_USECS);
}

TEST(dds_DCPS_Time_Helper, finite_duration_to_time_value)
{
  DDS::Duration_t duration;
  duration.sec = ::DDS::DURATION_INFINITE_SEC - 2;
  duration.nanosec = ::DDS::DURATION_INFINITE_NSEC;
  const ACE_Time_Value tv = duration_to_time_value(duration);
  // see value.
  //time_t sec = tv.sec ();
  //suseconds_t usec = tv.usec ();
  //unsigned long msec = tv.msec ();

  //std::cout << "finite sec convert to time value: sec="
  //          << sec << " usec=" << usec << " msec=" << msec << std::endl;

  EXPECT_TRUE(tv.sec() == duration.sec  + (time_t)(duration.nanosec/1000/ACE_ONE_SECOND_IN_USECS));
  EXPECT_TRUE(tv.usec() == (suseconds_t)(duration.nanosec/1000%ACE_ONE_SECOND_IN_USECS));
  EXPECT_TRUE(tv < ACE_Time_Value::max_time);
}

TEST(dds_DCPS_Time_Helper, Duration_t_difference)
{
  const DDS::Time_t tt1 = {3,1};
  const DDS::Time_t tt2 = {1,3};
  const DDS::Duration_t result1 = tt2 - tt1;
  const DDS::Duration_t result2 = tt1 - tt2;
  // std::cout << "tt2 - tt1 : " << result1.sec << " : " << result1.nanosec  << std::endl;
  // std::cout << "tt1 - tt2 : " << result2.sec << " : " << result2.nanosec << std::endl;
  EXPECT_TRUE(result1.sec == -2 && result1.nanosec == 2);
  EXPECT_TRUE(result2.sec == 1 && result2.nanosec == 999999998);
}

TEST(dds_DCPS_Time_Helper, MonotonicTime_t_equal)
{
  const MonotonicTime_t tt1 = {3,1};
  const MonotonicTime_t tt2 = {1,3};
  const MonotonicTime_t tt3 = {3,1};
  EXPECT_EQ(tt1, tt1);
  EXPECT_EQ(tt1, tt3);
  EXPECT_EQ(tt3, tt1);
  EXPECT_TRUE(!(tt1 == tt2));
  EXPECT_TRUE(!(tt2 == tt1));
}

TEST(dds_DCPS_Time_Helper, make_duration)
{
  const DDS::Duration_t d = make_duration_t(1, 2);
  EXPECT_EQ(d.sec, 1);
  EXPECT_EQ(d.nanosec, 2u);
}

TEST(dds_DCPS_Time_Helper, duration_to_dds_string)
{
  {
    const DDS::Duration_t d = { 1, 2 };
    const String s = to_dds_string(d);
    EXPECT_EQ(s, "1.000000002");
  }

  {
    const DDS::Duration_t d = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
    const String s = to_dds_string(d);
    EXPECT_EQ(s, "DURATION_INFINITY");
  }
}

TEST(dds_DCPS_Time_Helper, duration_from_dds_string)
{
  DDS::Duration_t x = { 0, 0 };
  EXPECT_TRUE(from_dds_string("1.000000002", x));
  EXPECT_TRUE(x == make_duration_t(1,2));

  EXPECT_TRUE(from_dds_string("1", x));
  EXPECT_TRUE(x == make_duration_t(1,0));

  EXPECT_TRUE(from_dds_string(".1", x));
  EXPECT_TRUE(x == make_duration_t(0, 100000000));

  EXPECT_FALSE(from_dds_string(".", x));
  EXPECT_FALSE(from_dds_string("not a duration", x));
}

TEST(dds_DCPS_Time_Helper, add_time_duration)
{
  const DDS::Time_t t = {1, 500000000};
  const DDS::Duration_t d = make_duration_t(1, 500000000);
  const DDS::Time_t r = t + d;
  EXPECT_EQ(r.sec, 3);
  EXPECT_EQ(r.nanosec, 0u);
}

TEST(dds_DCPS_Time_Helper, time_value_to_double)
{
  const ACE_Time_Value tv(1,500000);
  EXPECT_EQ(time_value_to_double(tv), 1.5);
}
