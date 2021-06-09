#include "typecodeTypeSupportImpl.h"

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(TestTypecodes, can_allocate)
{
  ASSERT_TRUE(my_module::_tc_my_struct);
  ASSERT_TRUE(my_module::_tc_my_union);
  ASSERT_TRUE(my_module::_tc_my_long_seq_bound);
  ASSERT_TRUE(my_module::_tc_my_long_seq_unbound);
  ASSERT_TRUE(my_module::_tc_my_long_array);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
