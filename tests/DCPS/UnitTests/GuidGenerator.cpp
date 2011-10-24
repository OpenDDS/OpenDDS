/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/RTPS/GuidGenerator.h"

#include "../common/TestSupport.h"

using namespace OpenDDS::RTPS;
using namespace OpenDDS::DCPS;

int compare_prefix (GUID_t &g1, GUID_t &g2)
{
  for (size_t i = 0; i < sizeof(g1.guidPrefix); i++) {
    if (g1.guidPrefix[i] != g2.guidPrefix[i])
      return g1.guidPrefix[i] < g2.guidPrefix[i] ? -1 : 1;
  }
  return 0;
}

bool not_null (GUID_t &g1)
{
  for (size_t i = 2; i < sizeof(g1.guidPrefix); i++) {
    if (g1.guidPrefix[i] != 0)
      return true;
  }
  return false;
}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{

  // Test GUID uniqueness
  {
    GuidGenerator gen;
    GUID_t g1, g2;

    gen.populate(g1);
    gen.populate(g2);
    TEST_CHECK(not_null(g1));
    TEST_CHECK(compare_prefix(g1,g2) != 0);
  }

  return 0;
}
