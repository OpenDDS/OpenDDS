/***
 * This test is meant to test the annotations set by typeflags.
 * Currently IS_AUTOID_HASH is not implemented so that should always return false.
 ***/
#include "typecodeTypeSupportImpl.h"

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(TestAny, flags_match)
{
  typecode::my_struct struct_in;
  struct_in.my_long = 42;
  typecode::my_struct * struct_out_ptr;
  CORBA::Any my_any;
  my_any <<= struct_in;
  my_any >>= struct_out_ptr;

  EXPECT_EQ(struct_out_ptr->my_long, 42);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
