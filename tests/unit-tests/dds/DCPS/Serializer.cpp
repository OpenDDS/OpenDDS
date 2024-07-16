#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/debug.h>

#include <gtest/gtest.h>

#include <cstring>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_Serializer, Encoding_Encoding)
{
  Encoding enc;
  EXPECT_EQ(0, enc.to_string().compare(0, 24, "CDR/XCDR1, little-endian"));
}

TEST(dds_DCPS_Serializer, Encoding_Encoding_XCDR1_ENDIAN_BIG)
{
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  EXPECT_EQ(0, enc.to_string().compare(0, 21, "CDR/XCDR1, big-endian"));
}

TEST(dds_DCPS_Serializer, Encoding_Encoding_UNALIGNED_CDR_ENDIAN_BIG)
{
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_BIG);
  EXPECT_EQ(0, enc.to_string().compare(0, 25, "Unaligned CDR, big-endian"));
}

TEST(dds_DCPS_Serializer, Encoding_Encoding_XCDR2_ENDIAN_BIG)
{
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
  EXPECT_EQ(0, enc.to_string().compare(0, 17, "XCDR2, big-endian"));
}

TEST(dds_DCPS_Serializer, Encoding_Encoding_XCDR1_ENDIAN_LITTLE)
{
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
}

TEST(dds_DCPS_Serializer, Encoding_Encoding_UNALIGNED_CDR_ENDIAN_LITTLE)
{
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EXPECT_EQ(0, enc.to_string().compare(0, 28, "Unaligned CDR, little-endian"));
}

TEST(dds_DCPS_Serializer, Encoding_Encoding_XCDR2_ENDIAN_LITTLE)
{
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  EXPECT_EQ(0, enc.to_string().compare(0, 20, "XCDR2, little-endian"));
}

TEST(dds_DCPS_Serializer, Encoding_Encoding_XCDR2_Swap)
{
  Encoding enc(Encoding::KIND_XCDR2, true);
  EXPECT_EQ(0, enc.to_string().compare(0, 17, "XCDR2, big-endian"));
}

TEST(dds_DCPS_Serializer, Encoding_Encoding_XCDR2_No_Swap)
{
  Encoding enc(Encoding::KIND_XCDR2, false);
  EXPECT_EQ(0, enc.to_string().compare(0, 20, "XCDR2, little-endian"));
}

TEST(dds_DCPS_Serializer, Encoding_Encoding_XCDR1_max_align)
{
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  EXPECT_EQ(size_t(8), enc.max_align());
}

TEST(dds_DCPS_Serializer, Encoding_Encoding_UNALIGNED_CDR_max_align)
{
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_BIG);
  EXPECT_EQ(size_t(0), enc.max_align());
}

TEST(dds_DCPS_Serializer, Encoding_Encoding_XCDR2_max_align)
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
  align(value, 8);
  EXPECT_EQ(size_t(8), value);
}

TEST(dds_DCPS_Serializer, Encoding_is_encapsulated_this_XCDR1)
{
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  EXPECT_TRUE(enc.is_encapsulated());
}

TEST(dds_DCPS_Serializer, Encoding_is_encapsulated_this_UNALIGNED_CDR)
{
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EXPECT_FALSE(enc.is_encapsulated());
}

TEST(dds_DCPS_Serializer, Encoding_Encoding_static_is_encacapsulatd_XCDR2)
{
  EXPECT_TRUE(Encoding::is_encapsulated(Encoding::KIND_XCDR2));
}

TEST(dds_DCPS_Serializer, Serializer_Serializer_ACE_Message_Block_Encoding)
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

TEST(dds_DCPS_Serializer, Serializer_align_context_basic_reference)
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

TEST(dds_DCPS_Serializer, Serializer_align_context_basic)
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

TEST(dds_DCPS_Serializer, Serializer_align_context_2_buff)
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

TEST(dds_DCPS_Serializer, Serializer_align_context_2_buff_diff_walign)
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

TEST(dds_DCPS_Serializer, Serializer_align_context_2_buff_diff_walign_read)
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
  ACE_CDR::ULongLong d_out = 0;

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

TEST(dds_DCPS_Serializer, Serializer_align_context_2_buff_diff_walign_read_with_min)
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
  ACE_CDR::ULongLong d_out = 0;

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

TEST(dds_DCPS_Serializer, Serializer_test_peek_align)
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

TEST(dds_DCPS_Serializer, Serializer_test_peek_depth)
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

