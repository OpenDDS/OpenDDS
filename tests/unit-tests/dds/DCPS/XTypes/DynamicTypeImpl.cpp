#ifndef OPENDDS_SAFETY_PROFILE

/*
TODO Untested Types:
Map
Annotation
Bitmask
Bitset
*/

#include "../../../CompleteToDynamicTypeTypeSupportImpl.h"
#include "../../../CompleteToMinimalTypeObjectTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/XTypes/TypeLookupService.h>
#include <dds/DCPS/XTypes/TypeObject.h>
#include <dds/DCPS/XTypes/TypeDescriptorImpl.h>
#include <dds/DCPS/XTypes/MemberDescriptorImpl.h>
#include <dds/DCPS/XTypes/DynamicTypeImpl.h>
#include <dds/DCPS/XTypes/DynamicTypeMemberImpl.h>

#include <gtest/gtest.h>

using namespace OpenDDS;

class dds_DCPS_XTypes_DynamicTypeImpl : public ::testing::Test {
  void SetUp()
  {
    tls_ = DCPS::make_rch<XTypes::TypeLookupService>();
    MoreSetup();
  }

  void MoreSetup();

  XTypes::TypeLookupService_rch tls_;

public:

  template<typename T>
  void test_conversion(const DDS::DynamicType_ptr expected_dynamic_type)
  {
    const XTypes::TypeIdentifier& com_ti = DCPS::getCompleteTypeIdentifier<T>();
    const XTypes::TypeMap& com_map = DCPS::getCompleteTypeMap<T>();
    XTypes::TypeMap::const_iterator pos = com_map.find(com_ti);
    EXPECT_TRUE(pos != com_map.end());
    const XTypes::TypeObject& com_to = pos->second;
    DCPS::GUID_t fake_guid = OpenDDS::DCPS::GUID_UNKNOWN;
    DDS::DynamicType_var converted_dt = tls_->complete_to_dynamic(com_to.complete, fake_guid);
    EXPECT_TRUE(expected_dynamic_type->equals(converted_dt));
    converted_dt->clear();
  }
};

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl, CompleteToDynamicType_MyInnerStruct)
{
  XTypes::DynamicTypeImpl* expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var expected_dt_var = expected_dt;
  DDS::TypeDescriptor_var expected_td = new XTypes::TypeDescriptorImpl();
  expected_td->kind(XTypes::TK_STRUCTURE);
  expected_td->name("::MyModCompleteToDynamic::MyInnerStruct");
  expected_td->bound().length(0);
  expected_td->extensibility_kind(DDS::MUTABLE);
  expected_td->is_nested(false);
  XTypes::DynamicTypeMemberImpl* expected_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var expected_dtm_var = expected_dtm;
  DDS::MemberDescriptor_var expected_md = new XTypes::MemberDescriptorImpl();
  //expected_dtm->set_parent(expected_dt);
  expected_md->name("l");
  expected_md->id(false);
  expected_md->index(false);
  expected_md->try_construct_kind(DDS::DISCARD);
  XTypes::DynamicTypeImpl* long_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var long_expected_dt_var = long_expected_dt;
  DDS::TypeDescriptor_var long_td = new XTypes::TypeDescriptorImpl();
  long_td->kind(XTypes::TK_INT32);
  long_td->bound().length(0);
  long_td->name("Int32");
  expected_md->type(long_expected_dt);
  expected_dt->set_descriptor(expected_td);

  DDS::TypeDescriptor_var td2;
  EXPECT_EQ(DDS::RETCODE_OK, expected_dt->get_descriptor(td2));
  EXPECT_EQ(expected_td, td2);
  // get_descriptor can be called with the argument already pointing to an object.
  // It yields the same result but we'll check the reference count as well.
  const unsigned int refcount = expected_td->_refcount_value();
  EXPECT_EQ(DDS::RETCODE_OK, expected_dt->get_descriptor(td2));
  EXPECT_EQ(expected_td, td2);
  EXPECT_EQ(refcount, expected_td->_refcount_value());

  expected_dtm->set_descriptor(expected_md);
  long_expected_dt->set_descriptor(long_td);
  expected_dt->insert_dynamic_member(expected_dtm);
  test_conversion<DCPS::MyModCompleteToDynamic_MyInnerStruct_xtag>(expected_dt);
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl, CompleteToDynamicType_MyOuterStruct)
{
  XTypes::DynamicTypeImpl* expected_outer_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var expected_outer_dt_var = expected_outer_dt;
  DDS::TypeDescriptor_var outer_td = new XTypes::TypeDescriptorImpl();
  outer_td->kind(XTypes::TK_STRUCTURE);
  outer_td->name("::MyModCompleteToDynamic::MyOuterStruct");
  outer_td->bound().length(0);
  outer_td->extensibility_kind(DDS::APPENDABLE);
  outer_td->is_nested(0);
  XTypes::DynamicTypeMemberImpl* expected_outer_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var expected_outer_dtm_var = expected_outer_dtm;
  DDS::MemberDescriptor_var expected_outer_md = new XTypes::MemberDescriptorImpl();
  //expected_outer_dtm->set_parent(expected_outer_dt);
  expected_outer_md->name("ms");
  expected_outer_md->id(0);
  expected_outer_md->index(0);
  expected_outer_md->try_construct_kind(DDS::DISCARD);
  XTypes::DynamicTypeImpl* expected_inner_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var expected_inner_dt_var = expected_inner_dt;
  DDS::TypeDescriptor_var inner_td = new XTypes::TypeDescriptorImpl();
  inner_td->kind(XTypes::TK_STRUCTURE);
  inner_td->name("::MyModCompleteToDynamic::MyInnerStruct");
  inner_td->bound().length(0);
  inner_td->extensibility_kind(DDS::MUTABLE);
  inner_td->is_nested(0);
  XTypes::DynamicTypeMemberImpl* expected_inner_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var expected_inner_dtm_var = expected_inner_dtm;
  DDS::MemberDescriptor_var expected_inner_md = new XTypes::MemberDescriptorImpl();
  //expected_inner_dtm->set_parent(expected_inner_dt);
  expected_inner_md->name("l");
  expected_inner_md->id(0);
  expected_inner_md->index(0);
  expected_inner_md->try_construct_kind(DDS::DISCARD);
  XTypes::DynamicTypeImpl* long_expected_inner_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var long_expected_inner_dt_var = long_expected_inner_dt;
  DDS::TypeDescriptor_var long_td = new XTypes::TypeDescriptorImpl();
  long_td->kind(XTypes::TK_INT32);
  long_td->bound().length(0);
  long_td->name("Int32");
  expected_inner_md->type(long_expected_inner_dt);
  expected_outer_md->type(expected_inner_dt);
  expected_outer_dt->set_descriptor(outer_td);
  expected_outer_dtm->set_descriptor(expected_outer_md);
  expected_inner_dt->set_descriptor(inner_td);
  expected_inner_dtm->set_descriptor(expected_inner_md);
  long_expected_inner_dt->set_descriptor(long_td);
  expected_inner_dt->insert_dynamic_member(expected_inner_dtm);
  expected_outer_dt->insert_dynamic_member(expected_outer_dtm);
  test_conversion<DCPS::MyModCompleteToDynamic_MyOuterStruct_xtag>(expected_outer_dt);
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl, CompleteToDynamicType_MyAliasStruct)
{
  XTypes::DynamicTypeImpl* expected_alias_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var expected_alias_dt_var = expected_alias_dt;
  DDS::TypeDescriptor_var alias_td = new XTypes::TypeDescriptorImpl();
  alias_td->kind(XTypes::TK_ALIAS);
  alias_td->name("::MyModCompleteToDynamic::MyAliasStruct");
  alias_td->bound().length(0);
  alias_td->extensibility_kind(DDS::FINAL);
  alias_td->is_nested(0);
  XTypes::DynamicTypeImpl* expected_outer_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var expected_outer_dt_var = expected_outer_dt;
  DDS::TypeDescriptor_var outer_td = new XTypes::TypeDescriptorImpl();
  outer_td->kind(XTypes::TK_STRUCTURE);
  outer_td->name("::MyModCompleteToDynamic::MyOuterStruct");
  outer_td->bound().length(0);
  outer_td->extensibility_kind(DDS::APPENDABLE);
  outer_td->is_nested(0);
  XTypes::DynamicTypeMemberImpl* expected_outer_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var expected_outer_dtm_var = expected_outer_dtm;
  DDS::MemberDescriptor_var expected_outer_md = new XTypes::MemberDescriptorImpl();
  //expected_outer_dtm->set_parent(expected_outer_dt);
  expected_outer_md->name("ms");
  expected_outer_md->id(0);
  expected_outer_md->index(0);
  expected_outer_md->try_construct_kind(DDS::DISCARD);
  XTypes::DynamicTypeImpl* expected_inner_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var expected_inner_dt_var = expected_inner_dt;
  DDS::TypeDescriptor_var inner_td = new XTypes::TypeDescriptorImpl();
  inner_td->kind(XTypes::TK_STRUCTURE);
  inner_td->name("::MyModCompleteToDynamic::MyInnerStruct");
  inner_td->bound().length(0);
  inner_td->extensibility_kind(DDS::MUTABLE);
  inner_td->is_nested(0);
  XTypes::DynamicTypeMemberImpl* expected_inner_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var expected_inner_dtm_var = expected_inner_dtm;
  DDS::MemberDescriptor_var expected_inner_md = new XTypes::MemberDescriptorImpl();
  //expected_inner_dtm->set_parent(expected_inner_dt);
  expected_inner_md->name("l");
  expected_inner_md->id(0);
  expected_inner_md->index(0);
  expected_inner_md->try_construct_kind(DDS::DISCARD);
  XTypes::DynamicTypeImpl* long_expected_inner_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var long_expected_inner_dt_var = long_expected_inner_dt;
  DDS::TypeDescriptor_var long_td = new XTypes::TypeDescriptorImpl();
  long_td->kind(XTypes::TK_INT32);
  long_td->bound().length(0);
  long_td->name("Int32");
  expected_inner_md->type(long_expected_inner_dt);
  expected_outer_md->type(expected_inner_dt);
  alias_td->base_type(expected_outer_dt);
  expected_alias_dt->set_descriptor(alias_td);
  expected_outer_dt->set_descriptor(outer_td);
  expected_outer_dtm->set_descriptor(expected_outer_md);
  expected_inner_dt->set_descriptor(inner_td);
  expected_inner_dtm->set_descriptor(expected_inner_md);
  long_expected_inner_dt->set_descriptor(long_td);
  expected_inner_dt->insert_dynamic_member(expected_inner_dtm);
  expected_outer_dt->insert_dynamic_member(expected_outer_dtm);
  test_conversion<DCPS::MyModCompleteToDynamic_MyAliasStruct_xtag>(expected_alias_dt);
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl, CompleteToDynamicType_PrimitiveKind)
{
  XTypes::DynamicTypeImpl* expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var expected_dt_var = expected_dt;
  DDS::TypeDescriptor_var td = new XTypes::TypeDescriptorImpl();
  td->kind(XTypes::TK_ENUM);
  td->name("::MyModCompleteToDynamic::PrimitiveKind");
  td->bound().length(1);
  td->bound()[0] = 32;
  td->extensibility_kind(DDS::FINAL);
  td->is_nested(0);
  XTypes::DynamicTypeMemberImpl* first_expected_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var first_expected_dtm_var = first_expected_dtm;
  DDS::MemberDescriptor_var first_expected_md = new XTypes::MemberDescriptorImpl();
  //first_expected_dtm->set_parent(expected_dt);
  first_expected_md->name("TK_INT32");
  first_expected_md->id(ACE_UINT32_MAX);
  first_expected_md->index(0);
  first_expected_md->is_default_label(1);
  first_expected_md->try_construct_kind(DDS::DISCARD);
  XTypes::DynamicTypeMemberImpl* second_expected_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var second_expected_dtm_var = second_expected_dtm;
  DDS::MemberDescriptor_var second_expected_md = new XTypes::MemberDescriptorImpl();
  //second_expected_dtm->set_parent(expected_dt);
  second_expected_md->name("TK_CHAR8");
  second_expected_md->id(ACE_UINT32_MAX);
  second_expected_md->index(1);
  second_expected_md->try_construct_kind(DDS::DISCARD);
  first_expected_md->type(expected_dt);
  second_expected_md->type(expected_dt);
  expected_dt->set_descriptor(td);
  first_expected_dtm->set_descriptor(first_expected_md);
  second_expected_dtm->set_descriptor(second_expected_md);
  expected_dt->insert_dynamic_member(first_expected_dtm);
  expected_dt->insert_dynamic_member(second_expected_dtm);
  test_conversion<DCPS::MyModCompleteToDynamic_PrimitiveKind_xtag>(expected_dt);
  // Enum types are circular.  Break apart.
  expected_dt_var->clear();
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl, CompleteToDynamicType_MyUnion)
{
  XTypes::DynamicTypeImpl* expected_union_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var expected_union_dt_var = expected_union_dt;
  DDS::TypeDescriptor_var td = new XTypes::TypeDescriptorImpl();
  td->kind(XTypes::TK_UNION);
  td->name("::MyModCompleteToDynamic::MyUnion");
  td->bound().length(0);
  td->extensibility_kind(DDS::APPENDABLE);
  td->is_nested(0);
  XTypes::DynamicTypeMemberImpl* long_expected_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var long_expected_dtm_var = long_expected_dtm;
  DDS::MemberDescriptor_var long_expected_md = new XTypes::MemberDescriptorImpl();
  //long_expected_dtm->set_parent(expected_union_dt);
  long_expected_md->name("l");
  long_expected_md->id(0);
  long_expected_md->index(0);
  long_expected_md->try_construct_kind(DDS::DISCARD);
  long_expected_md->label().length(1);
  long_expected_md->label()[0] = 0;
  long_expected_md->is_default_label(0);
  XTypes::DynamicTypeMemberImpl* char_expected_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var char_expected_dtm_var = char_expected_dtm;
  DDS::MemberDescriptor_var char_expected_md = new XTypes::MemberDescriptorImpl();
  //char_expected_dtm->set_parent(expected_union_dt);
  char_expected_md->name("c");
  char_expected_md->id(1);
  char_expected_md->index(1);
  char_expected_md->try_construct_kind(DDS::DISCARD);
  char_expected_md->label().length(1);
  char_expected_md->label()[0] = 1;
  char_expected_md->is_default_label(0);
  XTypes::DynamicTypeMemberImpl* short_expected_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var short_expected_dtm_var = short_expected_dtm;
  DDS::MemberDescriptor_var short_expected_md = new XTypes::MemberDescriptorImpl();
  //short_expected_dtm->set_parent(expected_union_dt);
  short_expected_md->name("s");
  short_expected_md->id(2);
  short_expected_md->index(2);
  short_expected_md->try_construct_kind(DDS::DISCARD);
  short_expected_md->is_default_label(1);

  XTypes::DynamicTypeImpl* long_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var long_expected_dt_var = long_expected_dt;
  DDS::TypeDescriptor_var long_td = new XTypes::TypeDescriptorImpl();
  long_td->kind(XTypes::TK_INT32);
  long_td->bound().length(0);
  long_td->extensibility_kind(DDS::FINAL);
  long_td->name("Int32");
  long_expected_md->type(long_expected_dt);
  XTypes::DynamicTypeImpl* char_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var char_expected_dt_var = char_expected_dt;
  DDS::TypeDescriptor_var char_td = new XTypes::TypeDescriptorImpl();
  char_td->kind(XTypes::TK_CHAR8);
  char_td->bound().length(0);
  char_td->extensibility_kind(DDS::FINAL);
  char_td->name("Char8");
  char_expected_md->type(char_expected_dt);
  XTypes::DynamicTypeImpl* short_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var short_expected_dt_var = short_expected_dt;
  DDS::TypeDescriptor_var short_td = new XTypes::TypeDescriptorImpl();
  short_td->kind(XTypes::TK_INT16);
  short_td->bound().length(0);
  short_td->extensibility_kind(DDS::FINAL);
  short_td->name("Int16");
  short_expected_md->type(short_expected_dt);
  XTypes::DynamicTypeImpl* enum_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var enum_expected_dt_var = enum_expected_dt;
  DDS::TypeDescriptor_var enum_td = new XTypes::TypeDescriptorImpl();
  enum_td->kind(XTypes::TK_ENUM);
  enum_td->name("::MyModCompleteToDynamic::PrimitiveKind");
  enum_td->bound().length(1);
  enum_td->bound()[0] = 32;
  enum_td->extensibility_kind(DDS::FINAL);
  enum_td->is_nested(0);
  XTypes::DynamicTypeMemberImpl* first_expected_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var first_expected_dtm_var = first_expected_dtm;
  DDS::MemberDescriptor_var first_expected_md = new XTypes::MemberDescriptorImpl();
  //first_expected_dtm->set_parent(enum_expected_dt);
  first_expected_md->name("TK_INT32");
  first_expected_md->id(ACE_UINT32_MAX);
  first_expected_md->index(0);
  first_expected_md->is_default_label(1);
  first_expected_md->try_construct_kind(DDS::DISCARD);
  XTypes::DynamicTypeMemberImpl* second_expected_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var second_expected_dtm_var = second_expected_dtm;
  DDS::MemberDescriptor_var second_expected_md = new XTypes::MemberDescriptorImpl();
  //second_expected_dtm->set_parent(enum_expected_dt);
  second_expected_md->name("TK_CHAR8");
  second_expected_md->id(ACE_UINT32_MAX);
  second_expected_md->index(1);
  second_expected_md->try_construct_kind(DDS::DISCARD);
  first_expected_md->type(enum_expected_dt);
  second_expected_md->type(enum_expected_dt);
  long_expected_md->type(long_expected_dt);
  char_expected_md->type(char_expected_dt);
  short_expected_md->type(short_expected_dt);
  td->discriminator_type(enum_expected_dt);
  expected_union_dt->set_descriptor(td);
  long_expected_dtm->set_descriptor(long_expected_md);
  char_expected_dtm->set_descriptor(char_expected_md);
  short_expected_dtm->set_descriptor(short_expected_md);
  long_expected_dt->set_descriptor(long_td);
  char_expected_dt->set_descriptor(char_td);
  short_expected_dt->set_descriptor(short_td);
  enum_expected_dt->set_descriptor(enum_td);
  first_expected_dtm->set_descriptor(first_expected_md);
  second_expected_dtm->set_descriptor(second_expected_md);
  enum_expected_dt->insert_dynamic_member(first_expected_dtm);
  enum_expected_dt->insert_dynamic_member(second_expected_dtm);
  expected_union_dt->insert_dynamic_member(long_expected_dtm);
  expected_union_dt->insert_dynamic_member(char_expected_dtm);
  expected_union_dt->insert_dynamic_member(short_expected_dtm);
  test_conversion<DCPS::MyModCompleteToDynamic_MyUnion_xtag>(expected_union_dt);
  // Enum types are circular.  Break apart.
  enum_expected_dt_var->clear();
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl, CompleteToDynamicType_MyInnerArray)
{
  XTypes::DynamicTypeImpl* alias_inner_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var alias_inner_expected_dt_var = alias_inner_expected_dt;
  DDS::TypeDescriptor_var alias_inner_td = new XTypes::TypeDescriptorImpl();
  alias_inner_td->kind(XTypes::TK_ALIAS);
  alias_inner_td->name("::MyModCompleteToDynamic::MyInnerArray");
  alias_inner_td->bound().length(0);
  alias_inner_td->extensibility_kind(DDS::FINAL);
  XTypes::DynamicTypeImpl* inner_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var inner_expected_dt_var = inner_expected_dt;
  DDS::TypeDescriptor_var inner_td = new XTypes::TypeDescriptorImpl();
  inner_td->kind(XTypes::TK_ARRAY);
  inner_td->name("Array");
  inner_td->bound().length(1);
  inner_td->bound()[0] = 2;
  inner_td->extensibility_kind(DDS::FINAL);
  XTypes::DynamicTypeImpl* long_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var long_expected_dt_var = long_expected_dt;
  DDS::TypeDescriptor_var long_td = new XTypes::TypeDescriptorImpl();
  long_td->kind(XTypes::TK_INT32);
  long_td->bound().length(0);
  long_td->name("Int32");
  long_td->extensibility_kind(DDS::FINAL);
  inner_td->element_type(long_expected_dt);
  alias_inner_td->base_type(inner_expected_dt);
  alias_inner_expected_dt->set_descriptor(alias_inner_td);
  inner_expected_dt->set_descriptor(inner_td);
  long_expected_dt->set_descriptor(long_td);
  test_conversion<DCPS::MyModCompleteToDynamic_MyInnerArray_xtag>(alias_inner_expected_dt);
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl, CompleteToDynamicType_MyOuterArray)
{
  XTypes::DynamicTypeImpl* alias_outer_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var alias_outer_expected_dt_var = alias_outer_expected_dt;
  DDS::TypeDescriptor_var alias_outer_td = new XTypes::TypeDescriptorImpl();
  alias_outer_td->kind(XTypes::TK_ALIAS);
  alias_outer_td->name("::MyModCompleteToDynamic::MyOuterArray");
  alias_outer_td->bound().length(0);
  alias_outer_td->extensibility_kind(DDS::FINAL);
  XTypes::DynamicTypeImpl* outer_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var outer_expected_dt_var = outer_expected_dt;
  DDS::TypeDescriptor_var outer_td = new XTypes::TypeDescriptorImpl();
  outer_td->kind(XTypes::TK_ARRAY);
  outer_td->name("Array");
  outer_td->bound().length(2);
  outer_td->bound()[0] = 3;
  outer_td->bound()[1] = 2;
  outer_td->extensibility_kind(DDS::FINAL);
  XTypes::DynamicTypeImpl* alias_inner_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var alias_inner_expected_dt_var = alias_inner_expected_dt;
  DDS::TypeDescriptor_var alias_inner_td = new XTypes::TypeDescriptorImpl();
  alias_inner_td->kind(XTypes::TK_ALIAS);
  alias_inner_td->name("::MyModCompleteToDynamic::MyInnerArray");
  alias_inner_td->bound().length(0);
  alias_inner_td->extensibility_kind(DDS::FINAL);
  XTypes::DynamicTypeImpl* inner_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var inner_expected_dt_var = inner_expected_dt;
  DDS::TypeDescriptor_var inner_td = new XTypes::TypeDescriptorImpl();
  inner_td->kind(XTypes::TK_ARRAY);
  inner_td->name("Array");
  inner_td->bound().length(1);
  inner_td->bound()[0] = 2;
  inner_td->extensibility_kind(DDS::FINAL);
  XTypes::DynamicTypeImpl* long_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var long_expected_dt_var = long_expected_dt;
  DDS::TypeDescriptor_var long_td = new XTypes::TypeDescriptorImpl();
  long_td->kind(XTypes::TK_INT32);
  long_td->bound().length(0);
  long_td->name("Int32");
  long_td->extensibility_kind(DDS::FINAL);
  outer_td->element_type(alias_inner_expected_dt);
  alias_outer_td->base_type(outer_expected_dt);
  inner_td->element_type(long_expected_dt);
  alias_inner_td->base_type(inner_expected_dt);
  alias_outer_expected_dt->set_descriptor(alias_outer_td);
  outer_expected_dt->set_descriptor(outer_td);
  alias_inner_expected_dt->set_descriptor(alias_inner_td);
  inner_expected_dt->set_descriptor(inner_td);
  long_expected_dt->set_descriptor(long_td);
  test_conversion<DCPS::MyModCompleteToDynamic_MyOuterArray_xtag>(alias_outer_expected_dt);
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl, CompleteToDynamicType_MySeq)
{
  XTypes::DynamicTypeImpl* alias_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var alias_expected_dt_var = alias_expected_dt;
  DDS::TypeDescriptor_var alias_td = new XTypes::TypeDescriptorImpl();
  alias_td->kind(XTypes::TK_ALIAS);
  alias_td->name("::MyModCompleteToDynamic::MySeq");
  alias_td->bound().length(0);
  alias_td->extensibility_kind(DDS::FINAL);
  XTypes::DynamicTypeImpl* expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var expected_dt_var = expected_dt;
  DDS::TypeDescriptor_var td = new XTypes::TypeDescriptorImpl();
  td->kind(XTypes::TK_SEQUENCE);
  td->name("Sequence");
  td->bound().length(1);
  td->bound()[0] = 0;
  td->extensibility_kind(DDS::FINAL);
  XTypes::DynamicTypeImpl* long_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var long_expected_dt_var = long_expected_dt;
  DDS::TypeDescriptor_var long_td = new XTypes::TypeDescriptorImpl();
  long_td->kind(XTypes::TK_INT32);
  long_td->bound().length(0);
  long_td->name("Int32");
  long_td->extensibility_kind(DDS::FINAL);
  td->element_type(long_expected_dt);
  alias_td->base_type(expected_dt);
  alias_expected_dt->set_descriptor(alias_td);
  expected_dt->set_descriptor(td);
  long_expected_dt->set_descriptor(long_td);
  test_conversion<DCPS::MyModCompleteToDynamic_MySeq_xtag>(alias_expected_dt);
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl, CompleteToDynamicType_MyAnonStruct)
{
  XTypes::DynamicTypeImpl* expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var expected_dt_var = expected_dt;
  DDS::TypeDescriptor_var td = new XTypes::TypeDescriptorImpl();
  td->kind(XTypes::TK_STRUCTURE);
  td->name("::MyModCompleteToDynamic::MyAnonStruct");
  td->bound().length(0);
  td->extensibility_kind(DDS::APPENDABLE);
  td->is_nested(0);
  XTypes::DynamicTypeMemberImpl* sequence_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var sequence_dtm_var = sequence_dtm;
  DDS::MemberDescriptor_var sequence_md = new XTypes::MemberDescriptorImpl();
  //sequence_dtm->set_parent(expected_dt);
  sequence_md->name("lseq");
  sequence_md->id(0);
  sequence_md->index(0);
  sequence_md->try_construct_kind(DDS::DISCARD);
  XTypes::DynamicTypeMemberImpl* array_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var array_dtm_var = array_dtm;
  DDS::MemberDescriptor_var array_md = new XTypes::MemberDescriptorImpl();
  //array_dtm->set_parent(expected_dt);
  array_md->name("larr");
  array_md->id(1);
  array_md->index(1);
  array_md->try_construct_kind(DDS::DISCARD);
  XTypes::DynamicTypeImpl* sequence_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var sequence_expected_dt_var = sequence_expected_dt;
  DDS::TypeDescriptor_var sequence_td = new XTypes::TypeDescriptorImpl();
  sequence_td->kind(XTypes::TK_SEQUENCE);
  sequence_td->name("Sequence");
  sequence_td->bound().length(1);
  sequence_td->bound()[0] = 5;
  sequence_td->extensibility_kind(DDS::FINAL);
  XTypes::DynamicTypeImpl* array_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var array_expected_dt_var = array_expected_dt;
  DDS::TypeDescriptor_var array_td = new XTypes::TypeDescriptorImpl();
  array_td->kind(XTypes::TK_ARRAY);
  array_td->name("Array");
  array_td->bound().length(1);
  array_td->bound()[0] = 3;
  array_td->extensibility_kind(DDS::FINAL);
  XTypes::DynamicTypeImpl* long_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var long_expected_dt_var = long_expected_dt;
  DDS::TypeDescriptor_var long_td = new XTypes::TypeDescriptorImpl();
  long_td->kind(XTypes::TK_INT32);
  long_td->bound().length(0);
  long_td->name("Int32");
  long_td->extensibility_kind(DDS::FINAL);
  sequence_td->element_type(long_expected_dt);
  array_td->element_type(long_expected_dt);
  sequence_md->type(sequence_expected_dt);
  array_md->type(array_expected_dt);
  expected_dt->set_descriptor(td);
  sequence_dtm->set_descriptor(sequence_md);
  array_dtm->set_descriptor(array_md);
  sequence_expected_dt->set_descriptor(sequence_td);
  array_expected_dt->set_descriptor(array_td);
  long_expected_dt->set_descriptor(long_td);
  expected_dt->insert_dynamic_member(sequence_dtm);
  expected_dt->insert_dynamic_member(array_dtm);
  test_conversion<DCPS::MyModCompleteToDynamic_MyAnonStruct_xtag>(expected_dt);
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl, CompleteToDynamicType_CircularStruct)
{
  XTypes::DynamicTypeImpl* struct_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var struct_expected_dt_var = struct_expected_dt;
  DDS::TypeDescriptor_var struct_td = new XTypes::TypeDescriptorImpl();
  struct_td->kind(XTypes::TK_STRUCTURE);
  struct_td->name("::MyModCompleteToDynamic::CircularStruct");
  struct_td->bound().length(0);
  struct_td->extensibility_kind(DDS::MUTABLE);
  struct_td->is_nested(0);
  XTypes::DynamicTypeMemberImpl* struct_seq_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var struct_seq_dtm_var = struct_seq_dtm;
  DDS::MemberDescriptor_var struct_seq_md = new XTypes::MemberDescriptorImpl();
  //struct_seq_dtm->set_parent(struct_expected_dt);
  struct_seq_md->name("circular_struct2_seq");
  struct_seq_md->id(0);
  struct_seq_md->index(0);
  struct_seq_md->try_construct_kind(DDS::DISCARD);
  XTypes::DynamicTypeImpl* struct_seq_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var struct_seq_dt_var = struct_seq_dt;
  DDS::TypeDescriptor_var struct_seq_td = new XTypes::TypeDescriptorImpl();
  struct_seq_td->kind(XTypes::TK_SEQUENCE);
  struct_seq_td->bound().length(1);
  struct_seq_td->bound()[0] = 0;
  struct_seq_td->name("");
  XTypes::DynamicTypeImpl* struct2_expected_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var struct2_expected_dt_var = struct2_expected_dt;
  DDS::TypeDescriptor_var struct2_td = new XTypes::TypeDescriptorImpl();
  struct2_td->kind(XTypes::TK_STRUCTURE);
  struct2_td->name("::MyModCompleteToDynamic::CircularStruct2");
  struct2_td->bound().length(0);
  struct2_td->extensibility_kind(DDS::MUTABLE);
  struct2_td->is_nested(0);
  XTypes::DynamicTypeMemberImpl* struct2_seq_dtm = new XTypes::DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var struct2_seq_dtm_var = struct2_seq_dtm;
  DDS::MemberDescriptor_var struct2_seq_md = new XTypes::MemberDescriptorImpl();
  //struct2_seq_dtm->set_parent(struct2_expected_dt);
  struct2_seq_md->name("circular_struct_seq");
  struct2_seq_md->id(0);
  struct2_seq_md->index(0);
  struct2_seq_md->try_construct_kind(DDS::DISCARD);
  XTypes::DynamicTypeImpl* struct2_seq_dt = new XTypes::DynamicTypeImpl();
  DDS::DynamicType_var struct2_seq_dt_var = struct2_seq_dt;
  DDS::TypeDescriptor_var struct2_seq_td = new XTypes::TypeDescriptorImpl();
  struct2_seq_td->kind(XTypes::TK_SEQUENCE);
  struct2_seq_td->bound().length(1);
  struct2_seq_td->bound()[0] = 0;
  struct2_seq_td->name("");

  struct2_seq_md->type(struct2_seq_dt);
  struct2_seq_td->element_type(struct_expected_dt);

  struct_seq_md->type(struct_seq_dt);
  struct_seq_td->element_type(struct2_expected_dt);
  struct_expected_dt->set_descriptor(struct_td);
  struct_seq_dtm->set_descriptor(struct_seq_md);
  struct_seq_dt->set_descriptor(struct_seq_td);
  struct2_expected_dt->set_descriptor(struct2_td);
  struct2_seq_dtm->set_descriptor(struct2_seq_md);
  struct2_seq_dt->set_descriptor(struct2_seq_td);
  struct_expected_dt->insert_dynamic_member(struct_seq_dtm);
  struct2_expected_dt->insert_dynamic_member(struct2_seq_dtm);
  test_conversion<DCPS::MyModCompleteToDynamic_CircularStruct_xtag>(struct_expected_dt);
  // Circular.  Break apart.
  struct_expected_dt_var->clear();
}

