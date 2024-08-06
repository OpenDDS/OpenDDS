#include <dds/DCPS/SafetyProfileStreams.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_SafetyProfileStreams, to_dds_string_octet)
{
  const ACE_CDR::Octet nine = 9, ninetyNine = 99;
  EXPECT_EQ(to_dds_string(nine), "9");
  EXPECT_EQ(to_dds_string(nine, true), "09");
  EXPECT_EQ(to_dds_string(ninetyNine), "99");
  EXPECT_EQ(to_dds_string(ninetyNine, true), "63");
}

TEST(dds_DCPS_SafetyProfileStreams, to_dds_string_ushort)
{
  const unsigned short ninetyNine = 99, nineThousand = 9000, fiftyNineThousand = 59000;
  EXPECT_EQ(to_dds_string(ninetyNine), "99");
  EXPECT_EQ(to_dds_string(ninetyNine, true), "0063");
  EXPECT_EQ(to_dds_string(nineThousand), "9000");
  EXPECT_EQ(to_dds_string(nineThousand, true), "2328");
  EXPECT_EQ(to_dds_string(fiftyNineThousand), "59000");
  EXPECT_EQ(to_dds_string(fiftyNineThousand, true), "e678");
}

TEST(dds_DCPS_SafetyProfileStreams, to_dds_string_int)
{
  const int ninetyNine = 99, nineThousand = 9000, negativeNineThousand = -9000;
  EXPECT_EQ(to_dds_string(ninetyNine), "99");
  EXPECT_EQ(to_dds_string(nineThousand), "9000");
  EXPECT_EQ(to_dds_string(negativeNineThousand), "-9000");
}

TEST(dds_DCPS_SafetyProfileStreams, to_dds_string_uint)
{
  const unsigned int ninetyNine = 99, nineThousand = 9000, fiftyNineThousand = 59000;
  EXPECT_EQ(to_dds_string(ninetyNine), "99");
  EXPECT_EQ(to_dds_string(ninetyNine, true), "00000063");
  EXPECT_EQ(to_dds_string(nineThousand), "9000");
  EXPECT_EQ(to_dds_string(nineThousand, true), "00002328");
  EXPECT_EQ(to_dds_string(fiftyNineThousand), "59000");
  EXPECT_EQ(to_dds_string(fiftyNineThousand, true), "0000e678");
}

TEST(dds_DCPS_SafetyProfileStreams, to_dds_string_long)
{
  const long ninetyNine = 99, nineThousand = 9000, negativeNineThousand = -9000;
  EXPECT_EQ(to_dds_string(ninetyNine), "99");
  EXPECT_EQ(to_dds_string(nineThousand), "9000");
  EXPECT_EQ(to_dds_string(negativeNineThousand), "-9000");
}

TEST(dds_DCPS_SafetyProfileStreams, to_dds_string_ulong)
{
  const unsigned long ninetyNine = 99, nineThousand = 9000;
  EXPECT_EQ(to_dds_string(ninetyNine), "99");
  EXPECT_EQ(to_dds_string(nineThousand), "9000");
}

TEST(dds_DCPS_SafetyProfileStreams, to_dds_string_longlong)
{
  const long long ninetyNine = 99, nineThousand = 9000, negativeNineThousand = -9000;
  EXPECT_EQ(to_dds_string(ninetyNine), "99");
  EXPECT_EQ(to_dds_string(nineThousand), "9000");
  EXPECT_EQ(to_dds_string(negativeNineThousand), "-9000");
}

TEST(dds_DCPS_SafetyProfileStreams, to_dds_string_ulonglong)
{
  const unsigned long long ninetyNine = 99, nineThousand = 9000, fiftyNineThousand = 59000;
  EXPECT_EQ(to_dds_string(ninetyNine), "99");
  EXPECT_EQ(to_dds_string(ninetyNine, true), "0000000000000063");
  EXPECT_EQ(to_dds_string(nineThousand), "9000");
  EXPECT_EQ(to_dds_string(nineThousand, true), "0000000000002328");
  EXPECT_EQ(to_dds_string(fiftyNineThousand), "59000");
  EXPECT_EQ(to_dds_string(fiftyNineThousand, true), "000000000000e678");
}

TEST(dds_DCPS_SafetyProfileStreams, to_dds_string_double)
{
  EXPECT_EQ(to_dds_string(1.5), "1.5");
}