TEST(dds_DCPS_Serializer, Serializer_test_trim)
{
  OpenDDS::DCPS::Message_Block_Ptr amb(new ACE_Message_Block(1));
  ACE_Message_Block* cont = amb.get();
  for (size_t i = 0; i < 9; ++i) {
    cont->cont(new ACE_Message_Block(1));
    cont = cont->cont();
  }

  const Encoding enc;
  Serializer ser_w(amb.get(), enc);
  ACE_CDR::Octet x[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  ASSERT_TRUE(ser_w.write_octet_array(x, sizeof x));

  Serializer ser(amb.get(), enc);
  ASSERT_FALSE(ser.trim(11));

  ASSERT_TRUE(ser.read_octet_array(x, 3));
  OpenDDS::DCPS::Message_Block_Ptr subset(ser.trim(3));
  ASSERT_EQ(subset->total_length(), 3u);
  ASSERT_EQ(subset->length(), 1u);
  ASSERT_EQ(subset->rd_ptr()[0], 3);
  ASSERT_TRUE(subset->cont());
  ASSERT_TRUE(subset->cont()->cont());
  ASSERT_FALSE(subset->cont()->cont()->cont());
}

TEST(dds_DCPS_Serializer, Serializer_test_bad_string)
{
  Message_Block_Ptr amb(new ACE_Message_Block(4));
  const Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
  Serializer ser_w(amb.get(), enc);
  ACE_CDR::Octet x[] = {1, 0, 0, 0};
  ASSERT_TRUE(ser_w.write_octet_array(x, sizeof x));

  Serializer ser(amb.get(), enc);
  ACE_CDR::Char* str = 0; // since read_string fails, no need to deallocate
  ASSERT_EQ(0u, ser.read_string(str));
  ASSERT_FALSE(ser.good_bit());
  ASSERT_EQ(0, str);
}

TEST(dds_DCPS_Serializer, Serializer_test_bad_wstring)
{
  Message_Block_Ptr amb(new ACE_Message_Block(4));
  const Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
  Serializer ser_w(amb.get(), enc);
  ACE_CDR::Octet x[] = {1, 0, 0, 0};
  ASSERT_TRUE(ser_w.write_octet_array(x, sizeof x));

  Serializer ser(amb.get(), enc);
  ACE_CDR::WChar* str = 0; // since read_string fails, no need to deallocate
  ASSERT_EQ(0u, ser.read_string(str));
  ASSERT_FALSE(ser.good_bit());
  ASSERT_EQ(0, str);
}

TEST(dds_DCPS_Serializer, Serializer_test_bad_string2)
{
  static const ACE_CDR::Octet x[] = {1, 0, 0, 0, 1};
  Message_Block_Ptr amb(new ACE_Message_Block(sizeof x));
  const Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
  Serializer ser_w(amb.get(), enc);
  ASSERT_TRUE(ser_w.write_octet_array(x, sizeof x));

  Serializer ser(amb.get(), enc);
  ACE_CDR::Char* str = 0; // since read_string fails, no need to deallocate
  ASSERT_EQ(0u, ser.read_string(str));
  ASSERT_FALSE(ser.good_bit());
  ASSERT_EQ(0, str);
}

TEST(dds_DCPS_Serializer, read_parameter_id_xcdr2)
{
  unsigned char xcdr[] = {
    // must understand, length code = 0, member id = 0x1111111
    0x81, 0x11, 0x11, 0x11,
    0xff,
    0x0, 0x0, 0x0, // Padding
    // length code = 1, member id = 0x2222222
    0x12, 0x22, 0x22, 0x22,
    0xff, 0xff,
    0x0, 0x0, // Padding
    // must understand, length code = 2, member id = 0x3333333
    0xa3, 0x33, 0x33, 0x33,
    0xff, 0xff, 0xff, 0xff,
    // length code = 3, member id = 0x4444444
    0x34, 0x44, 0x44, 0x44,
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
    // must understand, length code = 4, member id = 0x5555555
    0xc5, 0x55, 0x55, 0x55,
    0x0, 0x0, 0x0, 0x3, // NEXTINT that is NOT shared with member
    0xff, 0xff, 0xff,
    0x0, // Padding
    // length code = 5, member id = 0x6666666
    0x56, 0x66, 0x66, 0x66,
    0x0, 0x0, 0x0, 0x3, // NEXTINT that is shared with member
    0xff, 0xff, 0xff,
    0x0, // Padding
    // must understand, length code = 6, member id = 0x7777777
    0xe7, 0x77, 0x77, 0x77,
    0x0, 0x0, 0x0, 0x3, // NEXTINT that is shared with member
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
    // length code = 7, member id = 0x8888888
    0x78, 0x88, 0x88, 0x88,
    0x0, 0x0, 0x0, 0x3, // NEXTINT that is shared with member
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  };

  const Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
  ACE_Message_Block mb(sizeof(xcdr));
  mb.copy((const char*)xcdr, sizeof(xcdr));
  Serializer ser(&mb, enc);
  unsigned id;
  size_t size;
  bool must_understand;

  ASSERT_TRUE(ser.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(0x1111111u, id);
  EXPECT_EQ(1u, size);
  EXPECT_TRUE(must_understand);
  ASSERT_TRUE(ser.skip(size));

  ASSERT_TRUE(ser.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(0x2222222u, id);
  EXPECT_EQ(2u, size);
  EXPECT_FALSE(must_understand);
  ASSERT_TRUE(ser.skip(size));

  ASSERT_TRUE(ser.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(0x3333333u, id);
  EXPECT_EQ(4u, size);
  EXPECT_TRUE(must_understand);
  ASSERT_TRUE(ser.skip(size));

  ASSERT_TRUE(ser.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(0x4444444u, id);
  EXPECT_EQ(8u, size);
  EXPECT_FALSE(must_understand);
  ASSERT_TRUE(ser.skip(size));

  ASSERT_TRUE(ser.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(0x5555555u, id);
  EXPECT_EQ(3u, size);
  EXPECT_TRUE(must_understand);
  ASSERT_TRUE(ser.skip(size));

  ASSERT_TRUE(ser.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(0x6666666u, id);
  EXPECT_EQ(7u, size);
  EXPECT_FALSE(must_understand);
  ASSERT_TRUE(ser.skip(size));

  ASSERT_TRUE(ser.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(0x7777777u, id);
  EXPECT_EQ(16u, size);
  EXPECT_TRUE(must_understand);
  ASSERT_TRUE(ser.skip(size));

  ASSERT_TRUE(ser.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(0x8888888u, id);
  EXPECT_EQ(28u, size);
  EXPECT_FALSE(must_understand);
  ASSERT_TRUE(ser.skip(size));
}