void dds_DCPS_XTypes_DynamicTypeImpl::MoreSetup()
{
  XTypes::TypeIdentifierPairSeq tid_pairs;

  XTypes::TypeIdentifierPair my_inner_struct_tids;
  my_inner_struct_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToDynamic_MyInnerStruct_xtag>();
  my_inner_struct_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToDynamic_MyInnerStruct_xtag>();
  tid_pairs.append(my_inner_struct_tids);

  XTypes::TypeIdentifierPair my_outer_struct_tids;
  my_outer_struct_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToDynamic_MyOuterStruct_xtag>();
  my_outer_struct_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToDynamic_MyOuterStruct_xtag>();
  tid_pairs.append(my_outer_struct_tids);

  XTypes::TypeIdentifierPair my_alias_struct_tids;
  my_alias_struct_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToDynamic_MyAliasStruct_xtag>();
  my_alias_struct_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToDynamic_MyAliasStruct_xtag>();
  tid_pairs.append(my_alias_struct_tids);

  XTypes::TypeIdentifierPair enum_tids;
  enum_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToDynamic_PrimitiveKind_xtag>();
  enum_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToDynamic_PrimitiveKind_xtag>();
  tid_pairs.append(enum_tids);

  XTypes::TypeIdentifierPair union_tids;
  union_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToDynamic_MyUnion_xtag>();
  union_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToDynamic_MyUnion_xtag>();
  tid_pairs.append(union_tids);

  XTypes::TypeIdentifierPair inner_array_tids;
  inner_array_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToDynamic_MyInnerArray_xtag>();
  inner_array_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToDynamic_MyInnerArray_xtag>();
  tid_pairs.append(inner_array_tids);

  XTypes::TypeIdentifierPair outer_array_tids;
  outer_array_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToDynamic_MyOuterArray_xtag>();
  outer_array_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToDynamic_MyOuterArray_xtag>();
  tid_pairs.append(outer_array_tids);

  XTypes::TypeIdentifierPair sequence_tids;
  sequence_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToDynamic_MySeq_xtag>();
  sequence_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToDynamic_MySeq_xtag>();
  tid_pairs.append(sequence_tids);

  XTypes::TypeIdentifierPair anon_struct_tids;
  anon_struct_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToDynamic_MyAnonStruct_xtag>();
  anon_struct_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToDynamic_MyAnonStruct_xtag>();
  tid_pairs.append(anon_struct_tids);

  tls_->update_type_identifier_map(tid_pairs);

  MyModCompleteToDynamic::MyInnerStructTypeSupportImpl inner_typesupport;
  MyModCompleteToDynamic::MyOuterStructTypeSupportImpl outer_typesupport;
  MyModCompleteToDynamic::MyUnionTypeSupportImpl union_typesupport;
  MyModCompleteToDynamic::MyAnonStructTypeSupportImpl anon_typesupport;
  inner_typesupport.add_types(tls_);
  outer_typesupport.add_types(tls_);
  union_typesupport.add_types(tls_);
  anon_typesupport.add_types(tls_);
}

