/***
 * This test is meant to test the annotations set by typeflags.  
 * Currently IS_AUTOID_HASH is not implemented so that should always return false.
 ***/
#include "gtest/gtest.h"

#include "ExtensibilityTypeSupportImpl.h"
#include "dds/DCPS/TypeObject.h"

using namespace extensibility;
using namespace OpenDDS::XTypes;
using namespace OpenDDS::DCPS;

//tests the types capable of being FINAL with and without IS_NESTED
TEST(TestFinal, flags_match) {
  const TypeObject& to_s = getMinimalTypeObject<extensibility_struct_final_xtag>();
  EXPECT_TRUE(to_s.minimal.struct_type.struct_flags & IS_FINAL);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_MUTABLE);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_NESTED);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_AUTOID_HASH);

  const TypeObject& to_u = getMinimalTypeObject<extensibility_union_final_xtag>();
  EXPECT_TRUE(to_u.minimal.union_type.union_flags & IS_FINAL);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_MUTABLE);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_NESTED);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_AUTOID_HASH);

  const TypeObject& to_e = getMinimalTypeObject<extensibility_enum_final_xtag>();
  EXPECT_TRUE(to_e.minimal.enumerated_type.enum_flags & IS_FINAL);
  EXPECT_FALSE(to_e.minimal.enumerated_type.enum_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_e.minimal.enumerated_type.enum_flags & IS_MUTABLE);
  EXPECT_FALSE(to_e.minimal.enumerated_type.enum_flags & IS_NESTED);
  EXPECT_FALSE(to_e.minimal.enumerated_type.enum_flags & IS_AUTOID_HASH);

  const TypeObject& to_sn = getMinimalTypeObject<extensibility_struct_final_nested_xtag>();
  EXPECT_TRUE(to_sn.minimal.struct_type.struct_flags & IS_FINAL);
  EXPECT_FALSE(to_sn.minimal.struct_type.struct_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_sn.minimal.struct_type.struct_flags & IS_MUTABLE);
  EXPECT_TRUE(to_sn.minimal.struct_type.struct_flags & IS_NESTED);
  EXPECT_FALSE(to_sn.minimal.struct_type.struct_flags & IS_AUTOID_HASH);

  const TypeObject& to_un = getMinimalTypeObject<extensibility_union_final_nested_xtag>();
  EXPECT_TRUE(to_un.minimal.union_type.union_flags & IS_FINAL);
  EXPECT_FALSE(to_un.minimal.union_type.union_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_un.minimal.union_type.union_flags & IS_MUTABLE);
  EXPECT_TRUE(to_un.minimal.union_type.union_flags & IS_NESTED);
  EXPECT_FALSE(to_un.minimal.union_type.union_flags & IS_AUTOID_HASH);
}

//tests the types capable of being APPENDABLE with and without IS_NESTED
TEST(TestAppendable, flags_match) {
  const TypeObject& to_s = getMinimalTypeObject<extensibility_struct_appendable_xtag>();
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_FINAL);
  EXPECT_TRUE(to_s.minimal.struct_type.struct_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_MUTABLE);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_NESTED);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_AUTOID_HASH);

  const TypeObject& to_u = getMinimalTypeObject<extensibility_union_appendable_xtag>();
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_FINAL);
  EXPECT_TRUE(to_u.minimal.union_type.union_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_MUTABLE);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_NESTED);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_AUTOID_HASH);

  const TypeObject& to_e = getMinimalTypeObject<extensibility_enum_appendable_xtag>();
  EXPECT_FALSE(to_e.minimal.enumerated_type.enum_flags & IS_FINAL);
  EXPECT_TRUE(to_e.minimal.enumerated_type.enum_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_e.minimal.enumerated_type.enum_flags & IS_NESTED);
  EXPECT_FALSE(to_e.minimal.enumerated_type.enum_flags & IS_MUTABLE);
  EXPECT_FALSE(to_e.minimal.enumerated_type.enum_flags & IS_AUTOID_HASH);

  const TypeObject& to_sn = getMinimalTypeObject<extensibility_struct_appendable_nested_xtag>();
  EXPECT_FALSE(to_sn.minimal.struct_type.struct_flags & IS_FINAL);
  EXPECT_TRUE(to_sn.minimal.struct_type.struct_flags & IS_APPENDABLE);
  EXPECT_TRUE(to_sn.minimal.struct_type.struct_flags & IS_NESTED);
  EXPECT_FALSE(to_sn.minimal.struct_type.struct_flags & IS_MUTABLE);
  EXPECT_FALSE(to_sn.minimal.struct_type.struct_flags & IS_AUTOID_HASH);

  const TypeObject& to_un = getMinimalTypeObject<extensibility_union_appendable_nested_xtag>();
  EXPECT_FALSE(to_un.minimal.union_type.union_flags & IS_FINAL);
  EXPECT_TRUE(to_un.minimal.union_type.union_flags & IS_APPENDABLE);
  EXPECT_TRUE(to_un.minimal.union_type.union_flags & IS_NESTED);
  EXPECT_FALSE(to_un.minimal.union_type.union_flags & IS_MUTABLE);
  EXPECT_FALSE(to_un.minimal.union_type.union_flags & IS_AUTOID_HASH);
}

