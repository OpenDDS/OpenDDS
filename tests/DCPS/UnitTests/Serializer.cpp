#include <dds/DCPS/Serializer.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(serializer_test, Encoding__Encoding)
{
  Encoding enc;
  EXPECT_EQ(0, enc.to_string().compare(0, 24, "CDR/XCDR1, little-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR1_ENDIAN_BIG)
{
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  EXPECT_EQ(0, enc.to_string().compare(0, 21, "CDR/XCDR1, big-endian"));
}

TEST(serializer_test, Encoding__Encoding_UNALIGNED_CDR_ENDIAN_BIG)
{
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_BIG);
  EXPECT_EQ(0, enc.to_string().compare(0, 25, "Unaligned CDR, big-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR2_ENDIAN_BIG)
{
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
  EXPECT_EQ(0, enc.to_string().compare(0, 17, "XCDR2, big-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR1_ENDIAN_LITTLE)
{
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
}

TEST(serializer_test, Encoding__Encoding_UNALIGNED_CDR_ENDIAN_LITTLE)
{
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EXPECT_EQ(0, enc.to_string().compare(0, 28, "Unaligned CDR, little-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR2_ENDIAN_LITTLE)
{
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  EXPECT_EQ(0, enc.to_string().compare(0, 20, "XCDR2, little-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR2_Swap)
{
  Encoding enc(Encoding::KIND_XCDR2, true);
  EXPECT_EQ(0, enc.to_string().compare(0, 17, "XCDR2, big-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR2_No_Swap)
{
  Encoding enc(Encoding::KIND_XCDR2, false);
  EXPECT_EQ(0, enc.to_string().compare(0, 20, "XCDR2, little-endian"));
}

TEST(serializer_test, Encoding__Encoding_XCDR1_max_align)
{
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  EXPECT_EQ(8, enc.max_align());
}

TEST(serializer_test, Encoding__Encoding_UNALIGNED_CDR_max_align)
{
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_BIG);
  EXPECT_EQ(0, enc.max_align());
}

TEST(serializer_test, Encoding__Encoding_XCDR2_max_align)
{
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
  EXPECT_EQ(4, enc.max_align());
}

TEST(serializer_test, align_value_no_offset)
{
  size_t value = 8;
  align(value, 4);
  EXPECT_EQ(8, value);
}

TEST(serializer_test, align_value_add_offset)
{
  size_t value = 9;
  align(value, 4);
  EXPECT_EQ(12, value);
}

TEST(serializer_test, align_value_smaller_than_by)
{
  size_t value = 4;
  align(value, 9);
  EXPECT_EQ(9, value);
}

TEST(serializer_test, Encoding__is_encapsulated_this_XCDR1)
{
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  EXPECT_TRUE(enc.is_encapsulated());
}

TEST(serializer_test, Encoding__is_encapsulated_this_UNALIGNED_CDR)
{
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EXPECT_FALSE(enc.is_encapsulated());
}

TEST(serializer_test, Encoding__Encoding_static_is_encacapsulatd_XCDR2)
{
  EXPECT_TRUE(Encoding::is_encapsulated(Encoding::KIND_XCDR2));
}

TEST(serializer_test, EncapsulationHeader__EncapsulationHeader)
{
  EncapsulationHeader eh;
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__from_encoding_XCDR1_BIG_FINAL)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  EXPECT_TRUE(eh.from_encoding(enc, FINAL));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__from_encoding_XCDR1_LITTLE_FINAL)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
  EXPECT_TRUE(eh.from_encoding(enc, FINAL));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Plain", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__from_encoding_XCDR1_LITTLE_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
  EXPECT_TRUE(eh.from_encoding(enc, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Parameter List", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__from_encoding_XCDR2_LITTLE_FINAL)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  EXPECT_TRUE(eh.from_encoding(enc, FINAL));
  EXPECT_STREQ("XCDR2 Little Endian Plain", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__from_encoding_XCDR2_LITTLE_APPENDABLE)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  EXPECT_TRUE(eh.from_encoding(enc, APPENDABLE));
  EXPECT_STREQ("XCDR2 Little Endian Delimited", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__from_encoding_XCDR2_LITTLE_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  EXPECT_TRUE(eh.from_encoding(enc, MUTABLE));
  EXPECT_STREQ("XCDR2 Little Endian Parameter List", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__from_encoding_UNALIGNED_CDR_LITTLE_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EXPECT_FALSE(eh.from_encoding(enc, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc;
  EXPECT_FALSE(eh.to_encoding(enc, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_NOT_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, FINAL));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_CDR_LE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_CDR_LE);
  Encoding enc;
  EXPECT_FALSE(eh.to_encoding(enc, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Plain", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_CDR_LE_NOT_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_CDR_LE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, FINAL));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Plain", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_PL_CDR_BE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_PL_CDR_BE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Parameter List", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_PL_CDR_LE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_PL_CDR_LE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc,MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Parameter List", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_CDR2_BE_FINAL)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_CDR2_BE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc,FINAL));
  EXPECT_STREQ("XCDR2 Big Endian Plain", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_CDR2_LE_FINAL)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_CDR2_LE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, FINAL));
  EXPECT_STREQ("XCDR2 Little Endian Plain", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_D_CDR2_BE_APPENDABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_D_CDR2_BE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, APPENDABLE));
  EXPECT_STREQ("XCDR2 Big Endian Delimited", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_D_CDR2_LE_APPENDABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_D_CDR2_LE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, APPENDABLE));
  EXPECT_STREQ("XCDR2 Little Endian Delimited", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_PL_CDR2_BE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_PL_CDR2_BE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, MUTABLE));
  EXPECT_STREQ("XCDR2 Big Endian Parameter List", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_PL_CDR2_LE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_PL_CDR2_LE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, MUTABLE));
  EXPECT_STREQ("XCDR2 Little Endian Parameter List", eh.to_string().c_str());
}

TEST(serializer_test, EncapsulationHeader__to_encoding_XML)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_XML);
  Encoding enc;
  EXPECT_FALSE(eh.to_encoding(enc, MUTABLE));
  EXPECT_STREQ("XML", eh.to_string().c_str());
}

TEST(serializer_test, Serializer__Serializer_ACE_Message_Block_Encoding)
{
  ACE_Message_Block amb;
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  Serializer ser(&amb, enc);

  EXPECT_TRUE(ser.good_bit());
  EXPECT_EQ(0, ser.encoding().to_string().compare(0, 28, "Unaligned CDR, little-endian"));
}

TEST(serializer_test, Serializer__Serializer_ACE_Message_Block_Kind)
{
  ACE_Message_Block amb;
  Serializer ser(&amb, Encoding::KIND_XCDR1);

  EXPECT_TRUE(ser.good_bit());
  EXPECT_EQ(0, ser.encoding().to_string().compare(0, 24, "CDR/XCDR1, little-endian"));
}

TEST(serializer_test, Serializer__Serializer_ACE_Message_Block_Kind_bool)
{
  ACE_Message_Block amb;
  Serializer ser(&amb, Encoding::KIND_XCDR2);

  EXPECT_TRUE(ser.good_bit());
  EXPECT_EQ(0, ser.encoding().to_string().compare(0, 20, "XCDR2, little-endian"));
}

TEST(serializer_test, Serializer__set_endianness)
{
  ACE_Message_Block amb;
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  Serializer ser(&amb, enc);

  EXPECT_TRUE(ser.good_bit());
  EXPECT_EQ(0, ser.encoding().to_string().compare(0, 28, "Unaligned CDR, little-endian"));

  ser.endianness(ENDIAN_BIG);
  EXPECT_EQ(0, ser.encoding().to_string().compare(0, 25, "Unaligned CDR, big-endian"));
}

TEST(serializer_test, Serializer__swap_bytes_endianness)
{
  ACE_Message_Block amb;
  Encoding enc;
  Serializer ser(&amb, enc);

  ser.swap_bytes(true);
  EXPECT_EQ(ser.endianness(), ENDIAN_NONNATIVE);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  DCPS_debug_level = 0;
  return RUN_ALL_TESTS();
}