// -- starting here, used to be in CompleteToMinimalTypeObject

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

bool compare_member_hash(const NameHash& lhs, const NameHash& rhs)
{
  return std::memcmp(lhs, rhs, sizeof(NameHash)) == 0;
}

template <typename T>
bool operator==(const Sequence<T>& lhs, const Sequence<T>& rhs)
{
  if (lhs.length() != lhs.length()) {
    return false;
  }

  for (ACE_CDR::ULong i = 0; i < lhs.length(); ++i) {
    if (!(lhs[i] == rhs[i])) {
      return false;
    }
  }
  return true;
}

bool operator==(const MinimalMemberDetail& lhs, const MinimalMemberDetail& rhs)
{
  return compare_member_hash(lhs.name_hash, rhs.name_hash);
}

bool operator==(const CommonStructMember& lhs, const CommonStructMember& rhs)
{
  return lhs.member_id == rhs.member_id &&
    lhs.member_flags == rhs.member_flags &&
    lhs.member_type_id == rhs.member_type_id;
}

bool operator==(const MinimalStructMember& lhs, const MinimalStructMember& rhs)
{
  return lhs.common == rhs.common &&
    compare_member_hash(lhs.detail.name_hash, rhs.detail.name_hash);
}

bool operator==(const MinimalStructHeader& lhs, const MinimalStructHeader& rhs)
{
  return lhs.base_type == rhs.base_type;
}

