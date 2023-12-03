#include "optionalC.h"
#include "optionalTypeSupportImpl.h"

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/TypeSupportImpl.h>
#include <dds/DCPS/FilterEvaluator.h>

#include <ace/ACE.h>
#include <ace/Log_Msg.h>
#include <gtest/gtest.h>

TEST(OptionalTests, Empty)
{
  optional::OptionalMembers empty;
  EXPECT_FALSE(empty.opt().has_value());
  empty.opt(12);
  EXPECT_TRUE(empty.opt().has_value());
}

TEST(OptionalTests, SerializationSize)
{
  optional::OptionalMembers empty;
  OpenDDS::DCPS::Encoding encoding;
  encoding.kind(OpenDDS::DCPS::Encoding::KIND_XCDR1);

  EXPECT_EQ(1, OpenDDS::DCPS::serialized_size(encoding, empty));
  empty.opt(12);
  // 8 Because of alignment? Should be 5: 1 has_value bytes, 4 for the int
  EXPECT_EQ(8, OpenDDS::DCPS::serialized_size(encoding, empty));
}

TEST(OptionalTests, Serialization)
{
  OpenDDS::DCPS::Encoding encoding;
  encoding.kind(OpenDDS::DCPS::Encoding::KIND_XCDR1);

  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), encoding);

  optional::OptionalMembers msg;
  msg.opt(12);
  EXPECT_TRUE(strm << msg);

  optional::OptionalMembers msg2;
  EXPECT_TRUE(strm >> msg2);
  EXPECT_TRUE(msg2.opt().has_value());
  EXPECT_EQ(msg.opt().value(), msg2.opt().value());
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
