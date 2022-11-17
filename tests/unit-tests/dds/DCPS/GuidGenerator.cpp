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


    int val1 = gen.interfaceName("Test_NotARealInterface");
    int val2 = gen.interfaceName("Test_NotARealInterface"); // synthesize a shortcut "found" that is a false success

    EXPECT_TRUE(val1 == -1); // This is a valid failure
    EXPECT_TRUE(val2 == 0); // but methinks the shortcut is a bug
}

  // Test GUID converter
  {
    GUID_t guid;
    guid.guidPrefix[ 0] = 0x01;
    guid.guidPrefix[ 1] = 0x02;
    guid.guidPrefix[ 2] = 0x03;
    guid.guidPrefix[ 3] = 0x04;
    guid.guidPrefix[ 4] = 0x05;
    guid.guidPrefix[ 5] = 0x06;
    guid.guidPrefix[ 6] = 0x07;
    guid.guidPrefix[ 7] = 0x08;
    guid.guidPrefix[ 8] = 0x09;
    guid.guidPrefix[ 9] = 0x0A;
    guid.guidPrefix[10] = 0x0B;
    guid.guidPrefix[11] = 0x0C;
    guid.entityId.entityKey[0] = guid.entityId.entityKey[1] = guid.entityId.entityKey[2] = 0;
    guid.entityId.entityKind = 0;
    EXPECT_TRUE(GuidConverter(guid).uniqueParticipantId() == "0102030405060708090a0b0c");

    guid.guidPrefix[2] = 0xE9;
    guid.guidPrefix[4] = 0xF4;
    guid.guidPrefix[6] = 0xFF;
    EXPECT_TRUE(GuidConverter(guid).uniqueParticipantId() == "0102e904f406ff08090a0b0c");
  }
}