bool operator==(const MinimalStructType& lhs, const MinimalStructType& rhs)
{
  return lhs.struct_flags == rhs.struct_flags &&
    lhs.header == rhs.header &&
    lhs.member_seq == rhs.member_seq;
}

bool operator==(const CommonUnionMember& lhs, const CommonUnionMember& rhs)
{
  return lhs.member_id == rhs.member_id &&
    lhs.member_flags == rhs.member_flags &&
    lhs.type_id == rhs.type_id &&
    lhs.label_seq == rhs.label_seq;
}

bool operator==(const MinimalUnionMember& lhs, const MinimalUnionMember& rhs)
{
  return lhs.common == rhs.common && lhs.detail == rhs.detail;
}

bool operator==(const CommonDiscriminatorMember& lhs, const CommonDiscriminatorMember& rhs)
{
  return lhs.member_flags == rhs.member_flags && lhs.type_id == rhs.type_id;
}

bool operator==(const MinimalDiscriminatorMember& lhs, const MinimalDiscriminatorMember& rhs)
{
  return lhs.common == rhs.common;
}

bool operator==(const MinimalUnionType& lhs, const MinimalUnionType& rhs)
{
  return lhs.union_flags == rhs.union_flags &&
    lhs.discriminator == rhs.discriminator &&
    lhs.member_seq == rhs.member_seq;
}

