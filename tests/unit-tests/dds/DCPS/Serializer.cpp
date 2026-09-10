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

TEST(dds_DCPS_Serializer, Serializer_read_boolean_normalizes)
{
  Message_Block_Ptr mb(new ACE_Message_Block(4));
  const Encoding enc;
  Serializer writer(mb.get(), enc);
  const ACE_CDR::Octet input[] = {0, 1, 2, 255};
  ASSERT_TRUE(writer.write_octet_array(input, 4));

  Serializer reader(mb.get(), enc);
  const ACE_CDR::Octet expected[] = {0, 1, 1, 1};
  for (size_t i = 0; i < 4; ++i) {
    ACE_CDR::Boolean value = false;
    ASSERT_TRUE(reader >> ACE_InputCDR::to_boolean(value));
    ACE_CDR::Octet representation = 0;
    std::memcpy(&representation, &value, boolean_cdr_size);
    EXPECT_EQ(expected[i], representation);
  }
}

TEST(dds_DCPS_Serializer, Serializer_read_boolean_array_normalizes)
{
  Message_Block_Ptr mb(new ACE_Message_Block(4));
  const Encoding enc;
  Serializer writer(mb.get(), enc);
  const ACE_CDR::Octet input[] = {0, 1, 2, 255};
  ASSERT_TRUE(writer.write_octet_array(input, 4));

  Serializer reader(mb.get(), enc);
  ACE_CDR::Boolean values[4] = {};
  ASSERT_TRUE(reader.read_boolean_array(values, 4));

  const ACE_CDR::Octet expected[] = {0, 1, 1, 1};
  ACE_CDR::Octet representations[4] = {};
  std::memcpy(representations, values, sizeof values);
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(expected[i], representations[i]);
  }
}

TEST(dds_DCPS_Serializer, Serializer_scoped_read_limit_nested)
{
  Message_Block_Ptr mb(new ACE_Message_Block(8));
  const Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
  Serializer writer(mb.get(), enc);
  const ACE_CDR::Octet input[] = {0, 1, 2, 3, 4, 5, 6, 7};
  ASSERT_TRUE(writer.write_octet_array(input, 8));

  Serializer reader(mb.get(), enc);
  {
    Serializer::ScopedReadLimit outer(reader, 6);
    ASSERT_TRUE(outer.valid());
    EXPECT_EQ(6u, reader.length());
    ACE_CDR::Octet value[2];
    ASSERT_TRUE(reader.read_octet_array(value, 2));
    {
      Serializer::ScopedReadLimit inner(reader, 2);
      ASSERT_TRUE(inner.valid());
      ASSERT_TRUE(reader.read_octet_array(value, 2));
      EXPECT_EQ(0u, inner.remaining());
    }
    EXPECT_EQ(2u, outer.remaining());
    ASSERT_TRUE(outer.skip_to_end());
  }
  EXPECT_EQ(2u, reader.length());
  EXPECT_EQ(6u, reader.rpos());
}

TEST(dds_DCPS_Serializer, Serializer_scoped_read_limit_rejects_crossing)
{
  Message_Block_Ptr mb(new ACE_Message_Block(8));
  const Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
  Serializer writer(mb.get(), enc);
  const ACE_CDR::Octet input[] = {0, 1, 2, 3, 4, 5, 6, 7};
  ASSERT_TRUE(writer.write_octet_array(input, 8));

  Serializer reader(mb.get(), enc);
  Serializer::ScopedReadLimit limit(reader, 3);
  ASSERT_TRUE(limit.valid());
  ACE_CDR::ULong value = 0;
  EXPECT_FALSE(reader >> value);
  EXPECT_FALSE(reader.good_bit());
  EXPECT_EQ(0u, reader.rpos());
}

