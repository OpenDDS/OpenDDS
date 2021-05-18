#include <testTypeSupportImpl.h>

#include <gtest/gtest.h>

#if CPP11_MAPPING
#  define REF(WHAT) (WHAT)()
#else
#  define REF(WHAT) (WHAT)
#endif

TEST(EscapedNonKeywords, struct_topic_type)
{
  the_module::the_struct a;
  REF(a.the_field) = the_module::the_const;
  ACE_UNUSED_ARG(a);
  the_module::the_structDataWriter* a_dw = 0;
  ACE_UNUSED_ARG(a_dw);

  the_module::the_structTypeSupportImpl ts;
  EXPECT_STREQ(ts.default_type_name(), "the_module::the_struct");
}

TEST(EscapedNonKeywords, union_topic_type)
{
  the_module::the_union au;
  au.the_field(the_module::the_const);
  ACE_UNUSED_ARG(au);
  the_module::the_unionDataWriter* au_dw = 0;
  ACE_UNUSED_ARG(au_dw);

  the_module::the_unionTypeSupportImpl ts;
  EXPECT_STREQ(ts.default_type_name(), "the_module::the_union");
}

TEST(IdlKeywords, struct_topic_type)
{
  boolean::attribute b;
  REF(b.component) = boolean::oneway;
  ACE_UNUSED_ARG(b);
  boolean::attributeDataWriter* b_dw = 0;
  ACE_UNUSED_ARG(b_dw);

  boolean::attributeTypeSupportImpl ts;
  EXPECT_STREQ(ts.default_type_name(), "boolean::attribute");
}

TEST(IdlKeywords, union_topic_type)
{
  boolean::primarykey bu;
  bu.truncatable(boolean::oneway);
  ACE_UNUSED_ARG(bu);
  boolean::primarykeyDataWriter* bu_dw = 0;
  ACE_UNUSED_ARG(bu_dw);

  boolean::primarykeyTypeSupportImpl ts;
  EXPECT_STREQ(ts.default_type_name(), "boolean::primarykey");
}

TEST(CppKeywords, struct_topic_type)
{
  _cxx_bool::_cxx_class c;
  REF(c._cxx_else) = _cxx_bool::_cxx_continue;
  ACE_UNUSED_ARG(c);
  _cxx_bool::classDataWriter* c_dw = 0;
  ACE_UNUSED_ARG(c_dw);

  _cxx_bool::classTypeSupportImpl ts;
  EXPECT_STREQ(ts.default_type_name(), "bool::class");
}

TEST(CppKeywords, union_topic_type)
{
  _cxx_bool::_cxx_goto cu;
  cu._cxx_asm(_cxx_bool::_cxx_continue);
  ACE_UNUSED_ARG(cu);
  _cxx_bool::gotoDataWriter* cu_dw = 0;
  ACE_UNUSED_ARG(cu_dw);

  _cxx_bool::gotoTypeSupportImpl ts;
  EXPECT_STREQ(ts.default_type_name(), "bool::goto");
}

TEST(DoubleKeywords, struct_topic_type)
{
  _cxx_case::_cxx_struct d;
  REF(d._cxx_private) = _cxx_case::_cxx_typeid;
  ACE_UNUSED_ARG(d);
  _cxx_case::structDataWriter* d_dw = 0;
  ACE_UNUSED_ARG(d_dw);

  _cxx_case::structTypeSupportImpl ts;
  EXPECT_STREQ(ts.default_type_name(), "case::struct");
}

TEST(DoubleKeywords, union_topic_type)
{
  _cxx_case::_cxx_union du;
  du._cxx_public(_cxx_case::_cxx_typeid);
  ACE_UNUSED_ARG(du);
  _cxx_case::unionDataWriter* du_dw = 0;
  ACE_UNUSED_ARG(du_dw);

  _cxx_case::unionTypeSupportImpl ts;
  EXPECT_STREQ(ts.default_type_name(), "case::union");
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