bool operator==(const CommonAnnotationParameter& lhs, const CommonAnnotationParameter& rhs)
{
  return lhs.member_flags == rhs.member_flags &&
    rhs.member_type_id == rhs.member_type_id;
}

bool operator==(const AnnotationParameterValue& lhs, const AnnotationParameterValue& rhs)
{
  if (lhs.kind != rhs.kind) {
    return false;
  }

  switch (rhs.kind) {
  case TK_BOOLEAN:
    return lhs.boolean_value == rhs.boolean_value;
  case TK_BYTE:
    return lhs.byte_value == rhs.byte_value;
  case TK_INT16:
    return lhs.int16_value == rhs.int16_value;
  case TK_UINT16:
    return lhs.uint16_value == rhs.uint16_value;
  case TK_INT32:
    return lhs.int32_value == rhs.int32_value;
  case TK_UINT32:
    return lhs.uint32_value == rhs.uint32_value;
  case TK_INT64:
    return lhs.int64_value == rhs.int64_value;
  case TK_UINT64:
    return lhs.uint64_value == rhs.uint64_value;
  case TK_FLOAT32:
    return lhs.float32_value == rhs.float32_value;
  case TK_FLOAT64:
    return lhs.float64_value == rhs.float64_value;
  case TK_FLOAT128:
    return lhs.float128_value == rhs.float128_value;
  case TK_CHAR8:
    return lhs.char_value == rhs.char_value;
  case TK_CHAR16:
    return lhs.wchar_value == rhs.wchar_value;
  case TK_ENUM:
    return lhs.enumerated_value == rhs.enumerated_value;
  case TK_STRING8:
    return lhs.string8_value == rhs.string8_value;
  case TK_STRING16:
    return lhs.string16_value == rhs.string16_value;
  default:
    return true;
  }
}

