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
//test various alias extensibilities
TEST(TestAlias, flags_match) {
  const TypeObject& to_sf = getMinimalTypeObject<extensibility_struct_final_alias_xtag>();
  EXPECT_TRUE(to_sf.minimal.alias_type.alias_flags & IS_FINAL);
  EXPECT_FALSE(to_sf.minimal.alias_type.alias_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_sf.minimal.alias_type.alias_flags & IS_MUTABLE);
  EXPECT_FALSE(to_sf.minimal.alias_type.alias_flags & IS_NESTED);
  EXPECT_FALSE(to_sf.minimal.alias_type.alias_flags & IS_AUTOID_HASH);

  const TypeObject& to_sa = getMinimalTypeObject<extensibility_struct_appendable_alias_xtag>();
  EXPECT_FALSE(to_sa.minimal.alias_type.alias_flags & IS_FINAL);
  EXPECT_TRUE(to_sa.minimal.alias_type.alias_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_sa.minimal.alias_type.alias_flags & IS_MUTABLE);
  EXPECT_FALSE(to_sa.minimal.alias_type.alias_flags & IS_NESTED);
  EXPECT_FALSE(to_sa.minimal.alias_type.alias_flags & IS_AUTOID_HASH);

  const TypeObject& to_sm = getMinimalTypeObject<extensibility_struct_mutable_alias_xtag>();
  EXPECT_FALSE(to_sm.minimal.alias_type.alias_flags & IS_FINAL);
  EXPECT_FALSE(to_sm.minimal.alias_type.alias_flags & IS_APPENDABLE);
  EXPECT_TRUE(to_sm.minimal.alias_type.alias_flags & IS_MUTABLE);
  EXPECT_FALSE(to_sm.minimal.alias_type.alias_flags & IS_NESTED);
  EXPECT_FALSE(to_sm.minimal.alias_type.alias_flags & IS_AUTOID_HASH);

  const TypeObject& to_sd = getMinimalTypeObject<extensibility_struct_default_alias_xtag>();
  EXPECT_FALSE(to_sd.minimal.alias_type.alias_flags & IS_FINAL);
  EXPECT_TRUE(to_sd.minimal.alias_type.alias_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_sd.minimal.alias_type.alias_flags & IS_MUTABLE);
  EXPECT_FALSE(to_sd.minimal.alias_type.alias_flags & IS_NESTED);
  EXPECT_FALSE(to_sd.minimal.alias_type.alias_flags & IS_AUTOID_HASH);

  const TypeObject& to_uf = getMinimalTypeObject<extensibility_union_final_alias_xtag>();
  EXPECT_TRUE(to_uf.minimal.alias_type.alias_flags & IS_FINAL);
  EXPECT_FALSE(to_uf.minimal.alias_type.alias_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_uf.minimal.alias_type.alias_flags & IS_MUTABLE);
  EXPECT_FALSE(to_uf.minimal.alias_type.alias_flags & IS_NESTED);
  EXPECT_FALSE(to_uf.minimal.alias_type.alias_flags & IS_AUTOID_HASH);

  const TypeObject& to_ua = getMinimalTypeObject<extensibility_union_appendable_alias_xtag>();
  EXPECT_FALSE(to_ua.minimal.alias_type.alias_flags & IS_FINAL);
  EXPECT_TRUE(to_ua.minimal.alias_type.alias_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_ua.minimal.alias_type.alias_flags & IS_MUTABLE);
  EXPECT_FALSE(to_ua.minimal.alias_type.alias_flags & IS_NESTED);
  EXPECT_FALSE(to_ua.minimal.alias_type.alias_flags & IS_AUTOID_HASH);

  const TypeObject& to_um = getMinimalTypeObject<extensibility_union_mutable_alias_xtag>();
  EXPECT_FALSE(to_um.minimal.alias_type.alias_flags & IS_FINAL);
  EXPECT_FALSE(to_um.minimal.alias_type.alias_flags & IS_APPENDABLE);
  EXPECT_TRUE(to_um.minimal.alias_type.alias_flags & IS_MUTABLE);
  EXPECT_FALSE(to_um.minimal.alias_type.alias_flags & IS_NESTED);
  EXPECT_FALSE(to_um.minimal.alias_type.alias_flags & IS_AUTOID_HASH);

  const TypeObject& to_ud = getMinimalTypeObject<extensibility_union_default_alias_xtag>();
  EXPECT_FALSE(to_ud.minimal.alias_type.alias_flags & IS_FINAL);
  EXPECT_TRUE(to_ud.minimal.alias_type.alias_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_ud.minimal.alias_type.alias_flags & IS_MUTABLE);
  EXPECT_FALSE(to_ud.minimal.alias_type.alias_flags & IS_NESTED);
  EXPECT_FALSE(to_ud.minimal.alias_type.alias_flags & IS_AUTOID_HASH);

  const TypeObject& to_ef = getMinimalTypeObject<extensibility_enum_final_alias_xtag>();
  EXPECT_TRUE(to_ef.minimal.alias_type.alias_flags & IS_FINAL);
  EXPECT_FALSE(to_ef.minimal.alias_type.alias_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_ef.minimal.alias_type.alias_flags & IS_MUTABLE);
  EXPECT_FALSE(to_ef.minimal.alias_type.alias_flags & IS_NESTED);
  EXPECT_FALSE(to_ef.minimal.alias_type.alias_flags & IS_AUTOID_HASH);

  const TypeObject& to_ea = getMinimalTypeObject<extensibility_enum_appendable_alias_xtag>();
  EXPECT_FALSE(to_ea.minimal.alias_type.alias_flags & IS_FINAL);
  EXPECT_TRUE(to_ea.minimal.alias_type.alias_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_ea.minimal.alias_type.alias_flags & IS_MUTABLE);
  EXPECT_FALSE(to_ea.minimal.alias_type.alias_flags & IS_NESTED);
  EXPECT_FALSE(to_ea.minimal.alias_type.alias_flags & IS_AUTOID_HASH);

  const TypeObject& to_ed = getMinimalTypeObject<extensibility_enum_default_alias_xtag>();
  EXPECT_FALSE(to_ed.minimal.alias_type.alias_flags & IS_FINAL);
  EXPECT_TRUE(to_ed.minimal.alias_type.alias_flags & IS_APPENDABLE);
  EXPECT_FALSE(to_ed.minimal.alias_type.alias_flags & IS_MUTABLE);
  EXPECT_FALSE(to_ed.minimal.alias_type.alias_flags & IS_NESTED);
  EXPECT_FALSE(to_ed.minimal.alias_type.alias_flags & IS_AUTOID_HASH);

}
int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
