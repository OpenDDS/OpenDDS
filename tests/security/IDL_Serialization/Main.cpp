#include <dds/DdsDcpsCoreTypeSupportImpl.h>

#include <gtest/gtest.h>

using OpenDDS::DCPS::Encoding;
const Encoding encoding_plain_big(Encoding::KIND_XCDR1, OpenDDS::DCPS::ENDIAN_BIG);

void serializePropertySeq(ACE_Message_Block& mb, const char* str) {
  mb.init(100);
  OpenDDS::DCPS::Serializer s(&mb, encoding_plain_big);
  DDS::PropertySeq seq(1);
  seq.length(1);
  seq[0].name = "a";
  seq[0].value = str;
  seq[0].propagate = true;
  s << seq;
}

TEST(OptionalBinaryProperties, Deserialize)
{
  ACE_Message_Block mb;
  DDS::PropertyQosPolicy pol;

  serializePropertySeq(mb, "foo"); // aligned
  OpenDDS::DCPS::Serializer s0(&mb, encoding_plain_big);
  EXPECT_TRUE(s0 >> pol);
  mb.rd_ptr(mb.base());

  serializePropertySeq(mb, "fo");  // 1 pad byte
  OpenDDS::DCPS::Serializer s1(&mb, encoding_plain_big);
  EXPECT_TRUE(s1 >> pol);
  mb.rd_ptr(mb.base());

  serializePropertySeq(mb, "f");   // 2 pad bytes
  OpenDDS::DCPS::Serializer s2(&mb, encoding_plain_big);
  EXPECT_TRUE(s2 >> pol);
  mb.rd_ptr(mb.base());

  serializePropertySeq(mb, "");    // 3 pad bytes
  OpenDDS::DCPS::Serializer s3(&mb, encoding_plain_big);
  EXPECT_TRUE(s3 >> pol);
  mb.rd_ptr(mb.base());
}


int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