bool operator==(const MinimalAnnotationParameter& lhs, const MinimalAnnotationParameter& rhs)
{
  return lhs.common == rhs.common &&
    compare_member_hash(lhs.name_hash, rhs.name_hash) &&
    lhs.default_value == rhs.default_value;
}

bool operator==(const MinimalAnnotationType& lhs, const MinimalAnnotationType& rhs)
{
  return lhs.member_seq == rhs.member_seq;
}

bool operator==(const CommonAliasBody& lhs, const CommonAliasBody& rhs)
{
  return lhs.related_flags == rhs.related_flags &&
    lhs.related_type == rhs.related_type;
}

bool operator==(const MinimalAliasBody& lhs, const MinimalAliasBody& rhs) {
  return lhs.common == rhs.common;
}

bool operator==(const MinimalAliasType& lhs, const MinimalAliasType& rhs)
{
  return lhs.alias_flags == rhs.alias_flags && lhs.body == rhs.body;
}

bool operator==(const CommonCollectionElement& lhs, const CommonCollectionElement& rhs)
{
  return lhs.element_flags == rhs.element_flags && lhs.type == rhs.type;
}

bool operator==(const MinimalCollectionElement& lhs, const MinimalCollectionElement& rhs)
{
  return lhs.common == rhs.common;
}

