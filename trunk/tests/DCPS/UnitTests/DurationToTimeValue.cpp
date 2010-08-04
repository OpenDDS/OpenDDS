/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"
#include "ace/OS.h"
#include "ace/Time_Value.h"

#include "dds/DCPS/Qos_Helper.h"

#include "../common/TestSupport.h"

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
    time_t sec = tv.sec ();
    suseconds_t usec = tv.usec ();
    unsigned long msec = tv.msec ();
    TEST_CHECK (tv.sec() == ACE_Time_Value::max_time.sec());
    TEST_CHECK (tv.usec() == ACE_Time_Value::max_time.usec());

    ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t)infinite sec and nsec convert to time value:")
                          ACE_TEXT (" sec %d usec %d msec %u\n"), 
      sec, usec, msec));
  }
  
  {
    ACE_Time_Value tv = duration_to_absolute_time_value(duration);
    // see value.
    time_t sec = tv.sec ();
    suseconds_t usec = tv.usec ();
    unsigned long msec = tv.msec ();
    TEST_CHECK (tv.sec() == ACE_Time_Value::max_time.sec());
    TEST_CHECK (tv.usec() == ACE_Time_Value::max_time.usec());
    
    ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t)infinite sec and nsec convert to absolute ")
                          ACE_TEXT ("time value: sec %d usec %d msec %u\n"), 
      sec, usec, msec));
  }
  {
    duration.sec = ::DDS::DURATION_INFINITE_SEC - 2;
    duration.nanosec = ::DDS::DURATION_INFINITE_NSEC;
    ACE_Time_Value tv = duration_to_time_value(duration);
    // see value.
    time_t sec = tv.sec ();
    suseconds_t usec = tv.usec ();
    unsigned long msec = tv.msec ();
    TEST_CHECK (tv.sec() == ::DDS::DURATION_INFINITE_SEC);
    TEST_CHECK (tv.usec() == ::DDS::DURATION_INFINITE_NSEC/1000%ACE_ONE_SECOND_IN_USECS);
    TEST_CHECK (tv < ACE_Time_Value::max_time);
    ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t)finite sec and infinite nsec ")
                          ACE_TEXT ("conversion: sec %d usec %d msec %u\n"), 
      sec, usec, msec));

  }
  return 0;
}
