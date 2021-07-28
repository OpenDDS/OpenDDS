#include "CompleteToDynamicTypeTypeSupportImpl.h"
#include <map>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/XTypes/TypeObject.h>
#include <dds/DCPS/XTypes/DynamicType.h>
#include <dds/DCPS/XTypes/DynamicTypeMember.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/TopicDescriptionImpl.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>

#include <gtest/gtest.h>

using namespace OpenDDS;

XTypes::TypeLookupService tls;

template<typename T>
void test_conversion(const XTypes::DynamicType_rch& expected_dynamic_type)
{
  const XTypes::TypeIdentifier& com_ti = DCPS::getCompleteTypeIdentifier<T>();
  const XTypes::TypeMap& com_map = DCPS::getCompleteTypeMap<T>();
  XTypes::TypeMap::const_iterator pos = com_map.find(com_ti);
  EXPECT_TRUE(pos != com_map.end());
  const XTypes::TypeObject& com_to = pos->second;
  XTypes::DynamicType_rch converted_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  tls.complete_to_dynamic(converted_dt, com_to.complete);
  EXPECT_EQ(*expected_dynamic_type.in(), *converted_dt.in());
}

// TEST(CompleteToDynamicType, MyInnerStruct)
// {
//   XTypes::DynamicType_rch expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
//   XTypes::TypeDescriptor* td(new XTypes::TypeDescriptor);
//   td->kind = XTypes::TK_STRUCTURE;
//   td->name = "::MyMod::MyInnerStruct";
//   td->bound.length(0);
//   td->extensibility_kind = XTypes::MUTABLE;
//   td->is_nested = 0;
//   expected_dt->descriptor_ = td;
//   XTypes::DynamicTypeMember_rch expected_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
//   XTypes::MemberDescriptor* md(new XTypes::MemberDescriptor);
//   expected_dtm->descriptor_ = md;
//   md->name = "l";
//   md->id = 0;
//   md->index = 0;
//   md->try_construct_kind = XTypes::DISCARD;
//   XTypes::DynamicType_rch long_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
//   XTypes::TypeDescriptor* long_td(new XTypes::TypeDescriptor);
//   long_td->kind = XTypes::TK_INT32;
//   long_td->bound.length(0);
//   long_td->name = "long";
//   long_expected_dt->descriptor_ = long_td;
//   md->type = long_expected_dt;
//   expected_dt->member_by_index.insert(expected_dt->member_by_index.end(), expected_dtm);
//   expected_dt->member_by_id.insert(expected_dt->member_by_id.end(), std::make_pair(md->id , expected_dtm));
//   expected_dt->member_by_name.insert(expected_dt->member_by_name.end(), std::make_pair(md->name , expected_dtm));
//   test_conversion<DCPS::MyMod_MyInnerStruct_xtag>(expected_dt);
// }

