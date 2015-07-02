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
  {
    DDS::Time_t tt1 = {3,1};
    DDS::Time_t tt2 = {1,3};
    DDS::Time_t result1 = tt2 - tt1;
    DDS::Time_t result2 = tt1 - tt2;
    // std::cout << "tt2 - tt1 : " << result1.sec << " : " << result1.nanosec  << std::endl;
    // std::cout << "tt1 - tt2 : " << result2.sec << " : " << result2.nanosec << std::endl;
    TEST_CHECK (result1.sec == -2 && result1.nanosec == 2);
    TEST_CHECK (result2.sec == 1 && result2.nanosec == 999999998);
  }

  return 0;
}
