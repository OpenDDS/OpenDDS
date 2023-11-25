#include "optionalC.h"
#include "optionalTypeSupportImpl.h"

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/TypeSupportImpl.h>
#include <dds/DCPS/FilterEvaluator.h>

#include <ace/ACE.h>
#include <ace/Log_Msg.h>
#include <gtest/gtest.h>

#include <cstdlib>

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
  
  EXPECT_EQ(0, OpenDDS::DCPS::serialized_size(encoding, empty));
  empty.opt(12);
  EXPECT_EQ(4, OpenDDS::DCPS::serialized_size(encoding, empty));
}

TEST(OptionalTests, Serialization)
{
  optional::OptionalMembers empty;
  OpenDDS::DCPS::Encoding encoding;
  encoding.kind(OpenDDS::DCPS::Encoding::KIND_XCDR1);
  
  ACE_Message_Block mb(4);
  OpenDDS::DCPS::Serializer serializer(&mb, encoding);
  EXPECT_TRUE(serializer << empty);
  EXPECT_EQ(0, mb.length());
  empty.opt(12);
  EXPECT_TRUE(serializer << empty);
  EXPECT_EQ(4, mb.length());

  optional::OptionalMembers empty2;
  EXPECT_TRUE(serializer >> empty2);
  EXPECT_FALSE(empty2.opt().has_value());
  EXPECT_EQ(empty.opt().value(), empty2.opt().value());
}

int main(int argc, char ** argv)
{  
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
