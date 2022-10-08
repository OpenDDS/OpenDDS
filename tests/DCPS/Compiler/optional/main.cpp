#include <gtest/gtest.h>

#include "testTypeSupportC.h"

TEST(Optional, OptionalTest) {
  Data d;
  d.opt_data(true);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}