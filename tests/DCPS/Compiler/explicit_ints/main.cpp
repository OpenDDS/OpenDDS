#include <testTypeSupportImpl.h>

#include <dds/DCPS/Definitions.h>

#include <gtest/gtest.h>

#if OPENDDS_HAS_EXPLICIT_INTS
TEST(ExplicitInts, min_max)
{
  EXPECT_EQ(u8_max, 255);
  EXPECT_EQ(i8_min, -128);
  EXPECT_EQ(i8_max, 127);
  EXPECT_EQ(u16_max, 65535);
  EXPECT_EQ(i16_min, -32768);
  EXPECT_EQ(i16_max, 32767);
  EXPECT_EQ(u32_max, 4294967295);
  EXPECT_EQ(i32_min, -2147483647 - 1);
  EXPECT_EQ(i32_max, 2147483647);
  EXPECT_EQ(u64_max, 18446744073709551615ULL);
  EXPECT_EQ(i64_min, -9223372036854775807 - 1);
  EXPECT_EQ(i64_max, 9223372036854775807);
}
#endif

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