TEST(dds_DCPS_Serializer, Serializer_scoped_read_limit_skips_remainder)
{
  Message_Block_Ptr mb(new ACE_Message_Block(4));
  const Encoding enc;
  Serializer writer(mb.get(), enc);
  const ACE_CDR::Octet input[] = {0, 1, 2, 3};
  ASSERT_TRUE(writer.write_octet_array(input, 4));

  Serializer reader(mb.get(), enc);
  {
    Serializer::ScopedReadLimit limit(reader, 3, true, true);
    ASSERT_TRUE(limit.valid());
    ACE_CDR::Octet value = 0;
    ASSERT_TRUE(reader >> ACE_InputCDR::to_octet(value));
    EXPECT_EQ(0, value);
  }
  EXPECT_EQ(3u, reader.rpos());
  EXPECT_EQ(1u, reader.length());
}

TEST(dds_DCPS_Serializer, Serializer_check_size)
{
  Message_Block_Ptr mb(new ACE_Message_Block(12));
  const Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
  Serializer writer(mb.get(), enc);
  const ACE_CDR::Octet input[12] = {};
  ASSERT_TRUE(writer.write_octet_array(input, 12));

  Serializer reader(mb.get(), enc);
  ACE_CDR::Octet value = 0;
  ASSERT_TRUE(reader >> ACE_InputCDR::to_octet(value));
  EXPECT_TRUE(reader.check_size(2, uint32_cdr_size, uint32_cdr_size));
  EXPECT_FALSE(reader.check_size(3, uint32_cdr_size, uint32_cdr_size));
  EXPECT_FALSE(reader.good_bit());
}

TEST(dds_DCPS_Serializer, Serializer_check_size_overflow)
{
  ACE_Message_Block mb(1);
  const ACE_CDR::Octet value = 0;
  ASSERT_EQ(0, mb.copy(reinterpret_cast<const char*>(&value), 1));
  Serializer reader(&mb, Encoding());
  EXPECT_FALSE(reader.check_size((std::numeric_limits<size_t>::max)(), 2));
  EXPECT_FALSE(reader.good_bit());
}

TEST(dds_DCPS_Serializer, Serializer_read_limit_in_rdstate)
{
  Message_Block_Ptr mb(new ACE_Message_Block(8));
  Serializer writer(mb.get(), Encoding());
  const ACE_CDR::Octet input[8] = {};
  ASSERT_TRUE(writer.write_octet_array(input, 8));

  Serializer reader(mb.get(), Encoding());
  const Serializer::RdState unrestricted = reader.rdstate();
  ASSERT_TRUE(reader.set_read_limit(3));
  const Serializer::RdState restricted = reader.rdstate();
  EXPECT_EQ(3u, reader.length());

  reader.rdstate(unrestricted);
  EXPECT_EQ(8u, reader.length());
  reader.rdstate(restricted);
  EXPECT_EQ(3u, reader.length());
}

TEST(dds_DCPS_Serializer, Serializer_scoped_read_limit_rejects_skip)
{
  Message_Block_Ptr mb(new ACE_Message_Block(8));
  const Encoding enc;
  Serializer writer(mb.get(), enc);
  const ACE_CDR::Octet input[] = {0, 1, 2, 3, 4, 5, 6, 7};
  ASSERT_TRUE(writer.write_octet_array(input, 8));

  Serializer reader(mb.get(), enc);
  Serializer::ScopedReadLimit limit(reader, 3);
  ASSERT_TRUE(limit.valid());
  EXPECT_FALSE(reader.skip(4));
  EXPECT_FALSE(reader.good_bit());
  EXPECT_EQ(0u, reader.rpos());
}

