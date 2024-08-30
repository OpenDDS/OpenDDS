#include <dds/DCPS/EncapsulationHeader.h>

#include <dds/DCPS/debug.h>

#include <gtest/gtest.h>

#include <cstring>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_EncapsulationHeader, EncapsulationHeader_EncapsulationHeader)
{
  EncapsulationHeader eh;
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, EncapsulationHeader_EncapsulationHeader_Encoding_Valid)
{
  Encoding initenc;
  EncapsulationHeader eh(initenc, FINAL);
  EXPECT_TRUE(eh.is_good());
  EXPECT_STREQ("CDR/XCDR1 Little Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, EncapsulationHeader_EncapsulationHeader_Encoding_Invalid)
{
  OpenDDS::DCPS::LogRestore restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::None);
  Encoding initenc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EncapsulationHeader eh(initenc, FINAL);
  EXPECT_FALSE(eh.is_good());
  EXPECT_STREQ("Invalid", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, from_encoding_XCDR1_BIG_FINAL)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_BIG);
  EXPECT_TRUE(from_encoding(eh, enc, FINAL));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, from_encoding_XCDR1_LITTLE_FINAL)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
  EXPECT_TRUE(from_encoding(eh, enc, FINAL));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, from_encoding_XCDR1_LITTLE_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
  EXPECT_TRUE(from_encoding(eh, enc, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Parameter List", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, from_encoding_XCDR2_LITTLE_FINAL)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  EXPECT_TRUE(from_encoding(eh, enc, FINAL));
  EXPECT_STREQ("XCDR2 Little Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, from_encoding_XCDR2_LITTLE_APPENDABLE)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  EXPECT_TRUE(from_encoding(eh, enc, APPENDABLE));
  EXPECT_STREQ("XCDR2 Little Endian Delimited", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, from_encoding_XCDR2_LITTLE_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  EXPECT_TRUE(from_encoding(eh, enc, MUTABLE));
  EXPECT_STREQ("XCDR2 Little Endian Parameter List", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, from_encoding_UNALIGNED_CDR_LITTLE_MUTABLE)
{
  OpenDDS::DCPS::LogRestore restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::None);
  EncapsulationHeader eh;
  Encoding enc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EXPECT_FALSE(from_encoding(eh, enc, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_MUTABLE)
{
  OpenDDS::DCPS::LogRestore restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::None);
  EncapsulationHeader eh;
  Encoding enc;
  EXPECT_FALSE(to_encoding(enc, eh, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_NOT_MUTABLE)
{
  EncapsulationHeader eh;
  Encoding enc;
  EXPECT_TRUE(to_encoding(enc, eh, FINAL));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_CDR_LE_MUTABLE)
{
  OpenDDS::DCPS::LogRestore restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::None);
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_CDR_LE);
  Encoding enc;
  EXPECT_FALSE(to_encoding(enc, eh, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_CDR_LE_NOT_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_CDR_LE);
  Encoding enc;
  EXPECT_TRUE(to_encoding(enc, eh, FINAL));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_PL_CDR_BE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_PL_CDR_BE);
  Encoding enc;
  EXPECT_TRUE(to_encoding(enc, eh, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Big Endian Parameter List", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_PL_CDR_LE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_PL_CDR_LE);
  Encoding enc;
  EXPECT_TRUE(to_encoding(enc, eh, MUTABLE));
  EXPECT_STREQ("CDR/XCDR1 Little Endian Parameter List", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_CDR2_BE_FINAL)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_CDR2_BE);
  Encoding enc;
  EXPECT_TRUE(to_encoding(enc, eh, FINAL));
  EXPECT_STREQ("XCDR2 Big Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_CDR2_LE_FINAL)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_CDR2_LE);
  Encoding enc;
  EXPECT_TRUE(to_encoding(enc, eh, FINAL));
  EXPECT_STREQ("XCDR2 Little Endian Plain", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_D_CDR2_BE_APPENDABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_D_CDR2_BE);
  Encoding enc;
  EXPECT_TRUE(to_encoding(enc, eh, APPENDABLE));
  EXPECT_STREQ("XCDR2 Big Endian Delimited", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_D_CDR2_LE_APPENDABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_D_CDR2_LE);
  Encoding enc;
  EXPECT_TRUE(to_encoding(enc, eh, APPENDABLE));
  EXPECT_STREQ("XCDR2 Little Endian Delimited", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_PL_CDR2_BE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_PL_CDR2_BE);
  Encoding enc;
  EXPECT_TRUE(to_encoding(enc, eh, MUTABLE));
  EXPECT_STREQ("XCDR2 Big Endian Parameter List", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_PL_CDR2_LE_MUTABLE)
{
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_PL_CDR2_LE);
  Encoding enc;
  EXPECT_TRUE(to_encoding(enc, eh, MUTABLE));
  EXPECT_STREQ("XCDR2 Little Endian Parameter List", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_XML)
{
  OpenDDS::DCPS::LogRestore restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::None);
  EncapsulationHeader eh;
  eh.kind(EncapsulationHeader::KIND_XML);
  Encoding enc;
  EXPECT_FALSE(to_encoding(enc, eh, MUTABLE));
  EXPECT_STREQ("XML", eh.to_string().c_str());
}

TEST(dds_DCPS_EncapsulationHeader, to_encoding_INVALID)
{
  OpenDDS::DCPS::LogRestore restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::None);
  Encoding initenc(Encoding::KIND_UNALIGNED_CDR, ENDIAN_LITTLE);
  EncapsulationHeader eh(initenc, FINAL);
  Encoding enc;
  EXPECT_FALSE(to_encoding(enc, eh, FINAL));
  EXPECT_STREQ("Invalid", eh.to_string().c_str());
}