bool operator==(const CommonCollectionHeader& lhs, const CommonCollectionHeader& rhs)
{
  return lhs.bound == rhs.bound;
}

bool operator==(const MinimalCollectionHeader& lhs, const MinimalCollectionHeader& rhs)
{
  return lhs.common == rhs.common;
}

bool operator==(const MinimalSequenceType& lhs, const MinimalSequenceType& rhs)
{
  return lhs.collection_flag == rhs.collection_flag &&
    lhs.header == rhs.header &&
    lhs.element == rhs.element;
}

bool operator==(const CommonArrayHeader& lhs, const CommonArrayHeader& rhs)
{
  return lhs.bound_seq == rhs.bound_seq;
}

bool operator==(const MinimalArrayHeader& lhs, const MinimalArrayHeader& rhs)
{
  return lhs.common == rhs.common;
}

bool operator==(const MinimalArrayType& lhs, const MinimalArrayType& rhs)
{
  return lhs.header == rhs.header && lhs.element == rhs.element;
}

bool operator==(const MinimalMapType& lhs, const MinimalMapType& rhs)
{
  return lhs.collection_flag == rhs.collection_flag &&
    lhs.header == rhs.header &&
    lhs.key == rhs.key &&
    lhs. element == rhs.element;
}

bool operator==(const CommonEnumeratedLiteral& lhs, const CommonEnumeratedLiteral& rhs)
{
  return lhs.value == rhs.value && lhs.flags == rhs.flags;
}

bool operator==(const MinimalEnumeratedLiteral& lhs, const MinimalEnumeratedLiteral& rhs)
{
  return lhs.common == rhs.common && lhs.detail == rhs.detail;
}

bool operator==(const CommonEnumeratedHeader& lhs, const CommonEnumeratedHeader& rhs)
{
  return lhs.bit_bound == rhs.bit_bound;
}

bool operator==(const MinimalEnumeratedHeader& lhs, const MinimalEnumeratedHeader& rhs)
{
  return lhs.common == rhs.common;
}

bool operator==(const MinimalEnumeratedType& lhs, const MinimalEnumeratedType& rhs)
{
  return lhs.header == rhs.header && lhs.literal_seq == rhs.literal_seq;
}

bool operator==(const CommonBitflag& lhs, const CommonBitflag& rhs)
{
  return lhs.position == rhs.position && lhs.flags == rhs.flags;
}

bool operator==(const MinimalBitflag& lhs, const MinimalBitflag& rhs)
{
  return lhs.common == rhs.common && lhs.detail == rhs.detail;
}

bool operator==(const MinimalBitmaskType& lhs, const MinimalBitmaskType& rhs)
{
  return lhs.header == rhs.header && lhs.flag_seq == rhs.flag_seq;
}

