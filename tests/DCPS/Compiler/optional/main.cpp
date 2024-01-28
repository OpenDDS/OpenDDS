#include "optionalC.h"
#include "optionalTypeSupportImpl.h"

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/TypeSupportImpl.h>
#include <dds/DCPS/FilterEvaluator.h>

#include <ace/ACE.h>
#include <ace/Log_Msg.h>
#include <gtest/gtest.h>

// Ensure opendds_idl is generating std::optional
TEST(OptionalTests, Empty)
{
  optional::OptionalMembers empty;
  EXPECT_FALSE(empty.opt().has_value());
  empty.opt(12);
  EXPECT_TRUE(empty.opt().has_value());
}

TEST(OptionalTests, SerializationSizeXCDR1)
{
  optional::OptionalMembers empty;
  OpenDDS::DCPS::Encoding encoding;
  encoding.kind(OpenDDS::DCPS::Encoding::KIND_XCDR1);
  EXPECT_EQ(12, OpenDDS::DCPS::serialized_size(encoding, empty)); // TODO this is probably incorrect

  optional::OptionalMembers notEmpty;
  notEmpty.opt(12);
  notEmpty.strOpt("Hello World");
  notEmpty.seqOpt(std::vector{123, 456});
  EXPECT_EQ(44, OpenDDS::DCPS::serialized_size(encoding, empty)); // TODO this is probably incorrect
}

TEST(OptionalTests, SerializationSizeXCDR2)
{
  optional::OptionalMembers empty{};
  OpenDDS::DCPS::Encoding encoding;
  encoding.kind(OpenDDS::DCPS::Encoding::KIND_XCDR2);
  EXPECT_EQ(3, OpenDDS::DCPS::serialized_size(encoding, empty));

  optional::OptionalMembers notEmpty;
  notEmpty.opt(12);
  notEmpty.strOpt("Hello World");
  notEmpty.seqOpt(std::vector{123, 456});
  EXPECT_EQ(44, OpenDDS::DCPS::serialized_size(encoding, empty)); // TODO this is probably incorrect
}

TEST(OptionalTests, SerializationXCDR1)
{
  OpenDDS::DCPS::Encoding encoding;
  encoding.kind(OpenDDS::DCPS::Encoding::KIND_XCDR1);

  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), encoding);

  optional::OptionalMembers msg;
  msg.opt(12);
  msg.strOpt("Hello World");
  EXPECT_TRUE(strm << msg);

 optional::OptionalMembers msg2;
 EXPECT_TRUE(strm >> msg2);
 EXPECT_TRUE(msg2.opt().has_value());
 EXPECT_EQ(msg.opt().value(), msg2.opt().value());
 EXPECT_FALSE(msg2.seqOpt().has_value());
}

TEST(OptionalTests, SerializationXCDR2)
{
  OpenDDS::DCPS::Encoding encoding;
  encoding.kind(OpenDDS::DCPS::Encoding::KIND_XCDR2);

  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), encoding);

  optional::OptionalMembers msg;
  msg.opt(12);
  msg.strOpt("Hello World");
  EXPECT_TRUE(strm << msg);

 optional::OptionalMembers msg2;
 EXPECT_TRUE(strm >> msg2);
 EXPECT_TRUE(msg2.opt().has_value());
 EXPECT_EQ(msg.opt().value(), msg2.opt().value());
 EXPECT_FALSE(msg2.seqOpt().has_value());
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