TEST(CompleteToDynamicType, MyOuterStruct)
{
  XTypes::DynamicType_rch expected_outer_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* outer_td(new XTypes::TypeDescriptor);
  outer_td->kind = XTypes::TK_STRUCTURE;
  outer_td->name = "::MyMod::MyOuterStruct";
  outer_td->bound.length(0);
  outer_td->extensibility_kind = XTypes::APPENDABLE;
  outer_td->is_nested = 0;
  expected_outer_dt->descriptor_ = outer_td;
  XTypes::DynamicTypeMember_rch expected_outer_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* outer_md(new XTypes::MemberDescriptor);
  expected_outer_dtm->descriptor_ = outer_md;
  outer_md->name = "ms";
  outer_md->id = 0;
  outer_md->index = 0;
  outer_md->try_construct_kind = XTypes::DISCARD;
  XTypes::DynamicType_rch expected_inner_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* inner_td(new XTypes::TypeDescriptor);
  inner_td->kind = XTypes::TK_STRUCTURE;
  inner_td->name = "::MyMod::MyInnerStruct";
  inner_td->bound.length(0);
  inner_td->extensibility_kind = XTypes::MUTABLE;
  inner_td->is_nested = 0;
  expected_inner_dt->descriptor_ = inner_td;
  XTypes::DynamicTypeMember_rch expected_inner_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* inner_md(new XTypes::MemberDescriptor);
  expected_inner_dtm->descriptor_ = inner_md;
  inner_md->name = "l";
  inner_md->id = 0;
  inner_md->index = 0;
  inner_md->try_construct_kind = XTypes::DISCARD;
  XTypes::DynamicType_rch long_expected_inner_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* long_td(new XTypes::TypeDescriptor);
  long_td->kind = XTypes::TK_INT32;
  long_td->bound.length(0);
  long_td->name = "long";
  long_expected_inner_dt->descriptor_ = long_td;
  inner_md->type = long_expected_inner_dt;
  outer_md->type = expected_inner_dt;
  expected_inner_dt->member_by_index.insert(expected_inner_dt->member_by_index.end(), expected_inner_dtm);
  expected_inner_dt->member_by_id.insert(expected_inner_dt->member_by_id.end(), std::make_pair(inner_md->id , expected_inner_dtm));
  expected_inner_dt->member_by_name.insert(expected_inner_dt->member_by_name.end(), std::make_pair(inner_md->name , expected_inner_dtm));
  expected_outer_dt->member_by_index.insert(expected_outer_dt->member_by_index.end(), expected_outer_dtm);
  expected_outer_dt->member_by_id.insert(expected_outer_dt->member_by_id.end(), std::make_pair(outer_md->id , expected_outer_dtm));
  expected_outer_dt->member_by_name.insert(expected_outer_dt->member_by_name.end(), std::make_pair(outer_md->name , expected_outer_dtm));
  test_conversion<DCPS::MyMod_MyOuterStruct_xtag>(expected_outer_dt);
}

TEST(CompleteToDynamicType, MyAliasStruct)
{
  XTypes::DynamicType_rch expected_alias_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* alias_td(new XTypes::TypeDescriptor);
  alias_td->kind = XTypes::TK_ALIAS;
  alias_td->name = "::MyMod::MyAliasStruct";
  alias_td->bound.length(0);
  alias_td->extensibility_kind = XTypes::FINAL;
  alias_td->is_nested = 0;
  expected_alias_dt->descriptor_ = alias_td;
  XTypes::DynamicType_rch expected_outer_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* outer_td(new XTypes::TypeDescriptor);
  outer_td->kind = XTypes::TK_STRUCTURE;
  outer_td->name = "::MyMod::MyOuterStruct";
  outer_td->bound.length(0);
  outer_td->extensibility_kind = XTypes::APPENDABLE;
  outer_td->is_nested = 0;
  expected_outer_dt->descriptor_ = outer_td;
  XTypes::DynamicTypeMember_rch expected_outer_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* outer_md(new XTypes::MemberDescriptor);
  expected_outer_dtm->descriptor_ = outer_md;
  outer_md->name = "ms";
  outer_md->id = 0;
  outer_md->index = 0;
  outer_md->try_construct_kind = XTypes::DISCARD;
  XTypes::DynamicType_rch expected_inner_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* inner_td(new XTypes::TypeDescriptor);
  inner_td->kind = XTypes::TK_STRUCTURE;
  inner_td->name = "::MyMod::MyInnerStruct";
  inner_td->bound.length(0);
  inner_td->extensibility_kind = XTypes::MUTABLE;
  inner_td->is_nested = 0;
  expected_inner_dt->descriptor_ = inner_td;
  XTypes::DynamicTypeMember_rch expected_inner_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* inner_md(new XTypes::MemberDescriptor);
  expected_inner_dtm->descriptor_ = inner_md;
  inner_md->name = "l";
  inner_md->id = 0;
  inner_md->index = 0;
  inner_md->try_construct_kind = XTypes::DISCARD;
  XTypes::DynamicType_rch long_expected_inner_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* long_td(new XTypes::TypeDescriptor);
  long_td->kind = XTypes::TK_INT32;
  long_td->bound.length(0);
  long_td->name = "long";
  long_expected_inner_dt->descriptor_ = long_td;
  inner_md->type = long_expected_inner_dt;
  outer_md->type = expected_inner_dt;
  alias_td->base_type = expected_outer_dt;
  expected_inner_dt->member_by_index.insert(expected_inner_dt->member_by_index.end(), expected_inner_dtm);
  expected_inner_dt->member_by_id.insert(expected_inner_dt->member_by_id.end(), std::make_pair(inner_md->id , expected_inner_dtm));
  expected_inner_dt->member_by_name.insert(expected_inner_dt->member_by_name.end(), std::make_pair(inner_md->name , expected_inner_dtm));
  expected_outer_dt->member_by_index.insert(expected_outer_dt->member_by_index.end(), expected_outer_dtm);
  expected_outer_dt->member_by_id.insert(expected_outer_dt->member_by_id.end(), std::make_pair(outer_md->id , expected_outer_dtm));
  expected_outer_dt->member_by_name.insert(expected_outer_dt->member_by_name.end(), std::make_pair(outer_md->name , expected_outer_dtm));
  test_conversion<DCPS::MyMod_MyAliasStruct_xtag>(expected_alias_dt);
}