bool operator==(const CommonBitfield& lhs, const CommonBitfield& rhs)
{
  return lhs.position == rhs.position &&
    lhs.flags == rhs.flags &&
    lhs.bitcount == rhs.bitcount &&
    lhs.holder_type == rhs.holder_type;
}

bool operator==(const MinimalBitfield& lhs, const MinimalBitfield& rhs)
{
  return lhs.common == rhs.common &&
    compare_member_hash(lhs.name_hash, rhs.name_hash);
}

bool operator==(const MinimalBitsetType& lhs, const MinimalBitsetType& rhs)
{
  return lhs.field_seq == rhs.field_seq;
}

bool operator==(const MinimalTypeObject& lhs, const MinimalTypeObject& rhs)
{
  if (lhs.kind != rhs.kind) {
    return false;
  }

  switch (lhs.kind) {
  case TK_ALIAS:
    return lhs.alias_type == rhs.alias_type;
  case TK_ANNOTATION:
    return lhs.annotation_type == rhs.annotation_type;
  case TK_STRUCTURE:
    return lhs.struct_type == rhs.struct_type;
  case TK_UNION:
    return lhs.union_type == rhs.union_type;
  case TK_BITSET:
    return lhs.bitset_type == rhs.bitset_type;
  case TK_SEQUENCE:
    return lhs.sequence_type == rhs.sequence_type;
  case TK_ARRAY:
    return lhs.array_type == rhs.array_type;
  case TK_MAP:
    return lhs.map_type == rhs.map_type;
  case TK_ENUM:
    return lhs.enumerated_type == rhs.enumerated_type;
  case TK_BITMASK:
    return lhs.bitmask_type == rhs.bitmask_type;
  }
  return false;
}

bool operator==(const TypeObject& lhs, const TypeObject& rhs)
{
  if (lhs.kind != rhs.kind) {
    return false;
  }

  if (lhs.kind == EK_MINIMAL) {
    return lhs.minimal == rhs.minimal;
  }
  return true;
}
} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

class dds_DCPS_XTypes_DynamicTypeImpl_CompleteToMinimal : public ::testing::Test {
  void SetUp()
  {
    tls_ = DCPS::make_rch<XTypes::TypeLookupService>();
    MoreSetup();
  }

  void MoreSetup();

  XTypes::TypeLookupService_rch tls_;

public:

  template<typename T>
  void test_conversion()
  {
    const XTypes::TypeIdentifier& min_ti = DCPS::getMinimalTypeIdentifier<T>();
    const XTypes::TypeMap& min_map = DCPS::getMinimalTypeMap<T>();
    XTypes::TypeMap::const_iterator pos = min_map.find(min_ti);
    EXPECT_TRUE(pos != min_map.end());
    const XTypes::TypeObject& expected_min_to = pos->second;

    const XTypes::TypeIdentifier& com_ti = DCPS::getCompleteTypeIdentifier<T>();
    const XTypes::TypeMap& com_map = DCPS::getCompleteTypeMap<T>();
    pos = com_map.find(com_ti);
    EXPECT_TRUE(pos != com_map.end());
    const XTypes::TypeObject& com_to = pos->second;

    XTypes::TypeObject converted_min_to;
    EXPECT_TRUE(tls_->complete_to_minimal_type_object(com_to, converted_min_to));
    EXPECT_EQ(expected_min_to, converted_min_to);
  }
};


TEST_F(dds_DCPS_XTypes_DynamicTypeImpl_CompleteToMinimal, MyStruct)
{
  test_conversion<DCPS::MyModCompleteToMinimal_MyStruct_xtag>();
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl_CompleteToMinimal, MyUnion)
{
  test_conversion<DCPS::MyModCompleteToMinimal_MyUnion_xtag>();
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl_CompleteToMinimal, SCC)
{
  // TODO(sonndinh): Verify conversion of the types in the SCC including
  // CircularStruct, CircularStruct2, sequence<CircularStruct>,
  // sequence<CircularStruct2>, and CircularStruct[3].
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl_CompleteToMinimal, LSeq)
{
  test_conversion<DCPS::MyModCompleteToMinimal_LSeq_xtag>();
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl_CompleteToMinimal, LArr)
{
  test_conversion<DCPS::MyModCompleteToMinimal_LArr_xtag>();
}

TEST_F(dds_DCPS_XTypes_DynamicTypeImpl_CompleteToMinimal, MyEnum)
{
  test_conversion<DCPS::MyModCompleteToMinimal_MyEnum_xtag>();
}

void dds_DCPS_XTypes_DynamicTypeImpl_CompleteToMinimal::MoreSetup()
{
  XTypes::TypeIdentifierPairSeq tid_pairs;

  XTypes::TypeIdentifierPair my_struct_tids;
  my_struct_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToMinimal_MyStruct_xtag>();
  my_struct_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToMinimal_MyStruct_xtag>();
  tid_pairs.append(my_struct_tids);

  XTypes::TypeIdentifierPair circular_struct_tids;
  circular_struct_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToMinimal_CircularStruct_xtag>();
  circular_struct_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToMinimal_CircularStruct_xtag>();
  tid_pairs.append(circular_struct_tids);

  XTypes::TypeIdentifierPair circular_struct2_tids;
  circular_struct2_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToMinimal_CircularStruct2_xtag>();
  circular_struct2_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToMinimal_CircularStruct2_xtag>();
  tid_pairs.append(circular_struct2_tids);

  XTypes::TypeIdentifierPair my_enum_tids;
  my_enum_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToMinimal_MyEnum_xtag>();
  my_enum_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToMinimal_MyEnum_xtag>();
  tid_pairs.append(my_enum_tids);

  XTypes::TypeIdentifierPair my_union_tids;
  my_union_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToMinimal_MyUnion_xtag>();
  my_union_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToMinimal_MyUnion_xtag>();
  tid_pairs.append(my_union_tids);

  XTypes::TypeIdentifierPair lseq_tids;
  lseq_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToMinimal_LSeq_xtag>();
  lseq_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToMinimal_LSeq_xtag>();
  tid_pairs.append(lseq_tids);

  XTypes::TypeIdentifierPair larr_tids;
  larr_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyModCompleteToMinimal_LArr_xtag>();
  larr_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyModCompleteToMinimal_LArr_xtag>();
  tid_pairs.append(larr_tids);

  tls_->update_type_identifier_map(tid_pairs);
}

#endif // OPENDDS_SAFETY_PROFILE
