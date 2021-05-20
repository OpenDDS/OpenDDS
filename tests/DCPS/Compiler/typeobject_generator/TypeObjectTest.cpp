#include "TypeObjectTestTypeSupportImpl.h"

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;

void verify_typemap(const TypeMap& type_map)
{
  
}

TEST(TypeMapTest, Minimal)
{
  const TypeMap& minimal_map = getMinimalTypeMap<A_xtag>();
  verify_typemap(minimal_map);
}

TEST(TypeMapTest, Complete)
{
  const TypeMap& complete_map = getCompleteTypeMap<A_xtag>();
  verify_typemap(complete_map);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