TEST(CompleteToDynamicType, PrimitiveKind)
{
  XTypes::DynamicType_rch expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* td(new XTypes::TypeDescriptor);
  td->kind = XTypes::TK_ENUM;
  td->name = "::MyMod::PrimitiveKind";
  td->bound.length(0);
  td->extensibility_kind = XTypes::FINAL;
  td->is_nested = 0;
  expected_dt->descriptor_ = td;
  XTypes::DynamicTypeMember_rch first_expected_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* first_md(new XTypes::MemberDescriptor);
  first_expected_dtm->descriptor_ = first_md;
  first_md->name = "TK_INT32";
  first_md->id = 0;
  first_md->index = 0;
  first_md->is_default_label = 1;
  first_md->try_construct_kind = XTypes::DISCARD;
  XTypes::DynamicTypeMember_rch second_expected_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* second_md(new XTypes::MemberDescriptor);
  second_expected_dtm->descriptor_ = second_md;
  second_md->name = "TK_CHAR8";
  second_md->id = 0;
  second_md->index = 1;
  second_md->try_construct_kind = XTypes::DISCARD;
  first_md->type = expected_dt;
  second_md->type = expected_dt;
  expected_dt->member_by_index.insert(expected_dt->member_by_index.end(), first_expected_dtm);
  expected_dt->member_by_id.insert(expected_dt->member_by_id.end(), std::make_pair(first_md->id , first_expected_dtm));
  expected_dt->member_by_name.insert(expected_dt->member_by_name.end(), std::make_pair(first_md->name , first_expected_dtm));
  expected_dt->member_by_index.insert(expected_dt->member_by_index.end(), second_expected_dtm);
  expected_dt->member_by_id.insert(expected_dt->member_by_id.end(), std::make_pair(second_md->id , second_expected_dtm));
  expected_dt->member_by_name.insert(expected_dt->member_by_name.end(), std::make_pair(second_md->name , second_expected_dtm));
  test_conversion<DCPS::MyMod_PrimitiveKind_xtag>(expected_dt);
}

