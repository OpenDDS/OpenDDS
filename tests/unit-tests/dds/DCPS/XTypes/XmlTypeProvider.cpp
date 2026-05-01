#if defined OPENDDS_XERCES3 && !defined OPENDDS_SAFETY_PROFILE

#include <XmlTypeProviderTypeSupportImpl.h>

#include <dds/DCPS/XTypes/DynamicTypeImpl.h>
#include <dds/DCPS/XTypes/XmlTypeProvider.h>

#include "gtest/gtest.h"

namespace {

const ACE_TCHAR* const XML_TYPE_FILE = ACE_TEXT("dds/DCPS/XTypes/XmlTypeProvider.xml");

DDS::DynamicType_var load_type(const char* name)
{
  DDS::DynamicType_var type;
  EXPECT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::load_xml_type(type, XML_TYPE_FILE, name));
  EXPECT_TRUE(type);
  return type;
}

DDS::MemberDescriptor_var member_descriptor(DDS::DynamicType_ptr type, const char* name)
{
  DDS::DynamicTypeMember_var member;
  EXPECT_EQ(DDS::RETCODE_OK, type->get_member_by_name(member, name));
  EXPECT_TRUE(member);
  DDS::MemberDescriptor_var descriptor;
  EXPECT_EQ(DDS::RETCODE_OK, member->get_descriptor(descriptor));
  EXPECT_TRUE(descriptor);
  return descriptor;
}

template <typename XTag>
void expect_complete_type_object(DDS::DynamicType_ptr type)
{
  OpenDDS::XTypes::DynamicTypeImpl* impl =
    dynamic_cast<OpenDDS::XTypes::DynamicTypeImpl*>(type);
  ASSERT_TRUE(impl);

  const OpenDDS::XTypes::TypeIdentifier& expected_ti =
    OpenDDS::DCPS::getCompleteTypeIdentifier<XTag>();
  EXPECT_TRUE(expected_ti == impl->get_complete_type_identifier());

  const OpenDDS::XTypes::TypeMap& expected_map =
    OpenDDS::DCPS::getCompleteTypeMap<XTag>();
  const OpenDDS::XTypes::TypeMap& actual_map = impl->get_complete_type_map();

  const OpenDDS::XTypes::TypeMap::const_iterator expected = expected_map.find(expected_ti);
  ASSERT_NE(expected_map.end(), expected);
  const OpenDDS::XTypes::TypeMap::const_iterator actual = actual_map.find(expected_ti);
  ASSERT_NE(actual_map.end(), actual);
  EXPECT_TRUE(expected->second == actual->second);
}

} // namespace

TEST(dds_DCPS_XTypes_XmlTypeProvider, StructIntrospectionAndTypeObject)
{
  DDS::DynamicType_var type = load_type("XmlTypeProviderTest::Sample");
  ASSERT_TRUE(type);
  EXPECT_EQ(OpenDDS::XTypes::TK_STRUCTURE, type->get_kind());
  CORBA::String_var name = type->get_name();
  EXPECT_STREQ("XmlTypeProviderTest::Sample", name.in());
  EXPECT_EQ(5u, type->get_member_count());

  DDS::MemberDescriptor_var id = member_descriptor(type, "id");
  EXPECT_EQ(10u, id->id());
  EXPECT_TRUE(id->is_key());
  EXPECT_EQ(OpenDDS::XTypes::TK_UINT32, id->type()->get_kind());

  DDS::MemberDescriptor_var names = member_descriptor(type, "names");
  EXPECT_EQ(11u, names->id());
  EXPECT_TRUE(names->is_must_understand());
  ASSERT_TRUE(names->type());
  EXPECT_EQ(OpenDDS::XTypes::TK_SEQUENCE, names->type()->get_kind());
  DDS::TypeDescriptor_var names_type;
  ASSERT_EQ(DDS::RETCODE_OK, names->type()->get_descriptor(names_type));
  ASSERT_EQ(1u, names_type->bound().length());
  EXPECT_EQ(4u, names_type->bound()[0]);
  ASSERT_TRUE(names_type->element_type());
  EXPECT_EQ(OpenDDS::XTypes::TK_STRING8, names_type->element_type()->get_kind());
  DDS::TypeDescriptor_var string_type;
  ASSERT_EQ(DDS::RETCODE_OK, names_type->element_type()->get_descriptor(string_type));
  ASSERT_EQ(1u, string_type->bound().length());
  EXPECT_EQ(8u, string_type->bound()[0]);

  DDS::MemberDescriptor_var values = member_descriptor(type, "values");
  EXPECT_EQ(14u, values->id());
  ASSERT_TRUE(values->type());
  EXPECT_EQ(OpenDDS::XTypes::TK_ARRAY, values->type()->get_kind());
  DDS::TypeDescriptor_var values_type;
  ASSERT_EQ(DDS::RETCODE_OK, values->type()->get_descriptor(values_type));
  ASSERT_EQ(1u, values_type->bound().length());
  EXPECT_EQ(3u, values_type->bound()[0]);

  expect_complete_type_object<OpenDDS::DCPS::XmlTypeProviderTest_Sample_xtag>(type);
}

TEST(dds_DCPS_XTypes_XmlTypeProvider, UnionIntrospectionAndTypeObject)
{
  DDS::DynamicType_var type = load_type("XmlTypeProviderTest::Choice");
  ASSERT_TRUE(type);
  EXPECT_EQ(OpenDDS::XTypes::TK_UNION, type->get_kind());
  CORBA::String_var name = type->get_name();
  EXPECT_STREQ("XmlTypeProviderTest::Choice", name.in());
  EXPECT_EQ(4u, type->get_member_count());

  DDS::MemberDescriptor_var a = member_descriptor(type, "a");
  ASSERT_EQ(1u, a->label().length());
  EXPECT_EQ(0, a->label()[0]);
  EXPECT_FALSE(a->is_default_label());

  DDS::MemberDescriptor_var b = member_descriptor(type, "b");
  ASSERT_EQ(1u, b->label().length());
  EXPECT_EQ(1, b->label()[0]);
  EXPECT_FALSE(b->is_default_label());
  EXPECT_EQ(OpenDDS::XTypes::TK_STRING8, b->type()->get_kind());

  DDS::MemberDescriptor_var other = member_descriptor(type, "other");
  EXPECT_TRUE(other->is_default_label());
  EXPECT_EQ(0u, other->label().length());
  EXPECT_EQ(OpenDDS::XTypes::TK_STRUCTURE, other->type()->get_kind());

  expect_complete_type_object<OpenDDS::DCPS::XmlTypeProviderTest_Choice_xtag>(type);
}

TEST(dds_DCPS_XTypes_XmlTypeProvider, RejectsMissingType)
{
  DDS::DynamicType_var type;
  EXPECT_EQ(DDS::RETCODE_BAD_PARAMETER,
            OpenDDS::XTypes::load_xml_type(type, XML_TYPE_FILE, "XmlTypeProviderTest::Missing"));
  EXPECT_FALSE(type);
}

#endif
