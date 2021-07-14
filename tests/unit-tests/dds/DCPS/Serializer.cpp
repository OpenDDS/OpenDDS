#include <dds/DCPS/Serializer.h>

#include <gtest/gtest.h>

#include <cstring>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_Serializer, Encoding__Encoding)
{
  Encoding enc;
  EXPECT_EQ(0, enc.to_string().compare(0, 24, "CDR/XCDR1, little-endian"));
}

TEST(dds_DCPS_Serializer, Encoding__Encoding_XCDR1_ENDIAN_BIG)
{
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  EXPECT_EQ(0, enc.to_string().compare(0, 21, "CDR/XCDR1, big-endian"));
}

TEST(dds_DCPS_Serializer, Encoding__Encoding_UNALIGNED_CDR_ENDIAN_BIG)
{
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_BIG);
  EXPECT_EQ(0, enc.to_string().compare(0, 25, "Unaligned CDR, big-endian"));
}

TEST(dds_DCPS_Serializer, Encoding__Encoding_XCDR2_ENDIAN_BIG)
{
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
  EXPECT_EQ(0, enc.to_string().compare(0, 17, "XCDR2, big-endian"));
}

TEST(dds_DCPS_Serializer, Encoding__Encoding_XCDR1_ENDIAN_LITTLE)
{
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
}

TEST(dds_DCPS_Serializer, Encoding__Encoding_UNALIGNED_CDR_ENDIAN_LITTLE)
{
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EXPECT_EQ(0, enc.to_string().compare(0, 28, "Unaligned CDR, little-endian"));
}

TEST(dds_DCPS_Serializer, Encoding__Encoding_XCDR2_ENDIAN_LITTLE)
{
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  EXPECT_EQ(0, enc.to_string().compare(0, 20, "XCDR2, little-endian"));
}

TEST(dds_DCPS_Serializer, Encoding__Encoding_XCDR2_Swap)
{
  Encoding enc(Encoding::KIND_XCDR2, true);
  EXPECT_EQ(0, enc.to_string().compare(0, 17, "XCDR2, big-endian"));
}

TEST(dds_DCPS_Serializer, Encoding__Encoding_XCDR2_No_Swap)
{
  Encoding enc(Encoding::KIND_XCDR2, false);
  EXPECT_EQ(0, enc.to_string().compare(0, 20, "XCDR2, little-endian"));
}

TEST(dds_DCPS_Serializer, Encoding__Encoding_XCDR1_max_align)
{
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  EXPECT_EQ(size_t(8), enc.max_align());
}

TEST(dds_DCPS_Serializer, Encoding__Encoding_UNALIGNED_CDR_max_align)
{
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_BIG);
  EXPECT_EQ(size_t(0), enc.max_align());
}

TEST(dds_DCPS_Serializer, Encoding__Encoding_XCDR2_max_align)
{
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
  EXPECT_EQ(size_t(4), enc.max_align());
}

TEST(dds_DCPS_Serializer, align_value_no_offset)
{
  size_t value = 8;
  align(value, 4);
  EXPECT_EQ(size_t(8), value);
}

TEST(dds_DCPS_Serializer, align_value_add_offset)
{
  size_t value = 9;
  align(value, 4);
  EXPECT_EQ(size_t(12), value);
}

TEST(dds_DCPS_Serializer, align_value_smaller_than_by)
{
  size_t value = 4;
  align(value, 9);
  EXPECT_EQ(size_t(9), value);
}

TEST(dds_DCPS_Serializer, Encoding__is_encapsulated_this_XCDR1)
{
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  EXPECT_TRUE(enc.is_encapsulated());
}

TEST(dds_DCPS_Serializer, Encoding__is_encapsulated_this_UNALIGNED_CDR)
{
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EXPECT_FALSE(enc.is_encapsulated());
}

