#include <testTypeSupportImpl.h>

#include <dds/DCPS/Definitions.h>

#include <gtest/gtest.h>

int main(int argc, char* argv[])
{
  Data d;
  d.seqData().push_back(1);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