//tests the types capable of being MUTABLE with and without IS_NESTED
TEST(TestMutable, flags_match) {
  const TypeObject& to_s = getMinimalTypeObject<extensibility_struct_mutable_xtag>();
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_FINAL);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_APPENDABLE);
  EXPECT_TRUE(to_s.minimal.struct_type.struct_flags & IS_MUTABLE);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_NESTED);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_AUTOID_HASH);

  const TypeObject& to_u = getMinimalTypeObject<extensibility_union_mutable_xtag>();
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_FINAL);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_APPENDABLE);
  EXPECT_TRUE(to_u.minimal.union_type.union_flags & IS_MUTABLE);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_NESTED);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_AUTOID_HASH);

  const TypeObject& to_sn = getMinimalTypeObject<extensibility_struct_mutable_nested_xtag>();
  EXPECT_FALSE(to_sn.minimal.struct_type.struct_flags & IS_FINAL);
  EXPECT_FALSE(to_sn.minimal.struct_type.struct_flags & IS_APPENDABLE);
  EXPECT_TRUE(to_sn.minimal.struct_type.struct_flags & IS_MUTABLE);
  EXPECT_TRUE(to_sn.minimal.struct_type.struct_flags & IS_NESTED);
  EXPECT_FALSE(to_sn.minimal.struct_type.struct_flags & IS_AUTOID_HASH);

  const TypeObject& to_un = getMinimalTypeObject<extensibility_union_mutable_nested_xtag>();
  EXPECT_FALSE(to_un.minimal.union_type.union_flags & IS_FINAL);
  EXPECT_FALSE(to_un.minimal.union_type.union_flags & IS_APPENDABLE);
  EXPECT_TRUE(to_un.minimal.union_type.union_flags & IS_MUTABLE);
  EXPECT_TRUE(to_un.minimal.union_type.union_flags & IS_NESTED);
  EXPECT_FALSE(to_un.minimal.union_type.union_flags & IS_AUTOID_HASH);
}

//tests the types capable of being APPENDABLE when the extensibility is not explicitly set.
//Done with and without IS_NESTED
TEST(TestDefault, flags_match) {
  const TypeObject& to_s = getMinimalTypeObject<extensibility_struct_default_xtag>();
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_FINAL);
  EXPECT_TRUE(to_s.minimal.struct_type.struct_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_MUTABLE);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_NESTED);
  EXPECT_FALSE(to_s.minimal.struct_type.struct_flags & IS_AUTOID_HASH);

  const TypeObject& to_u = getMinimalTypeObject<extensibility_union_default_xtag>();
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_FINAL);
  EXPECT_TRUE(to_u.minimal.union_type.union_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_MUTABLE);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_NESTED);
  EXPECT_FALSE(to_u.minimal.union_type.union_flags & IS_AUTOID_HASH);

  const TypeObject& to_e = getMinimalTypeObject<extensibility_enum_default_xtag>();
  EXPECT_FALSE(to_e.minimal.enumerated_type.enum_flags & IS_FINAL);
  EXPECT_TRUE(to_e.minimal.enumerated_type.enum_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_e.minimal.enumerated_type.enum_flags & IS_MUTABLE);
  EXPECT_FALSE(to_e.minimal.enumerated_type.enum_flags & IS_NESTED);
  EXPECT_FALSE(to_e.minimal.enumerated_type.enum_flags & IS_AUTOID_HASH);

  const TypeObject& to_sn = getMinimalTypeObject<extensibility_struct_default_nested_xtag>();
  EXPECT_FALSE(to_sn.minimal.struct_type.struct_flags & IS_FINAL);
  EXPECT_TRUE(to_sn.minimal.struct_type.struct_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_sn.minimal.struct_type.struct_flags & IS_MUTABLE);
  EXPECT_TRUE(to_sn.minimal.struct_type.struct_flags & IS_NESTED);
  EXPECT_FALSE(to_sn.minimal.struct_type.struct_flags & IS_AUTOID_HASH);

  const TypeObject& to_un = getMinimalTypeObject<extensibility_union_default_nested_xtag>();
  EXPECT_FALSE(to_un.minimal.union_type.union_flags & IS_FINAL);
  EXPECT_TRUE(to_un.minimal.union_type.union_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_un.minimal.union_type.union_flags & IS_MUTABLE);
  EXPECT_TRUE(to_un.minimal.union_type.union_flags & IS_NESTED);
  EXPECT_FALSE(to_un.minimal.union_type.union_flags & IS_AUTOID_HASH);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