TEST(CompleteToDynamicType, MyUnion)
{
  XTypes::DynamicType_rch expected_union_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* td(new XTypes::TypeDescriptor);
  td->kind = XTypes::TK_UNION;
  td->name = "::MyMod::MyUnion";
  td->bound.length(0);
  td->extensibility_kind = XTypes::APPENDABLE;
  td->is_nested = 0;
  expected_union_dt->descriptor_ = td;
  XTypes::DynamicTypeMember_rch long_expected_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* long_md(new XTypes::MemberDescriptor);
  long_expected_dtm->descriptor_ = long_md;
  long_md->name = "l";
  long_md->id = 0;
  long_md->index = 0;
  long_md->try_construct_kind = XTypes::DISCARD;
  long_md->label.length(1);
  long_md->label[0] = 0;
  long_md->is_default_label = 0;
  XTypes::DynamicTypeMember_rch char_expected_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* char_md(new XTypes::MemberDescriptor);
  char_expected_dtm->descriptor_ = char_md;
  char_md->name = "c";
  char_md->id = 1;
  char_md->index = 1;
  char_md->try_construct_kind = XTypes::DISCARD;
  char_md->label.length(1);
  char_md->label[0] = 1;
  char_md->is_default_label = 0;
  XTypes::DynamicTypeMember_rch short_expected_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* short_md(new XTypes::MemberDescriptor);
  short_expected_dtm->descriptor_ = short_md;
  short_md->name = "s";
  short_md->id = 2;
  short_md->index = 2;
  short_md->try_construct_kind = XTypes::DISCARD;
  short_md->is_default_label = 1;
  XTypes::DynamicType_rch long_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* long_td(new XTypes::TypeDescriptor);
  long_td->kind = XTypes::TK_INT32;
  long_td->bound.length(0);
  long_td->extensibility_kind = XTypes::FINAL;
  long_td->name = "long";
  long_expected_dt->descriptor_ = long_td;
  long_md->type = long_expected_dt;
  XTypes::DynamicType_rch char_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* char_td(new XTypes::TypeDescriptor);
  char_td->kind = XTypes::TK_CHAR8;
  char_td->bound.length(0);
  char_td->extensibility_kind = XTypes::FINAL;
  char_td->name = "char";
  char_expected_dt->descriptor_ = char_td;
  char_md->type = char_expected_dt;
  XTypes::DynamicType_rch short_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* short_td(new XTypes::TypeDescriptor);
  short_td->kind = XTypes::TK_INT16;
  short_td->bound.length(0);
  short_td->extensibility_kind = XTypes::FINAL;
  short_td->name = "short";
  short_expected_dt->descriptor_ = short_td;
  short_md->type = short_expected_dt;
  XTypes::DynamicType_rch expected_enum_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* enum_td(new XTypes::TypeDescriptor);
  enum_td->kind = XTypes::TK_ENUM;
  enum_td->name = "::MyMod::PrimitiveKind";
  enum_td->bound.length(0);
  enum_td->extensibility_kind = XTypes::FINAL;
  enum_td->is_nested = 0;
  expected_enum_dt->descriptor_ = enum_td;
  XTypes::DynamicTypeMember_rch first_expected_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* first_md(new XTypes::MemberDescriptor);
  first_expected_dtm->descriptor_ = first_md;
  first_md->name = "TK_INT32";
  first_md->id = 0;
  first_md->index = 0;
  first_md->is_default_label = 1;
  first_md->try_construct_kind = XTypes::DISCARD;
  XTypes::DynamicTypeMember_rch second_expected_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* second_md(new XTypes::MemberDescriptor);
  second_expected_dtm->descriptor_ = second_md;
  second_md->name = "TK_CHAR8";
  second_md->id = 0;
  second_md->index = 1;
  second_md->try_construct_kind = XTypes::DISCARD;
  first_md->type = expected_enum_dt;
  second_md->type = expected_enum_dt;
  long_md->type = long_expected_dt;
  char_md->type = char_expected_dt;
  short_md->type = short_expected_dt;
  expected_union_dt->descriptor_->discriminator_type = expected_enum_dt;
  expected_enum_dt->member_by_index.insert(expected_enum_dt->member_by_index.end(), first_expected_dtm);
  expected_enum_dt->member_by_id.insert(expected_enum_dt->member_by_id.end(), std::make_pair(first_md->id , first_expected_dtm));
  expected_enum_dt->member_by_name.insert(expected_enum_dt->member_by_name.end(), std::make_pair(first_md->name , first_expected_dtm));
  expected_enum_dt->member_by_index.insert(expected_enum_dt->member_by_index.end(), second_expected_dtm);
  expected_enum_dt->member_by_id.insert(expected_enum_dt->member_by_id.end(), std::make_pair(second_md->id , second_expected_dtm));
  expected_enum_dt->member_by_name.insert(expected_enum_dt->member_by_name.end(), std::make_pair(second_md->name , second_expected_dtm));
  expected_union_dt->member_by_index.insert(expected_union_dt->member_by_index.end(), long_expected_dtm);
  expected_union_dt->member_by_id.insert(expected_union_dt->member_by_id.end(), std::make_pair(long_md->id , long_expected_dtm));
  expected_union_dt->member_by_name.insert(expected_union_dt->member_by_name.end(), std::make_pair(long_md->name , long_expected_dtm));
  expected_union_dt->member_by_index.insert(expected_union_dt->member_by_index.end(), char_expected_dtm);
  expected_union_dt->member_by_id.insert(expected_union_dt->member_by_id.end(), std::make_pair(char_md->id , char_expected_dtm));
  expected_union_dt->member_by_name.insert(expected_union_dt->member_by_name.end(), std::make_pair(char_md->name , char_expected_dtm));
  expected_union_dt->member_by_index.insert(expected_union_dt->member_by_index.end(), short_expected_dtm);
  expected_union_dt->member_by_id.insert(expected_union_dt->member_by_id.end(), std::make_pair(short_md->id , short_expected_dtm));
  expected_union_dt->member_by_name.insert(expected_union_dt->member_by_name.end(), std::make_pair(short_md->name , short_expected_dtm));
  test_conversion<DCPS::MyMod_MyUnion_xtag>(expected_union_dt);
}