TEST(dds_DCPS_Serializer, Encoding__Encoding_static_is_encacapsulatd_XCDR2)
{
  EXPECT_TRUE(Encoding::is_encapsulated(Encoding::KIND_XCDR2));
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__EncapsulationHeader)
{
  EncapsulationHeader eh;
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__EncapsulationHeader_Encoding_Valid)
{
  Encoding initenc;
  EncapsulationHeader eh(initenc, FINAL);
  EXPECT_TRUE(eh.is_good());
  EXPECT_STREQ("CDR/XCDR1 Little Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__EncapsulationHeader_Encoding_Invalid)
{
  Encoding initenc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EncapsulationHeader eh(initenc, FINAL);
  EXPECT_FALSE(eh.is_good());
  EXPECT_STREQ("Invalid", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__from_encoding_XCDR1_BIG_FINAL)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  EXPECT_TRUE(eh.from_encoding(enc, FINAL));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__from_encoding_XCDR1_LITTLE_FINAL)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
  EXPECT_TRUE(eh.from_encoding(enc, FINAL));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__from_encoding_XCDR1_LITTLE_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
  EXPECT_TRUE(eh.from_encoding(enc, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Parameter List", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__from_encoding_XCDR2_LITTLE_FINAL)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  EXPECT_TRUE(eh.from_encoding(enc, FINAL));
  EXPECT_STREQ("XCDR2 Little Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__from_encoding_XCDR2_LITTLE_APPENDABLE)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  EXPECT_TRUE(eh.from_encoding(enc, APPENDABLE));
  EXPECT_STREQ("XCDR2 Little Endian Delimited", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__from_encoding_XCDR2_LITTLE_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  EXPECT_TRUE(eh.from_encoding(enc, MUTABLE));
  EXPECT_STREQ("XCDR2 Little Endian Parameter List", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__from_encoding_UNALIGNED_CDR_LITTLE_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EXPECT_FALSE(eh.from_encoding(enc, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc;
  EXPECT_FALSE(eh.to_encoding(enc, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_NOT_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, FINAL));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_CDR_LE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_CDR_LE);
  Encoding enc;
  EXPECT_FALSE(eh.to_encoding(enc, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_CDR_LE_NOT_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_CDR_LE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, FINAL));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_PL_CDR_BE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_PL_CDR_BE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Parameter List", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_PL_CDR_LE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_PL_CDR_LE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc,MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Parameter List", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_CDR2_BE_FINAL)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_CDR2_BE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc,FINAL));
  EXPECT_STREQ("XCDR2 Big Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_CDR2_LE_FINAL)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_CDR2_LE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, FINAL));
  EXPECT_STREQ("XCDR2 Little Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_D_CDR2_BE_APPENDABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_D_CDR2_BE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, APPENDABLE));
  EXPECT_STREQ("XCDR2 Big Endian Delimited", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_D_CDR2_LE_APPENDABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_D_CDR2_LE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, APPENDABLE));
  EXPECT_STREQ("XCDR2 Little Endian Delimited", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_PL_CDR2_BE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_PL_CDR2_BE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, MUTABLE));
  EXPECT_STREQ("XCDR2 Big Endian Parameter List", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_PL_CDR2_LE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_PL_CDR2_LE);
  Encoding enc;
  EXPECT_TRUE(eh.to_encoding(enc, MUTABLE));
  EXPECT_STREQ("XCDR2 Little Endian Parameter List", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_XML)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_XML);
  Encoding enc;
  EXPECT_FALSE(eh.to_encoding(enc, MUTABLE));
  EXPECT_STREQ("XML", eh.to_string().c_str());
}

TEST(dds_DCPS_Serializer, EncapsulationHeader__to_encoding_INVALID)
{
  Encoding initenc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EncapsulationHeader eh(initenc, FINAL);
  Encoding enc;
  EXPECT_FALSE(eh.to_encoding(enc, FINAL));
  EXPECT_STREQ("Invalid", eh.to_string().c_str());
}

TEST(serializer_test, Serializer_Serializer_ACE_Message_Block_Encoding)
{
  ACE_Message_Block amb;
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  Serializer ser(&amb, enc);

  EXPECT_TRUE(ser.good_bit());
  EXPECT_EQ(0, ser.encoding().to_string().compare(0, 28, "Unaligned CDR, little-endian"));
}

TEST(dds_DCPS_Serializer, Serializer_Serializer_ACE_Message_Block_Kind)
{
  ACE_Message_Block amb;
  Serializer ser(&amb, Encoding::KIND_XCDR1);

  EXPECT_TRUE(ser.good_bit());
  EXPECT_EQ(0, ser.encoding().to_string().compare(0, 24, "CDR/XCDR1, little-endian"));
}

TEST(dds_DCPS_Serializer, Serializer_Serializer_ACE_Message_Block_Kind_bool)
{
  ACE_Message_Block amb;
  Serializer ser(&amb, Encoding::KIND_XCDR2);

  EXPECT_TRUE(ser.good_bit());
  EXPECT_EQ(0, ser.encoding().to_string().compare(0, 20, "XCDR2, little-endian"));
}

TEST(dds_DCPS_Serializer, Serializer_set_endianness)
{
  ACE_Message_Block amb;
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  Serializer ser(&amb, enc);

  EXPECT_TRUE(ser.good_bit());
  EXPECT_EQ(0, ser.encoding().to_string().compare(0, 28, "Unaligned CDR, little-endian"));

  ser.endianness(ENDIAN_BIG);
  EXPECT_EQ(0, ser.encoding().to_string().compare(0, 25, "Unaligned CDR, big-endian"));
}

TEST(dds_DCPS_Serializer, Serializer_swap_bytes_endianness)
{
  ACE_Message_Block amb;
  Encoding enc;
  Serializer ser(&amb, enc);

  ser.swap_bytes(true);
  EXPECT_EQ(ser.endianness(), ENDIAN_NONNATIVE);
}

TEST(serializer_test, Serializer_align_context_basic_reference)
{
  ACE_Message_Block amb(64);
  Encoding enc;
  Serializer ser(&amb, enc);

  std::memset(amb.wr_ptr(), 0, 64);

  const ACE_CDR::Octet c = 3;
  const ACE_CDR::Double d = 0.12345;

  ASSERT_TRUE(ser << c);
  ASSERT_TRUE(ser << d);
  ASSERT_TRUE(ser << c);
  ASSERT_TRUE(ser << d);
  ASSERT_TRUE(ser << d);


  std::set<size_t> expected_zeros;
  size_t zeros[] = {2, 3, 4, 5, 6, 7, 8, 18, 19, 20, 21, 22, 23, 24};
  expected_zeros.insert(zeros, zeros + (sizeof zeros / sizeof (size_t*)));
  Serializer rser(&amb, enc);
  char i = 0;
  while (rser.rpos() != ser.wpos()) {
    ASSERT_TRUE(rser >> i);
    //std::cout << static_cast<unsigned short>(static_cast<unsigned char>(i)) << " " << std::flush;
    ASSERT_TRUE(expected_zeros.count(rser.rpos()) ? !i : i);
  }
  //std::cout << std::endl;
}

TEST(serializer_test, Serializer_align_context_basic)
{
  ACE_Message_Block amb(64);

  Encoding enc;
  Serializer ser(&amb, enc);

  std::memset(amb.wr_ptr(), 0, 64);

  const ACE_CDR::Octet c = 3;
  const ACE_CDR::Double d = 0.12345;

  ASSERT_TRUE(ser << c);
  {
    Serializer::ScopedAlignmentContext sac(ser);
    ASSERT_TRUE(ser << d);
    ASSERT_TRUE(ser << c);
    ASSERT_TRUE(ser << d);
  }
  ASSERT_TRUE(ser << d);

  Serializer rser(&amb, enc);
  char i = 0;
  std::set<size_t> expected_zeros;
  size_t zeros[] = {2, 3, 4, 14, 15, 16, 17, 18, 19, 20, 29, 30, 31, 32};
  expected_zeros.insert(zeros, zeros + (sizeof zeros / sizeof (size_t*)));
  while (rser.rpos() != ser.wpos()) {
    ASSERT_TRUE(rser >> i);
    //std::cout << static_cast<unsigned short>(static_cast<unsigned char>(i)) << " " << std::flush;
    ASSERT_TRUE(expected_zeros.count(rser.rpos()) ? !i : i);
  }
  //std::cout << std::endl;
}

TEST(serializer_test, Serializer_align_context_2_buff)
{
  OpenDDS::DCPS::Message_Block_Ptr amb(new ACE_Message_Block(24));
  amb->cont(new ACE_Message_Block(32));

  Encoding enc;
  Serializer ser(amb.get(), enc);

  std::memset(amb->wr_ptr(), 0, 24);
  std::memset(amb->cont()->wr_ptr(), 0, 32);

  const ACE_CDR::Octet c = 3;
  const ACE_CDR::Double d = 0.12345;

  ASSERT_TRUE(ser << c);
  {
    Serializer::ScopedAlignmentContext sac(ser);
    ASSERT_TRUE(ser << d);
    ASSERT_TRUE(ser << c);
    ASSERT_TRUE(ser << d);
  }
  ASSERT_TRUE(ser << d);

  Serializer rser(amb.get(), enc);
  char i = 0;
  std::set<size_t> expected_zeros;
  size_t zeros[] = {2, 3, 4, 14, 15, 16, 17, 18, 19, 20, 29, 30, 31, 32};
  expected_zeros.insert(zeros, zeros + (sizeof zeros / sizeof (size_t*)));
  while (rser.rpos() != ser.wpos()) {
    ASSERT_TRUE(rser >> i);
    //std::cout << static_cast<unsigned short>(static_cast<unsigned char>(i)) << " " << std::flush;
    ASSERT_TRUE(expected_zeros.count(rser.rpos()) ? !i : i);
  }
  //std::cout << std::endl;
}

TEST(serializer_test, Serializer_align_context_2_buff_diff_walign)
{
  OpenDDS::DCPS::Message_Block_Ptr amb(new ACE_Message_Block(21));
  amb->cont(new ACE_Message_Block(32));

  Encoding enc;
  Serializer ser(amb.get(), enc);

  std::memset(amb->wr_ptr(), 0, 21);
  std::memset(amb->cont()->wr_ptr(), 0, 32);

  amb->cont()->rd_ptr(3);
  amb->cont()->wr_ptr(3);

  const ACE_CDR::Octet c = 3;
  const ACE_CDR::Double d = 0.12345;

  ASSERT_TRUE(ser << c);
  {
    Serializer::ScopedAlignmentContext sac(ser);
    ASSERT_TRUE(ser << d);
    ASSERT_TRUE(ser << c);
    ASSERT_TRUE(ser << d);
  }
  ASSERT_TRUE(ser << d);

  Serializer rser(amb.get(), enc);
  char i = 0;
  std::set<size_t> expected_zeros;
  size_t zeros[] = {2, 3, 4, 14, 15, 16, 17, 18, 19, 20, 29, 30, 31, 32};
  expected_zeros.insert(zeros, zeros + (sizeof zeros / sizeof (size_t*)));
  while (rser.rpos() != ser.wpos()) {
    ASSERT_TRUE(rser >> i);
    //std::cout << static_cast<unsigned short>(static_cast<unsigned char>(i)) << " " << std::flush;
    ASSERT_TRUE(expected_zeros.count(rser.rpos()) ? !i : i);
  }
  //std::cout << std::endl;
}

TEST(serializer_test, Serializer_align_context_2_buff_diff_walign_read)
{
  OpenDDS::DCPS::Message_Block_Ptr amb(new ACE_Message_Block(21));
  amb->cont(new ACE_Message_Block(32));

  Encoding enc;
  Serializer ser(amb.get(), enc);

  std::memset(amb->wr_ptr(), 0, 21);
  std::memset(amb->cont()->wr_ptr(), 0, 32);

  amb->cont()->rd_ptr(3);
  amb->cont()->wr_ptr(3);

  const ACE_CDR::UShort c = 3;
  const ACE_CDR::ULongLong d = 54321;

  ASSERT_TRUE(ser << c);
  {
    Serializer::ScopedAlignmentContext sac(ser);
    ASSERT_TRUE(ser << d);
    ASSERT_TRUE(ser << c);
    ASSERT_TRUE(ser << d);
  }
  ASSERT_TRUE(ser << d);

  ACE_CDR::UShort c_out = 0;
  ACE_CDR::ULongLong d_out = 0.0;

  Serializer rser(amb.get(), enc);

  ASSERT_TRUE(rser >> c_out);
  ASSERT_EQ(c, c_out);
  {
    Serializer::ScopedAlignmentContext sac(rser);
    ASSERT_TRUE(rser >> d_out);
    ASSERT_EQ(d, d_out);
    ASSERT_TRUE(rser >> c_out);
    ASSERT_EQ(c, c_out);
    ASSERT_TRUE(rser >> d_out);
    ASSERT_EQ(d, d_out);
  }
  ASSERT_TRUE(rser >> d_out);
  ASSERT_EQ(d, d_out);
}

TEST(serializer_test, Serializer_align_context_2_buff_diff_walign_read_with_min)
{
  OpenDDS::DCPS::Message_Block_Ptr amb(new ACE_Message_Block(21));
  amb->cont(new ACE_Message_Block(32));

  Encoding enc;
  Serializer ser(amb.get(), enc);

  std::memset(amb->wr_ptr(), 0, 21);
  std::memset(amb->cont()->wr_ptr(), 0, 32);

  amb->cont()->rd_ptr(3);
  amb->cont()->wr_ptr(3);

  const ACE_CDR::UShort c = 3;
  const ACE_CDR::ULongLong d = 54321;

  ASSERT_TRUE(ser << c); // Writes 03 00
  {
    Serializer::ScopedAlignmentContext sac(ser);
    ASSERT_TRUE(ser << d); // Writes 8 bytes
    ASSERT_TRUE(ser << c); // Writes 2 bytes
    ASSERT_TRUE(ser << d); // Skips 6 bytes for alignment, writes 8 bytes (so 24 total)
  }
  ASSERT_TRUE(ser << d);

  ACE_CDR::UShort c_out = 0;
  ACE_CDR::ULongLong d_out = 0.0;

  Serializer rser(amb.get(), enc);

  ASSERT_TRUE(rser >> c_out);
  ASSERT_EQ(c, c_out);
  {
    Serializer::ScopedAlignmentContext sac(rser, 24); // See above for why 24
    ASSERT_TRUE(rser >> d_out);
    ASSERT_EQ(d, d_out);
    ASSERT_TRUE(rser >> c_out);
    ASSERT_EQ(c, c_out);
    ASSERT_TRUE(rser >> c_out); // this should pass, even though it's not what was written
    // But we will rely on sac to skip until we've got 20 bytes so that the following d_out is aligned correctly
  }
  ASSERT_TRUE(rser >> d_out);
  ASSERT_EQ(d, d_out);
}

TEST(serializer_test, Serializer_test_peek_align)
{
  OpenDDS::DCPS::Message_Block_Ptr amb(new ACE_Message_Block(5));
  amb->cont(new ACE_Message_Block(8));

  Encoding enc;
  Serializer ser(amb.get(), enc);

  std::memset(amb->wr_ptr(), 0, 5);
  std::memset(amb->cont()->wr_ptr(), 0, 8);

  amb->cont()->rd_ptr(1);
  amb->cont()->wr_ptr(1);

  const ACE_CDR::ULong a = 7;
  const ACE_CDR::ULong b = 13;
  const ACE_CDR::ULong c = 42;

  ASSERT_TRUE(ser << a);
  ASSERT_TRUE(ser << b);
  ASSERT_TRUE(ser << c);

  Serializer rser(amb.get(), enc);
  ACE_CDR::ULong res = 0;

  ASSERT_TRUE(rser.peek(res));
  ASSERT_EQ(res, a);
  ASSERT_TRUE(rser >> res);
  ASSERT_EQ(res, a);
  ASSERT_TRUE(rser.peek(res));
  ASSERT_EQ(res, b);
  ASSERT_TRUE(rser >> res);
  ASSERT_EQ(res, b);
  ASSERT_TRUE(rser.peek(res));
  ASSERT_TRUE(res == c);
  ASSERT_TRUE(rser >> res);
  ASSERT_TRUE(res == c);
}

TEST(serializer_test, Serializer_test_peek_depth)
{
  OpenDDS::DCPS::Message_Block_Ptr amb(new ACE_Message_Block(1));
  ACE_Message_Block* cont = amb.get();
  for (size_t i = 0; i < 10000; ++i) {
    cont->cont(new ACE_Message_Block(1));
    cont = cont->cont();
  }

  Encoding enc;
  Serializer ser(amb.get(), enc);

  const ACE_CDR::ULong a = 7;
  const ACE_CDR::ULong b = 13;
  const ACE_CDR::ULong c = 42;

  ASSERT_TRUE(ser << a);
  ASSERT_TRUE(ser << b);
  ASSERT_TRUE(ser << c);

  Serializer rser(amb.get(), enc);
  ACE_CDR::ULong res = 0;

  ASSERT_TRUE(rser.peek(res));
  ASSERT_EQ(res, a);
  ASSERT_TRUE(rser >> res);
  ASSERT_EQ(res, a);
  ASSERT_TRUE(rser.peek(res));
  ASSERT_EQ(res, b);
  ASSERT_TRUE(rser >> res);
  ASSERT_EQ(res, b);
  ASSERT_TRUE(rser.peek(res));
  ASSERT_TRUE(res == c);
  ASSERT_TRUE(rser >> res);
  ASSERT_TRUE(res == c);
}

