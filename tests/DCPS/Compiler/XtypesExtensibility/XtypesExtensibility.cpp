/***
 * This test is meant to test the annotations set by typeflags.
 * Currently IS_AUTOID_HASH is not implemented so that should always return false.
 ***/
#include "ExtensibilityTypeSupportImpl.h"

#include <dds/DCPS/XTypes/TypeObject.h>

#include <gtest/gtest.h>

using namespace extensibility;
using namespace OpenDDS::XTypes;
using namespace OpenDDS::DCPS;

//tests the types capable of being FINAL with and without IS_NESTED
TEST(TestFinal, flags_match)
{
  TypeMap type_map = getMinimalTypeMap<extensibility_struct_final_xtag>();

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_struct_final_xtag>()]
            .minimal.struct_type.struct_flags, IS_FINAL);

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_union_final_xtag>()]
            .minimal.union_type.union_flags, IS_FINAL);

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_struct_final_nested_xtag>()]
            .minimal.struct_type.struct_flags , IS_FINAL | IS_NESTED);

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_union_final_nested_xtag>()]
            .minimal.union_type.union_flags, IS_FINAL | IS_NESTED);
}

//tests the types capable of being APPENDABLE with and without IS_NESTED
TEST(TestAppendable, flags_match)
{
  TypeMap type_map = getMinimalTypeMap<extensibility_struct_final_xtag>();

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_struct_appendable_xtag>()]
            .minimal.struct_type.struct_flags, IS_APPENDABLE);

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_union_appendable_xtag>()]
            .minimal.union_type.union_flags, IS_APPENDABLE);

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_struct_appendable_nested_xtag>()]
            .minimal.struct_type.struct_flags , IS_APPENDABLE | IS_NESTED);

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_union_appendable_nested_xtag>()]
            .minimal.union_type.union_flags, IS_APPENDABLE | IS_NESTED);
}

//tests the types capable of being MUTABLE with and without IS_NESTED
TEST(TestMutable, flags_match)
{
  TypeMap type_map = getMinimalTypeMap<extensibility_struct_final_xtag>();

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_struct_mutable_xtag>()]
            .minimal.struct_type.struct_flags, IS_MUTABLE);

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_union_mutable_xtag>()]
            .minimal.union_type.union_flags, IS_MUTABLE);

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_struct_mutable_nested_xtag>()]
            .minimal.struct_type.struct_flags , IS_MUTABLE | IS_NESTED);

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_union_mutable_nested_xtag>()]
            .minimal.union_type.union_flags, IS_MUTABLE | IS_NESTED);
}

//tests the types capable of being APPENDABLE when the extensibility is not explicitly set.
//Done with and without IS_NESTED
TEST(TestDefault, flags_match)
{
  TypeMap type_map = getMinimalTypeMap<extensibility_struct_final_xtag>();

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_struct_default_xtag>()]
            .minimal.struct_type.struct_flags, IS_APPENDABLE);

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_union_default_xtag>()]
            .minimal.union_type.union_flags, IS_APPENDABLE);

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_struct_default_nested_xtag>()]
            .minimal.struct_type.struct_flags , IS_APPENDABLE | IS_NESTED);

  EXPECT_EQ(type_map[getMinimalTypeIdentifier<extensibility_union_default_nested_xtag>()]
            .minimal.union_type.union_flags, IS_APPENDABLE | IS_NESTED);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
