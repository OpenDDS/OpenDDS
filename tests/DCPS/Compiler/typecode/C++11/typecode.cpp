#include "typecodeTypeSupportImpl.h"

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(TestTypecodes, can_allocate)
{
  ASSERT_EQ(my_module::_tc_my_struct, nullptr);
  ASSERT_EQ(my_module::_tc_my_union, nullptr);
  ASSERT_EQ(my_module::_tc_my_long_seq_bound, nullptr);
  ASSERT_EQ(my_module::_tc_my_long_seq_unbound, nullptr);
  ASSERT_EQ(my_module::_tc_my_long_array, nullptr);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
