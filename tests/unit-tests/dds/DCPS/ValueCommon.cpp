/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/ValueCommon.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_ValueCommon, ListEnumHelper)
{
  ListEnumHelper::Pair pairs[] = {
    {"MONDAY", 2},
    {"TUESDAY", 3},
    {"WEDNESDAY", 4},
    {"THURSDAY", 5},
    {"FRIDAY", 6},
    {"SATURDAY", 7},
    {"SUNDAY", 8},
    {0, 0}
  };
  ListEnumHelper helper(pairs);

  ACE_CDR::Long value = 0;
  EXPECT_TRUE(helper.get_value(value, "MONDAY"));
  EXPECT_EQ(2, value);
  EXPECT_TRUE(helper.get_value(value, "FRIDAY"));
  EXPECT_EQ(6, value);
  EXPECT_TRUE(helper.get_value(value, "WEDNESDAY"));
  EXPECT_EQ(4, value);
  EXPECT_FALSE(helper.get_value(value, "SOMEDAY"));

  const char* name = 0;
  EXPECT_TRUE(helper.get_name(name, 3));
  EXPECT_STREQ("TUESDAY", name);
  EXPECT_TRUE(helper.get_name(name, 5));
  EXPECT_STREQ("THURSDAY", name);
  EXPECT_FALSE(helper.get_name(name, 10));
}

TEST(dds_DCPS_ValueCommon, MapBitmaskHelper)
{
  MapBitmaskHelper::Pair pairs[] = {
    {"FLAG0", 0},
    {"FLAG1", 1},
    {"FLAG2", 2},
    {"FLAG4", 4},
    {"FLAG5", 5},
    {"FLAG6", 6},
    {0, 0}
  };
  MapBitmaskHelper helper(pairs, 32, OpenDDS::XTypes::TK_UINT32);

  OPENDDS_VECTOR(String) names(3);
  names[0] = "FLAG1";
  names[1] = "FLAG4";
  names[2] = "FLAG5"; // 00110010 0x32

  ACE_CDR::ULongLong value = 0;
  EXPECT_TRUE(helper.get_value(value, names));
  EXPECT_EQ(0x32ull, value);

  names.push_back("FLAG3");
  EXPECT_TRUE(helper.get_value(value, names));
  EXPECT_EQ(0x32ull, value);

  value = 0x65;
  EXPECT_GT(helper.get_names(names, value), (size_t)0);
  EXPECT_EQ(names.size(), (size_t)4);
  EXPECT_EQ("FLAG0", names[0]);
  EXPECT_EQ("FLAG2", names[1]);
  EXPECT_EQ("FLAG5", names[2]);
  EXPECT_EQ("FLAG6", names[3]);
}
