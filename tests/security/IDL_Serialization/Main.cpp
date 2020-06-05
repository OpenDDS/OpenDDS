
#include "gtest/gtest.h"

#include "dds/DdsDcpsCoreTypeSupportImpl.h"

const OpenDDS::DCPS::Encoding encoding_plain_big(OpenDDS::DCPS::Encoding::KIND_CDR_PLAIN, OpenDDS::DCPS::ENDIAN_BIG);

void serializePropertySeq(ACE_Message_Block& mb, const char* str) {
  mb.init(100);
  Serializer s(&mb, encoding_plain_big);
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
  Serializer s0(&mb, encoding_plain_big);
  EXPECT_TRUE(s0 >> pol);
  mb.rd_ptr(mb.base());

  serializePropertySeq(mb, "fo");  // 1 pad byte
  Serializer s1(&mb, encoding_plain_big);
  EXPECT_TRUE(s1 >> pol);
  mb.rd_ptr(mb.base());

  serializePropertySeq(mb, "f");   // 2 pad bytes
  Serializer s2(&mb, encoding_plain_big);
  EXPECT_TRUE(s2 >> pol);
  mb.rd_ptr(mb.base());

  serializePropertySeq(mb, "");    // 3 pad bytes
  Serializer s3(&mb, encoding_plain_big);
  EXPECT_TRUE(s3 >> pol);
  mb.rd_ptr(mb.base());
}


int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