TEST(CompleteToDynamicType, MyInnerArray)
{
  XTypes::DynamicType_rch alias_inner_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* alias_inner_td(new XTypes::TypeDescriptor);
  alias_inner_td->kind = XTypes::TK_ALIAS;
  alias_inner_td->name = "::MyMod::MyInnerArray";
  alias_inner_td->bound.length(0);
  alias_inner_td->extensibility_kind = XTypes::FINAL;
  alias_inner_expected_dt->descriptor_ = alias_inner_td;
  XTypes::DynamicType_rch inner_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* inner_td(new XTypes::TypeDescriptor);
  inner_td->kind = XTypes::TK_ARRAY;
  inner_td->name = "plain array";
  inner_td->bound.length(1);
  inner_td->bound[0] = 2;
  inner_td->extensibility_kind = XTypes::FINAL;
  inner_expected_dt->descriptor_ = inner_td;
  XTypes::DynamicType_rch long_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* long_td(new XTypes::TypeDescriptor);
  long_td->kind = XTypes::TK_INT32;
  long_td->bound.length(0);
  long_td->name = "long";
  long_td->extensibility_kind = XTypes::FINAL;
  long_expected_dt->descriptor_ = long_td;
  inner_td->element_type = long_expected_dt;
  alias_inner_td->base_type = inner_expected_dt;
  test_conversion<DCPS::MyMod_MyInnerArray_xtag>(alias_inner_expected_dt);
}

TEST(CompleteToDynamicType, MyOuterArray)
{
  XTypes::DynamicType_rch alias_outer_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* alias_outer_td(new XTypes::TypeDescriptor);
  alias_outer_td->kind = XTypes::TK_ALIAS;
  alias_outer_td->name = "::MyMod::MyOuterArray";
  alias_outer_td->bound.length(0);
  alias_outer_td->extensibility_kind = XTypes::FINAL;
  alias_outer_expected_dt->descriptor_ = alias_outer_td;
  XTypes::DynamicType_rch outer_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* outer_td(new XTypes::TypeDescriptor);
  outer_td->kind = XTypes::TK_ARRAY;
  outer_td->name = "plain array";
  outer_td->bound.length(2);
  outer_td->bound[0] = 3;
  outer_td->bound[1] = 2;
  outer_td->extensibility_kind = XTypes::FINAL;
  outer_expected_dt->descriptor_ = outer_td;
  XTypes::DynamicType_rch alias_inner_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* alias_inner_td(new XTypes::TypeDescriptor);
  alias_inner_td->kind = XTypes::TK_ALIAS;
  alias_inner_td->name = "::MyMod::MyInnerArray";
  alias_inner_td->bound.length(0);
  alias_inner_td->extensibility_kind = XTypes::FINAL;
  alias_inner_expected_dt->descriptor_ = alias_inner_td;
  XTypes::DynamicType_rch inner_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* inner_td(new XTypes::TypeDescriptor);
  inner_td->kind = XTypes::TK_ARRAY;
  inner_td->name = "plain array";
  inner_td->bound.length(1);
  inner_td->bound[0] = 2;
  inner_td->extensibility_kind = XTypes::FINAL;
  inner_expected_dt->descriptor_ = inner_td;
  XTypes::DynamicType_rch long_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* long_td(new XTypes::TypeDescriptor);
  long_td->kind = XTypes::TK_INT32;
  long_td->bound.length(0);
  long_td->name = "long";
  long_td->extensibility_kind = XTypes::FINAL;
  long_expected_dt->descriptor_ = long_td;

  outer_td->element_type = alias_inner_expected_dt;
  alias_outer_td->base_type = outer_expected_dt;
  inner_td->element_type = long_expected_dt;
  alias_inner_td->base_type = inner_expected_dt;
  test_conversion<DCPS::MyMod_MyOuterArray_xtag>(alias_outer_expected_dt);
}

