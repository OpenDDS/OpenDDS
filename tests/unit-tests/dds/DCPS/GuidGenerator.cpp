/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include "dds/DCPS/RTPS/GuidGenerator.h"

using namespace OpenDDS::RTPS;
using namespace OpenDDS::DCPS;

int compare_prefix(GUID_t &g1, GUID_t &g2)
{
  for(size_t i = 0; i < sizeof(g1.guidPrefix); i++)
  {
    if(g1.guidPrefix[i] != g2.guidPrefix[i])
      return g1.guidPrefix[i] < g2.guidPrefix[i] ? -1 : 1;
  }
  return 0;
}

bool not_null(GUID_t &g1)
{
  for(size_t i = 2; i < sizeof(g1.guidPrefix); i++)
  {
    if(g1.guidPrefix[i] != 0)
      return true;
  }
  return false;
}

TEST(dds_DCPS_RTPS_GuidGenerator, not_NULL)
{
  GuidGenerator gen;
  GUID_t g;

  gen.populate(g);
  EXPECT_TRUE(not_null(g));
}

TEST(dds_DCPS_RTPS_GuidGenerator, populate)
{
  // Test GUID uniqueness
  GuidGenerator gen;
  GUID_t g1, g2;

  gen.populate(g1);
  gen.populate(g2);
  EXPECT_TRUE(compare_prefix(g1, g2) != 0);
}

TEST(dds_DCPS_RTPS_GuidGenerator, getCount)
{
  // Test GUID uniqueness
  GuidGenerator gen;
  gen.testInitCount();
  GUID_t g1, g2, g3, g4, g5;

  int zeroCount = gen.testCount(); // this will also increment the count, so after this, count -> 1
  gen.populate(g1); // count -> 2
  int twoCount = gen.testCount();// this will also increment the count, so after this, count -> 3
  gen.populate(g2); // count -> 4
  gen.populate(g3); // count -> 5
  gen.populate(g4); // count -> 6
  gen.populate(g5); // count -> 7
  int sevenCount = gen.testCount(); // count -> 8

  EXPECT_EQ(0, zeroCount);
  EXPECT_EQ(2, twoCount);
  EXPECT_EQ(7, sevenCount);
}

TEST(dds_DCPS_RTPS_GuidGenerator, interfaceName)
{
  // Test interfaceName shortcut
  GuidGenerator gen;
  int val1 = gen.interfaceName("Test_NotARealInterface");
  int val2 = gen.interfaceName("Test_NotARealInterface"); // synthesize a shortcut "found" that is a false success

  EXPECT_EQ(val1, -1); // This is a valid failure
  EXPECT_EQ(val2, 0); // but methinks the shortcut is a bug
}
