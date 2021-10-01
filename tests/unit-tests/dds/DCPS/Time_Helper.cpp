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

TEST(dds_DCPS_Time_Helper, maintest)
{
  DDS::Duration_t duration;
  duration.sec = ::DDS::DURATION_INFINITE_SEC;
  duration.nanosec = ::DDS::DURATION_INFINITE_NSEC;
  {
    const ACE_Time_Value tv = duration_to_time_value(duration);
    // see value.
    //time_t sec = tv.sec ();
    //suseconds_t usec = tv.usec ();
    //unsigned long msec = tv.msec ();

    //std::cout << "infinite sec and nsec convert to time value: sec="
    //          << sec << " usec=" << usec << " msec=" << msec << std::endl;

    EXPECT_TRUE (tv.sec() == ACE_Time_Value::max_time.sec()
             || tv.sec() == (time_t)(duration.sec  + duration.nanosec/1000/ACE_ONE_SECOND_IN_USECS));
    EXPECT_TRUE (tv.usec() == ACE_Time_Value::max_time.usec()
             || tv.usec() == (suseconds_t)(duration.nanosec/1000%ACE_ONE_SECOND_IN_USECS));
  }

  {
    const ACE_Time_Value now = SystemTimePoint::now().value();
    const ACE_Time_Value tv = duration_to_absolute_time_value(duration, now);
    // see value.
    //time_t sec = tv.sec ();
    //suseconds_t usec = tv.usec ();
    //unsigned long msec = tv.msec ();

    //std::cout << "infinite sec and nsec convert to absolute time value: sec="
    //          << sec << " usec=" << usec << " msec=" << msec << std::endl;

    EXPECT_TRUE (tv.sec() == ACE_Time_Value::max_time.sec()
             || tv.sec() == duration.sec  + now.sec() + (time_t)((duration.nanosec/1000 + now.usec ())/ACE_ONE_SECOND_IN_USECS));
    EXPECT_TRUE (tv.usec() == ACE_Time_Value::max_time.usec()
             || tv.usec() == (suseconds_t)(duration.nanosec/1000 + now.usec ())%ACE_ONE_SECOND_IN_USECS);
  }
  {
    duration.sec = ::DDS::DURATION_INFINITE_SEC - 2;
    duration.nanosec = ::DDS::DURATION_INFINITE_NSEC;
    const ACE_Time_Value tv = duration_to_time_value(duration);
    // see value.
    //time_t sec = tv.sec ();
    //suseconds_t usec = tv.usec ();
    //unsigned long msec = tv.msec ();

    //std::cout << "finite sec convert to time value: sec="
    //          << sec << " usec=" << usec << " msec=" << msec << std::endl;

    EXPECT_TRUE (tv.sec() == duration.sec  + (time_t)(duration.nanosec/1000/ACE_ONE_SECOND_IN_USECS));
    EXPECT_TRUE (tv.usec() == (suseconds_t)(duration.nanosec/1000%ACE_ONE_SECOND_IN_USECS));
    EXPECT_TRUE (tv < ACE_Time_Value::max_time);
  }
  {
    DDS::Time_t tt1 = {3,1};
    DDS::Time_t tt2 = {1,3};
    DDS::Duration_t result1 = tt2 - tt1;
    DDS::Duration_t result2 = tt1 - tt2;
    // std::cout << "tt2 - tt1 : " << result1.sec << " : " << result1.nanosec  << std::endl;
    // std::cout << "tt1 - tt2 : " << result2.sec << " : " << result2.nanosec << std::endl;
    EXPECT_TRUE (result1.sec == -2 && result1.nanosec == 2);
    EXPECT_TRUE (result2.sec == 1 && result2.nanosec == 999999998);
  }
}