TEST(dds_DCPS_Serializer, Serializer_scoped_read_limit_rejects_alignment)
{
  Message_Block_Ptr mb(new ACE_Message_Block(8));
  const Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
  Serializer writer(mb.get(), enc);
  const ACE_CDR::Octet input[] = {0, 1, 2, 3, 4, 5, 6, 7};
  ASSERT_TRUE(writer.write_octet_array(input, 8));

  Serializer reader(mb.get(), enc);
  Serializer::ScopedReadLimit limit(reader, 3);
  ASSERT_TRUE(limit.valid());
  ACE_CDR::Octet octet = 0;
  ASSERT_TRUE(reader >> ACE_InputCDR::to_octet(octet));
  ACE_CDR::ULong value = 0;
  EXPECT_FALSE(reader >> value);
  EXPECT_FALSE(reader.good_bit());
  EXPECT_EQ(1u, reader.rpos());
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

TEST(dds_DCPS_Serializer, Serializer_test_odd_wstring_bytecount)
{
  Message_Block_Ptr amb(new ACE_Message_Block(7));
  const Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
  Serializer ser_w(amb.get(), enc);
  const ACE_CDR::Octet value[] = {0x41, 0, 0x42};
  ASSERT_TRUE(ser_w << ACE_CDR::ULong(sizeof value));
  ASSERT_TRUE(ser_w.write_octet_array(value, sizeof value));

  Serializer ser(amb.get(), enc);
  ACE_CDR::WChar* str = 0;
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
  mb.copy(reinterpret_cast<const char*>(xcdr), sizeof(xcdr));
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

TEST(dds_DCPS_Serializer, parameter_id_xcdr1_flags)
{
  const Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  ACE_Message_Block mb(48);
  Serializer writer(&mb, enc);

  ASSERT_TRUE(writer.write_parameter_id(1, 4, false));
  ASSERT_TRUE(writer.write_parameter_id(2, 4, true));
  ASSERT_TRUE(writer.write_parameter_id(0x4000, 4, false));
  ASSERT_TRUE(writer.write_parameter_id(0x4001, 4, true));
  ASSERT_TRUE(writer.write_parameter_id(3, 0x10000, false));

  const unsigned char expected[] = {
    0x00, 0x01, 0x00, 0x04,
    0x40, 0x02, 0x00, 0x04,
    0x7f, 0x01, 0x00, 0x08, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x7f, 0x01, 0x00, 0x08, 0x40, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00, 0x04,
    0x7f, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00
  };
  ASSERT_EQ(sizeof(expected), mb.length());
  EXPECT_EQ(0, std::memcmp(mb.rd_ptr(), expected, sizeof(expected)));

  Serializer reader(&mb, enc);
  unsigned id;
  size_t size;
  bool must_understand;
  ASSERT_TRUE(reader.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(1u, id);
  EXPECT_EQ(4u, size);
  EXPECT_FALSE(must_understand);
  ASSERT_TRUE(reader.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(2u, id);
  EXPECT_EQ(4u, size);
  EXPECT_TRUE(must_understand);
  ASSERT_TRUE(reader.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(0x4000u, id);
  EXPECT_EQ(4u, size);
  EXPECT_FALSE(must_understand);
  ASSERT_TRUE(reader.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(0x4001u, id);
  EXPECT_EQ(4u, size);
  EXPECT_TRUE(must_understand);
  ASSERT_TRUE(reader.read_parameter_id(id, size, must_understand));
  EXPECT_EQ(3u, id);
  EXPECT_EQ(0x10000u, size);
  EXPECT_FALSE(must_understand);
}

TEST(dds_DCPS_Serializer, parameter_id_xcdr1_serialized_size)
{
  const Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  size_t size = 0;
  size_t running_size = 0;
  bool previous_header_extended = false;

  serialized_size_parameter_id(enc, size, running_size, 1,
                               previous_header_extended);
  primitive_serialized_size_ulong(enc, size);

  serialized_size_parameter_id(enc, size, running_size, 0x4000,
                               previous_header_extended);
  primitive_serialized_size_ulong(enc, size);

  serialized_size_parameter_id(enc, size, running_size, 3,
                               previous_header_extended);
  size += 0x10000;

  // This member needs an extended header for both its ID and its size.  The
  // header must be counted once, not once for each reason.
  serialized_size_parameter_id(enc, size, running_size, 0x4001,
                               previous_header_extended);
  size += 0x10000;

  serialized_size_list_end_parameter_id(enc, size, running_size,
                                        previous_header_extended);
  EXPECT_EQ(131124u, size);
}

TEST(dds_DCPS_Serializer, read_parameter_id_xcdr1_short_extended_header)
{
  const unsigned char xcdr[] = {
    0x3f, 0x01, // PID_EXTENDED
    0x00, 0x07  // Too short to contain the long ID and size
  };
  ACE_Message_Block mb(sizeof xcdr);
  ASSERT_EQ(0, mb.copy(reinterpret_cast<const char*>(xcdr), sizeof xcdr));
  const Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  Serializer ser(&mb, enc);
  unsigned id = 0;
  size_t size = 0;
  bool must_understand = false;
  EXPECT_FALSE(ser.read_parameter_id(id, size, must_understand));
  EXPECT_FALSE(ser.good_bit());
}

TEST(dds_DCPS_Serializer, read_delimiter_larger_than_input)
{
  const unsigned char xcdr[] = {
    0x00, 0x00, 0x00, 0x05,
    0x00, 0x00, 0x00, 0x00
  };
  ACE_Message_Block mb(sizeof xcdr);
  ASSERT_EQ(0, mb.copy(reinterpret_cast<const char*>(xcdr), sizeof xcdr));
  const Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
  Serializer ser(&mb, enc);
  size_t size = 0;
  EXPECT_FALSE(ser.read_delimiter(size));
  EXPECT_FALSE(ser.good_bit());
}

namespace {
  bool read_parameter_id_xcdr2(const unsigned char* xcdr, size_t size)
  {
    const Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
    ACE_Message_Block mb(size);
    if (xcdr && mb.copy(reinterpret_cast<const char*>(xcdr), size) != 0) {
      ACE_ERROR((LM_ERROR, "read_parameter_id_xcdr2: failed to copy data to message block!\n"));
      return false;
    }
    Serializer ser(&mb, enc);
    unsigned id;
    size_t member_size;
    bool must_understand;
    return ser.read_parameter_id(id, member_size, must_understand);
  }

  void test_read_parameter_id_xcdr2_ok(const unsigned char* xcdr, size_t size)
  {
    ASSERT_TRUE(read_parameter_id_xcdr2(xcdr, size));
  }

  void test_read_parameter_id_xcdr2_malformed(const unsigned char* xcdr, size_t size)
  {
    ASSERT_FALSE(read_parameter_id_xcdr2(xcdr, size));
  }
}

TEST(dds_DCPS_Serializer, read_parameter_id_xcdr2_ok)
{
  // well-formed emheader and nextint for LC=5,6,7 cases
  {
    unsigned char xcdr[] = {
      0x50, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x04
    };
    test_read_parameter_id_xcdr2_ok(xcdr, sizeof(xcdr));

    // Splitted message into multiple blocks should also work
    const Encoding enc(Encoding::KIND_XCDR2, ENDIAN_BIG);
    const size_t total_size = sizeof(xcdr);
    ACE_Message_Block mb(total_size - 3);
    mb.copy(reinterpret_cast<const char*>(xcdr), total_size - 3);
    ACE_Message_Block mb2(3);
    mb2.copy(reinterpret_cast<const char*>(xcdr) + total_size - 3, 3);
    mb.cont(&mb2);
    Serializer ser(&mb, enc);
    unsigned id;
    size_t member_size;
    bool must_understand;
    ASSERT_TRUE(ser.read_parameter_id(id, member_size, must_understand));
  }
  {
    unsigned char xcdr[] = {
      0x60, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x04
    };
    test_read_parameter_id_xcdr2_ok(xcdr, sizeof(xcdr));
  }
  {
    unsigned char xcdr[] = {
      0x70, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x04
    };
    test_read_parameter_id_xcdr2_ok(xcdr, sizeof(xcdr));
  }
}

TEST(dds_DCPS_Serializer, read_parameter_id_xcdr2_malformed_emheader)
{
  {
    // Emheader is absent
    test_read_parameter_id_xcdr2_malformed(0, 0);
  }
  {
    // Emheader is truncated
    unsigned char xcdr[] = {
      0x40, 0x00, 0x00
    };
    test_read_parameter_id_xcdr2_malformed(xcdr, sizeof(xcdr));
  }
}

TEST(dds_DCPS_Serializer, read_parameter_id_xcdr2_missing_nextint)
{
  {
    // LC=4
    unsigned char xcdr[] = {
      0x40, 0x00, 0x00, 0x01,
    };
    test_read_parameter_id_xcdr2_malformed(xcdr, sizeof(xcdr));
  }
  {
    // LC=5
    unsigned char xcdr[] = {
      0x50, 0x00, 0x00, 0x01,
    };
    test_read_parameter_id_xcdr2_malformed(xcdr, sizeof(xcdr));
  }
  {
    // LC=6
    unsigned char xcdr[] = {
      0x60, 0x00, 0x00, 0x01,
    };
    test_read_parameter_id_xcdr2_malformed(xcdr, sizeof(xcdr));
  }
  {
    // LC=7
    unsigned char xcdr[] = {
      0x70, 0x00, 0x00, 0x01,
    };
    test_read_parameter_id_xcdr2_malformed(xcdr, sizeof(xcdr));
  }
}

TEST(dds_DCPS_Serializer, read_parameter_id_xcdr2_truncated_nextint)
{
  {
    // LC=4
    unsigned char xcdr[] = {
      0x40, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x03
    };
    test_read_parameter_id_xcdr2_malformed(xcdr, sizeof(xcdr));
  }
  {
    // LC=5
    unsigned char xcdr[] = {
      0x50, 0x00, 0x00, 0x01,
      0x01
    };
    test_read_parameter_id_xcdr2_malformed(xcdr, sizeof(xcdr));
  }
  {
    // LC=6
    unsigned char xcdr[] = {
      0x60, 0x00, 0x00, 0x01,
      0x00, 0x02
    };
    test_read_parameter_id_xcdr2_malformed(xcdr, sizeof(xcdr));
  }
  {
    // LC=7
    unsigned char xcdr[] = {
      0x70, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x03
    };
    test_read_parameter_id_xcdr2_malformed(xcdr, sizeof(xcdr));
  }
}

namespace {
  // Build a message block chain of `n` bytes from `xcdr`, split into `chunk`-byte
  // links (0 == single block), mimicking the fragmented buffers the RTPS receive
  // path hands to the Serializer.
  ACE_Message_Block* chain_from(const unsigned char* xcdr, size_t n, size_t chunk)
  {
    if (chunk == 0 || chunk >= n) {
      ACE_Message_Block* mb = new ACE_Message_Block(n ? n : 1);
      mb->copy(reinterpret_cast<const char*>(xcdr), n);
      return mb;
    }
    ACE_Message_Block* head = 0;
    ACE_Message_Block* tail = 0;
    for (size_t i = 0; i < n; i += chunk) {
      const size_t len = (n - i < chunk) ? (n - i) : chunk;
      ACE_Message_Block* mb = new ACE_Message_Block(len);
      mb->copy(reinterpret_cast<const char*>(xcdr) + i, len);
      if (!head) { head = tail = mb; } else { tail->cont(mb); tail = mb; }
    }
    return head;
  }

  // Walk an XCDR2 parameter list the way a mutable-struct/union reader does:
  // read_parameter_id() then skip(size), until the reader reports end of stream,
  // then call read_parameter_id() once more.  Returns the number of parameters
  // consumed; sets `extra_ok` from the trailing call.  Must never crash.
  int walk_parameter_ids_xcdr2(const unsigned char* xcdr, size_t n, Endianness endian,
                               size_t chunk, bool& extra_ok)
  {
    const Encoding enc(Encoding::KIND_XCDR2, endian);
    ACE_Message_Block* head = chain_from(xcdr, n, chunk);
    Serializer ser(head, enc);
    int count = 0;
    for (;;) {
      unsigned id = 0xdead;
      size_t size = 0xdead;
      bool must_understand = false;
      if (!ser.read_parameter_id(id, size, must_understand)) {
        break;
      }
      ++count;
      if (!ser.skip(size)) {
        break;
      }
    }
    unsigned id = 0xdead;
    size_t size = 0xdead;
    bool must_understand = false;
    extra_ok = ser.read_parameter_id(id, size, must_understand);
    ACE_Message_Block::release(head);
    return count;
  }
}

// Regression test for GHSA-w6x3-92q6-jr7g: reading past the end of an XCDR2
// parameter list must fail cleanly instead of dereferencing an exhausted
// message-block chain in Serializer::peek() / peek_helper().  Exercised over
// single and fragmented buffers and both endiannesses, plus a list whose final
// bytes are a bare LCgt4 EMHEADER (which forces the peek() at end of stream).
TEST(dds_DCPS_Serializer, read_parameter_id_xcdr2_walk_past_end)
{
  // Three well-formed members: LC=4 (4-byte body), LC=6 (nextint 1 => 4-byte
  // body), LC=7 (nextint 1 => 8-byte body).  Nothing follows the last member.
  const unsigned char be[] = {
    0x40, 0x00, 0x00, 0x01,  0x00, 0x00, 0x00, 0x04,  0xde, 0xad, 0xbe, 0xef,
    0x60, 0x00, 0x00, 0x02,  0x00, 0x00, 0x00, 0x01,  0x11, 0x22, 0x33, 0x44,
    0x70, 0x00, 0x00, 0x03,  0x00, 0x00, 0x00, 0x01,
    0x01, 0x02, 0x03, 0x04,  0x05, 0x06, 0x07, 0x08
  };
  const unsigned char le[] = {
    0x01, 0x00, 0x00, 0x40,  0x04, 0x00, 0x00, 0x00,  0xde, 0xad, 0xbe, 0xef,
    0x02, 0x00, 0x00, 0x60,  0x01, 0x00, 0x00, 0x00,  0x11, 0x22, 0x33, 0x44,
    0x03, 0x00, 0x00, 0x70,  0x01, 0x00, 0x00, 0x00,
    0x01, 0x02, 0x03, 0x04,  0x05, 0x06, 0x07, 0x08
  };
  const size_t chunks[] = {0, 1, 3, 4, 7};
  for (size_t c = 0; c < sizeof(chunks) / sizeof(chunks[0]); ++c) {
    bool extra_ok = true;
    EXPECT_EQ(3, walk_parameter_ids_xcdr2(be, sizeof(be), ENDIAN_BIG, chunks[c], extra_ok));
    EXPECT_FALSE(extra_ok);
    extra_ok = true;
    EXPECT_EQ(3, walk_parameter_ids_xcdr2(le, sizeof(le), ENDIAN_LITTLE, chunks[c], extra_ok));
    EXPECT_FALSE(extra_ok);
  }

  // List that ends on a bare LC=5 EMHEADER: the first read_parameter_id()
  // consumes it and then peek()s for the nextint with the stream exhausted.
  const unsigned char trailing_emheader[] = {
    0x40, 0x00, 0x00, 0x01,  0x00, 0x00, 0x00, 0x04,  0xde, 0xad, 0xbe, 0xef,
    0x50, 0x00, 0x00, 0x02
  };
  for (size_t c = 0; c < sizeof(chunks) / sizeof(chunks[0]); ++c) {
    bool extra_ok = true;
    EXPECT_EQ(1, walk_parameter_ids_xcdr2(trailing_emheader, sizeof(trailing_emheader),
                                          ENDIAN_BIG, chunks[c], extra_ok));
    EXPECT_FALSE(extra_ok);
  }
}
