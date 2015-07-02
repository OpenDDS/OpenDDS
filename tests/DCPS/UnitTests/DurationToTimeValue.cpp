/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"
#include "ace/Time_Value.h"

#include "dds/DCPS/Qos_Helper.h"

#include "../common/TestSupport.h"

#include <iostream>

using namespace OpenDDS::DCPS;

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  DDS::Duration_t duration;
  duration.sec = ::DDS::DURATION_INFINITE_SEC;
  duration.nanosec = ::DDS::DURATION_INFINITE_NSEC;
  {
    ACE_Time_Value tv = duration_to_time_value(duration);
    // see value.
    //time_t sec = tv.sec ();
    //suseconds_t usec = tv.usec ();
    //unsigned long msec = tv.msec ();

    //std::cout << "infinite sec and nsec convert to time value: sec="
    //          << sec << " usec=" << usec << " msec=" << msec << std::endl;

    TEST_CHECK (tv.sec() == ACE_Time_Value::max_time.sec()
             || tv.sec() == (time_t)(duration.sec  + duration.nanosec/1000/ACE_ONE_SECOND_IN_USECS));
    TEST_CHECK (tv.usec() == ACE_Time_Value::max_time.usec()
             || tv.usec() == (suseconds_t)(duration.nanosec/1000%ACE_ONE_SECOND_IN_USECS));
  }

  {
    ACE_Time_Value now = ACE_OS::gettimeofday ();
    ACE_Time_Value tv = duration_to_absolute_time_value(duration, now);
    // see value.
    //time_t sec = tv.sec ();
    //suseconds_t usec = tv.usec ();
    //unsigned long msec = tv.msec ();

    //std::cout << "infinite sec and nsec convert to absolute time value: sec="
    //          << sec << " usec=" << usec << " msec=" << msec << std::endl;

    TEST_CHECK (tv.sec() == ACE_Time_Value::max_time.sec()
             || tv.sec() == duration.sec  + now.sec() + (time_t)((duration.nanosec/1000 + now.usec ())/ACE_ONE_SECOND_IN_USECS));
    TEST_CHECK (tv.usec() == ACE_Time_Value::max_time.usec()
             || tv.usec() == (suseconds_t)(duration.nanosec/1000 + now.usec ())%ACE_ONE_SECOND_IN_USECS);
  }
  {
    duration.sec = ::DDS::DURATION_INFINITE_SEC - 2;
    duration.nanosec = ::DDS::DURATION_INFINITE_NSEC;
    ACE_Time_Value tv = duration_to_time_value(duration);
    // see value.
    //time_t sec = tv.sec ();
    //suseconds_t usec = tv.usec ();
    //unsigned long msec = tv.msec ();

    //std::cout << "finite sec convert to time value: sec="
    //          << sec << " usec=" << usec << " msec=" << msec << std::endl;

    TEST_CHECK (tv.sec() == duration.sec  + (time_t)(duration.nanosec/1000/ACE_ONE_SECOND_IN_USECS));
    TEST_CHECK (tv.usec() == (suseconds_t)(duration.nanosec/1000%ACE_ONE_SECOND_IN_USECS));
    TEST_CHECK (tv < ACE_Time_Value::max_time);
  }
  return 0;
}
