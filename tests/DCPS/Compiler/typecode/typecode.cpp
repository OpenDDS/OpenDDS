/***
 * This test is meant to test the annotations set by typeflags.
 * Currently IS_AUTOID_HASH is not implemented so that should always return false.
 ***/
#include "typecodeTypeSupportImpl.h"

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(TestTypecodes, can_allocate)
{
  my_module::_tc_my_struct;
  my_module::_tc_my_union;
  my_module::_tc_my_long_seq_bound;
  my_module::_tc_my_long_seq_unbound;
  my_module::_tc_my_long_array;
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
