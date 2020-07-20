#include <dds/DCPS/Serializer.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(serializer_test, Encoding__Encoding)
{
  Encoding enc;
  EXPECT_EQ(0,enc.to_string().compare(0,24,"CDR/XCDR1, little-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR1_ENDIAN_BIG)
{
  Encoding enc(Encoding::Kind::KIND_XCDR1,Endianness::ENDIAN_BIG);
  EXPECT_EQ(0,enc.to_string().compare(0,21,"CDR/XCDR1, big-endian"));
}

TEST(serializer_test, Encoding__Encoding_UNALIGNED_CDR_ENDIAN_BIG)
{
  Encoding enc(Encoding::Kind::KIND_UNALIGNED_CDR,Endianness::ENDIAN_BIG);
  EXPECT_EQ(0,enc.to_string().compare(0,25,"Unaligned CDR, big-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR2_ENDIAN_BIG)
{
  Encoding enc(Encoding::Kind::KIND_XCDR2,Endianness::ENDIAN_BIG);
  EXPECT_EQ(0,enc.to_string().compare(0,17,"XCDR2, big-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR1_ENDIAN_LITTLE)
{
  Encoding enc(Encoding::Kind::KIND_XCDR1,Endianness::ENDIAN_LITTLE);
  EXPECT_EQ(0,enc.to_string().compare(0,24,"CDR/XCDR1, little-endian"));
}

TEST(serializer_test, Encoding__Encoding_UNALIGNED_CDR_ENDIAN_LITTLE)
{
  Encoding enc(Encoding::Kind::KIND_UNALIGNED_CDR,Endianness::ENDIAN_LITTLE);
  EXPECT_EQ(0,enc.to_string().compare(0,28,"Unaligned CDR, little-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR2_ENDIAN_LITTLE)
{
  Encoding enc(Encoding::Kind::KIND_XCDR2,Endianness::ENDIAN_LITTLE);
  EXPECT_EQ(0,enc.to_string().compare(0,20,"XCDR2, little-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR2_Swap)
{
  Encoding enc(Encoding::Kind::KIND_XCDR2,true);
  EXPECT_EQ(0,enc.to_string().compare(0,17,"XCDR2, big-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR2_No_Swap)
{
  Encoding enc(Encoding::Kind::KIND_XCDR2,false);
  EXPECT_EQ(0,enc.to_string().compare(0,20,"XCDR2, little-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR1_max_align)
{
  Encoding enc(Encoding::Kind::KIND_XCDR1,Endianness::ENDIAN_BIG);
  EXPECT_EQ(8,enc.max_align());
}

TEST(serializer_test, Encoding__Encoding_UNALIGNED_CDR_max_align)
{
  Encoding enc(Encoding::Kind::KIND_UNALIGNED_CDR,Endianness::ENDIAN_BIG);
  EXPECT_EQ(0,enc.max_align());
}

TEST(serializer_test, Encoding__Encoding_XCDR2_max_align)
{
  Encoding enc(Encoding::Kind::KIND_XCDR2,Endianness::ENDIAN_BIG);
  EXPECT_EQ(4,enc.max_align());
}

TEST(serializer_test, align_value_no_offset)
{
  size_t value = 8;
  align(value,4);
  EXPECT_EQ(8,value);
}

TEST(serializer_test, align_value_add_offset)
{
  size_t value = 9;
  align(value,4);
  EXPECT_EQ(12,value);
}

TEST(serializer_test, align_value_smaller_than_by)
{
  size_t value = 4;
  align(value,9);
  EXPECT_EQ(9,value);
}

TEST(serializer_test, Encoding__is_encapsulated_this_XCDR1)
{
  Encoding enc(Encoding::Kind::KIND_XCDR1,Endianness::ENDIAN_BIG);
  EXPECT_TRUE(enc.is_encapsulated());
}

TEST(serializer_test, Encoding__is_encapsulated_this_UNALIGNED_CDR)
{
  Encoding enc(Encoding::Kind::KIND_UNALIGNED_CDR,Endianness::ENDIAN_LITTLE);
  EXPECT_FALSE(enc.is_encapsulated());
}

TEST(serializer_test, Encoding__Encoding_static_is_encacapsulatd_XCDR2)
{
  EXPECT_TRUE(Encoding::is_encapsulated(Encoding::Kind::KIND_XCDR2));
}
int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
