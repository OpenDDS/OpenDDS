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

int main(int argc, char ** argv)
{  
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
