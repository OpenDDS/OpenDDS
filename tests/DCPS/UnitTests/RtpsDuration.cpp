/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/Service_Participant.h"

#include "dds/DCPS/RTPS/BaseMessageUtils.h"

#include "../common/TestSupport.h"

#include <iostream>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  const VendorId_t SomeoneElse = { { '\x04', '\x02' } };

  try
  {
    {
      Duration_t duration = { 7, 0x80000000 };
      TimeDuration result = rtps_duration_to_time_duration(duration, PROTOCOLVERSION_2_4, VENDORID_OPENDDS);
      TEST_CHECK(result.value().sec() == 7);
      TEST_CHECK(result.value().usec() == 500000);
    }
    {
      Duration_t duration = { 0, 0x10000000 };
      TimeDuration result = rtps_duration_to_time_duration(duration, PROTOCOLVERSION_2_4, VENDORID_OPENDDS);
      TEST_CHECK(result.value().sec() == 0);
      TEST_CHECK(result.value().usec() == 62500);
    }
    {
      Duration_t duration = { 0, 0x08000000 };
      TimeDuration result = rtps_duration_to_time_duration(duration, PROTOCOLVERSION_2_3, SomeoneElse);
      TEST_CHECK(result.value().sec() == 0);
      TEST_CHECK(result.value().usec() == 31250);
    }
    {
      Duration_t duration = { 0, 967000 };
      TimeDuration result = rtps_duration_to_time_duration(duration, PROTOCOLVERSION_2_3, VENDORID_OPENDDS);
      TEST_CHECK(result.value().sec() == 0);
      TEST_CHECK(result.value().usec() == 967);
    }
  }
  catch (...) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Exception Caught")));
    return 1;
  }

  return 0;
}
