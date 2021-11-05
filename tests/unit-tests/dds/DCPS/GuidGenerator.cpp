/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include "dds/DCPS/RTPS/GuidGenerator.h"
#include "dds/DCPS/GuidConverter.h"

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

TEST(dds_DCPS_GuidGenerator, maintest)
{

  // Test GUID uniqueness
  {
    GuidGenerator gen;
    GUID_t g1, g2;

    gen.populate(g1);
    gen.populate(g2);
    EXPECT_TRUE(not_null(g1));
    EXPECT_TRUE(compare_prefix(g1,g2) != 0);
  }

  // Test GUID converter
  {
    GUID_t guid;
    guid.guidPrefix[ 0] =  1;
    guid.guidPrefix[ 1] =  2;
    guid.guidPrefix[ 2] =  3;
    guid.guidPrefix[ 3] =  4;
    guid.guidPrefix[ 4] =  5;
    guid.guidPrefix[ 5] =  6;
    guid.guidPrefix[ 6] =  7;
    guid.guidPrefix[ 7] =  8;
    guid.guidPrefix[ 8] =  9;
    guid.guidPrefix[ 9] = 10;
    guid.guidPrefix[10] = 11;
    guid.guidPrefix[11] = 12;
    guid.entityId.entityKey[0] = guid.entityId.entityKey[1] = guid.entityId.entityKey[2] = 0;
    guid.entityId.entityKind = 0;
    EXPECT_TRUE(GuidConverter(guid).uniqueParticipantId() == "0102030405060708090a0b0c");

    guid.guidPrefix[2] = 233;
    guid.guidPrefix[4] = 244;
    guid.guidPrefix[6] = 255;
    EXPECT_TRUE(GuidConverter(guid).uniqueParticipantId() == "0102e904f406ff08090a0b0c");
  }
}