TEST(CompleteToDynamicType, MySeq)
{
  XTypes::DynamicType_rch alias_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* alias_td(new XTypes::TypeDescriptor);
  alias_td->kind = XTypes::TK_ALIAS;
  alias_td->name = "::MyMod::MySeq";
  alias_td->bound.length(0);
  alias_td->extensibility_kind = XTypes::FINAL;
  alias_expected_dt->descriptor_ = alias_td;
  XTypes::DynamicType_rch expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* td(new XTypes::TypeDescriptor);
  td->kind = XTypes::TK_SEQUENCE;
  td->name = "plain sequence";
  td->bound.length(1);
  td->bound[0] = UINT32_MAX;
  td->extensibility_kind = XTypes::FINAL;
  expected_dt->descriptor_ = td;
  XTypes::DynamicType_rch long_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* long_td(new XTypes::TypeDescriptor);
  long_td->kind = XTypes::TK_INT32;
  long_td->bound.length(0);
  long_td->name = "long";
  long_td->extensibility_kind = XTypes::FINAL;
  long_expected_dt->descriptor_ = long_td;
  td->element_type = long_expected_dt;
  alias_td->base_type = expected_dt;
  test_conversion<DCPS::MyMod_MySeq_xtag>(alias_expected_dt);
}

TEST(CompleteToDynamicType, MyAnonStruct)
{
  XTypes::DynamicType_rch expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* td(new XTypes::TypeDescriptor);
  td->kind = XTypes::TK_STRUCTURE;
  td->name = "::MyMod::MyAnonStruct";
  td->bound.length(0);
  td->extensibility_kind = XTypes::APPENDABLE;
  td->is_nested = 0;
  expected_dt->descriptor_ = td;
  XTypes::DynamicTypeMember_rch sequence_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* sequence_md(new XTypes::MemberDescriptor);
  sequence_dtm->descriptor_ = sequence_md;
  sequence_md->name = "lseq";
  sequence_md->id = 0;
  sequence_md->index = 0;
  sequence_md->try_construct_kind = XTypes::DISCARD;
  XTypes::DynamicTypeMember_rch array_dtm(new XTypes::DynamicTypeMember, OpenDDS::DCPS::keep_count());
  XTypes::MemberDescriptor* array_md(new XTypes::MemberDescriptor);
  array_dtm->descriptor_ = array_md;
  array_md->name = "larr";
  array_md->id = 1;
  array_md->index = 1;
  array_md->try_construct_kind = XTypes::DISCARD;
  XTypes::DynamicType_rch sequence_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* sequence_td(new XTypes::TypeDescriptor);
  sequence_td->kind = XTypes::TK_SEQUENCE;
  sequence_td->name = "plain sequence";
  sequence_td->bound.length(1);
  sequence_td->bound[0] = 5;
  sequence_td->extensibility_kind = XTypes::FINAL;
  sequence_expected_dt->descriptor_ = sequence_td;
  XTypes::DynamicType_rch array_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* array_td(new XTypes::TypeDescriptor);
  array_td->kind = XTypes::TK_ARRAY;
  array_td->name = "plain array";
  array_td->bound.length(1);
  array_td->bound[0] = 3;
  array_td->extensibility_kind = XTypes::FINAL;
  array_expected_dt->descriptor_ = array_td;
  XTypes::DynamicType_rch long_expected_dt(new XTypes::DynamicType, OpenDDS::DCPS::keep_count());
  XTypes::TypeDescriptor* long_td(new XTypes::TypeDescriptor);
  long_td->kind = XTypes::TK_INT32;
  long_td->bound.length(0);
  long_td->name = "long";
  long_td->extensibility_kind = XTypes::FINAL;
  long_expected_dt->descriptor_ = long_td;
  sequence_td->element_type = long_expected_dt;
  array_td->element_type = long_expected_dt;
  sequence_md->type = sequence_expected_dt;
  array_md->type = array_expected_dt;
  expected_dt->member_by_index.insert(expected_dt->member_by_index.end(), sequence_dtm);
  expected_dt->member_by_id.insert(expected_dt->member_by_id.end(), std::make_pair(sequence_md->id , sequence_dtm));
  expected_dt->member_by_name.insert(expected_dt->member_by_name.end(), std::make_pair(sequence_md->name , sequence_dtm));
  expected_dt->member_by_index.insert(expected_dt->member_by_index.end(), array_dtm);
  expected_dt->member_by_id.insert(expected_dt->member_by_id.end(), std::make_pair(array_md->id , array_dtm));
  expected_dt->member_by_name.insert(expected_dt->member_by_name.end(), std::make_pair(array_md->name , array_dtm));
  test_conversion<DCPS::MyMod_MyAnonStruct_xtag>(expected_dt);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  XTypes::TypeIdentifierPairSeq tid_pairs;

  XTypes::TypeIdentifierPair my_inner_struct_tids;
  my_inner_struct_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_MyInnerStruct_xtag>();
  my_inner_struct_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_MyInnerStruct_xtag>();
  tid_pairs.append(my_inner_struct_tids);

  XTypes::TypeIdentifierPair my_outer_struct_tids;
  my_outer_struct_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_MyOuterStruct_xtag>();
  my_outer_struct_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_MyOuterStruct_xtag>();
  tid_pairs.append(my_outer_struct_tids);

  XTypes::TypeIdentifierPair my_alias_struct_tids;
  my_alias_struct_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_MyAliasStruct_xtag>();
  my_alias_struct_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_MyAliasStruct_xtag>();
  tid_pairs.append(my_alias_struct_tids);

  XTypes::TypeIdentifierPair enum_tids;
  enum_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_PrimitiveKind_xtag>();
  enum_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_PrimitiveKind_xtag>();
  tid_pairs.append(enum_tids);

  XTypes::TypeIdentifierPair union_tids;
  union_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_MyUnion_xtag>();
  union_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_MyUnion_xtag>();
  tid_pairs.append(union_tids);

  XTypes::TypeIdentifierPair inner_array_tids;
  inner_array_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_MyInnerArray_xtag>();
  inner_array_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_MyInnerArray_xtag>();
  tid_pairs.append(inner_array_tids);

  XTypes::TypeIdentifierPair outer_array_tids;
  outer_array_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_MyOuterArray_xtag>();
  outer_array_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_MyOuterArray_xtag>();
  tid_pairs.append(outer_array_tids);

  XTypes::TypeIdentifierPair sequence_tids;
  sequence_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_MySeq_xtag>();
  sequence_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_MySeq_xtag>();
  tid_pairs.append(sequence_tids);

  XTypes::TypeIdentifierPair anon_struct_tids;
  anon_struct_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_MyAnonStruct_xtag>();
  anon_struct_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_MyAnonStruct_xtag>();
  tid_pairs.append(anon_struct_tids);

  tls.update_type_identifier_map(tid_pairs);

  OpenDDS::DCPS::TypeSupportImpl* const inner_typesupport = new MyMod::MyInnerStructTypeSupportImpl;
  OpenDDS::DCPS::TypeSupportImpl* const outer_typesupport = new MyMod::MyOuterStructTypeSupportImpl;
  OpenDDS::DCPS::TypeSupportImpl* const union_typesupport = new MyMod::MyUnionTypeSupportImpl;
  OpenDDS::DCPS::TypeSupportImpl* const anon_typesupport = new MyMod::MyAnonStructTypeSupportImpl;
  tls.add_type_objects_to_cache(*inner_typesupport);
  tls.add_type_objects_to_cache(*outer_typesupport);
  tls.add_type_objects_to_cache(*union_typesupport);
  tls.add_type_objects_to_cache(*anon_typesupport);

  return RUN_ALL_TESTS();
}
