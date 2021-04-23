#include "typecodeTypeSupportImpl.h"

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(TestTypecodes, can_allocate)
{
  ASSERT_EQ(my_module::_tc_my_struct, my_module::_tc_my_struct);
  ASSERT_EQ(my_module::_tc_my_union, my_module::_tc_my_union);
  ASSERT_EQ(my_module::_tc_my_long_seq_bound, my_module::_tc_my_long_seq_bound);
  ASSERT_EQ(my_module::_tc_my_long_seq_unbound, my_module::_tc_my_long_seq_unbound);
  ASSERT_EQ(my_module::_tc_my_long_array, my_module::_tc_my_long_array);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
