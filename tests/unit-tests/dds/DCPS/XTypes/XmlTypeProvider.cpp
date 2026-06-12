#if defined OPENDDS_XERCES3 && !defined OPENDDS_SAFETY_PROFILE

#include <XmlTypeProviderTypeSupportImpl.h>

#include <dds/DCPS/debug.h>
#include <dds/DCPS/XTypes/DynamicTypeImpl.h>
#include <dds/DCPS/XTypes/XmlTypeProvider.h>

#include "gtest/gtest.h"

#include <vector>

namespace {

const ACE_TCHAR* const XML_TYPE_FILE = ACE_TEXT("dds/DCPS/XTypes/XmlTypeProvider.xml");
const ACE_TCHAR* const INVALID_XML_TYPE_FILE = ACE_TEXT("dds/DCPS/XTypes/XmlTypeProviderInvalid.xml");

std::vector<DDS::DynamicType_var>& loaded_types()
{
  // Keep XML-loaded type graphs alive for the test process. Some graphs contain
  // internal DynamicType references and these tests don't cover teardown.
  static std::vector<DDS::DynamicType_var>* const types =
    new std::vector<DDS::DynamicType_var>;
  return *types;
}

DDS::DynamicType_var load_type(const char* name)
{
  DDS::DynamicType_var type;
  EXPECT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::load_xml_type(type, XML_TYPE_FILE, name));
  EXPECT_TRUE(type);
  if (type) {
    loaded_types().push_back(DDS::DynamicType::_duplicate(type));
  }
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
  OpenDDS::DCPS::LogRestore restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::None);

  DDS::DynamicType_var type;
  EXPECT_EQ(DDS::RETCODE_BAD_PARAMETER,
            OpenDDS::XTypes::load_xml_type(type, XML_TYPE_FILE, "XmlTypeProviderTest::Missing"));
  EXPECT_FALSE(type);
}

TEST(dds_DCPS_XTypes_XmlTypeProvider, NestedAttribute)
{
  // Inner has nested="true"; IS_NESTED should be set on it.
  DDS::DynamicType_var inner = load_type("XmlTypeProviderTest::Inner");
  ASSERT_TRUE(inner);
  DDS::TypeDescriptor_var inner_td;
  ASSERT_EQ(DDS::RETCODE_OK, inner->get_descriptor(inner_td));
  EXPECT_TRUE(inner_td->is_nested());

  // Sample has no nested attribute; IS_NESTED should not be set.
  DDS::DynamicType_var sample = load_type("XmlTypeProviderTest::Sample");
  ASSERT_TRUE(sample);
  DDS::TypeDescriptor_var sample_td;
  ASSERT_EQ(DDS::RETCODE_OK, sample->get_descriptor(sample_td));
  EXPECT_FALSE(sample_td->is_nested());

  // Inner loaded as root should produce the same TypeIdentifier as Inner
  // encountered as a dependency of Sample.  Both get IS_NESTED from the XML
  // attribute, not from load context.  We verify this by checking that Inner's
  // complete TypeIdentifier (from its own load) appears in Sample's complete
  // type map (which was built from a separate load that encountered Inner as a
  // dependency).  If IS_NESTED differed between load contexts the TypeIdentifiers
  // would not match and the map lookup would fail.
  OpenDDS::XTypes::DynamicTypeImpl* inner_impl =
    dynamic_cast<OpenDDS::XTypes::DynamicTypeImpl*>(inner.in());
  ASSERT_TRUE(inner_impl);
  OpenDDS::XTypes::DynamicTypeImpl* sample_impl =
    dynamic_cast<OpenDDS::XTypes::DynamicTypeImpl*>(sample.in());
  ASSERT_TRUE(sample_impl);

  const OpenDDS::XTypes::TypeIdentifier& inner_ti =
    inner_impl->get_complete_type_identifier();
  const OpenDDS::XTypes::TypeMap& sample_map =
    sample_impl->get_complete_type_map();
  EXPECT_NE(sample_map.end(), sample_map.find(inner_ti));
}

TEST(dds_DCPS_XTypes_XmlTypeProvider, RejectsNonExistentFile)
{
  OpenDDS::DCPS::LogRestore restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::None);

  DDS::DynamicType_var type;
  EXPECT_EQ(DDS::RETCODE_ERROR,
            OpenDDS::XTypes::load_xml_type(type, ACE_TEXT("nonexistent_file.xml"), "SomeType"));
  EXPECT_FALSE(type);
}

TEST(dds_DCPS_XTypes_XmlTypeProvider, BitmaskUnionIntrospection)
{
  DDS::DynamicType_var type = load_type("XmlTypeProviderTest::Flagged");
  ASSERT_TRUE(type);
  EXPECT_EQ(OpenDDS::XTypes::TK_UNION, type->get_kind());
  EXPECT_EQ(3u, type->get_member_count()); // discriminator + 2 cases

  DDS::MemberDescriptor_var x = member_descriptor(type, "x");
  ASSERT_EQ(1u, x->label().length());
  EXPECT_EQ(1, x->label()[0]); // FLAG_A = 1 << 0 = 1

  DDS::MemberDescriptor_var y = member_descriptor(type, "y");
  ASSERT_EQ(1u, y->label().length());
  EXPECT_EQ(2, y->label()[0]); // FLAG_B = 1 << 1 = 2

  // HighBitUnion exercises the boundary of the old bitmask label_values bug
  // (positions >= 31 stored the raw position number instead of 1 << position).
  DDS::DynamicType_var hb = load_type("XmlTypeProviderTest::HighBitUnion");
  ASSERT_TRUE(hb);

  DDS::MemberDescriptor_var a = member_descriptor(hb, "a");
  ASSERT_EQ(1u, a->label().length());
  EXPECT_EQ(static_cast<CORBA::Long>(1u << 30), a->label()[0]); // BIT_30 = 1 << 30

  DDS::MemberDescriptor_var b = member_descriptor(hb, "b");
  ASSERT_EQ(1u, b->label().length());
  // BIT_31 = (uint32)1 << 31 = 0x80000000; as signed Long = -2147483648.
  // Old code (value < 31 guard) stored 31 here instead.
  EXPECT_EQ(static_cast<CORBA::Long>(ACE_CDR::ULong(1) << 31), b->label()[0]);
}

TEST(dds_DCPS_XTypes_XmlTypeProvider, RejectsWideBitmaskUnionLabel)
{
  OpenDDS::DCPS::LogRestore restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::None);

  DDS::DynamicType_var type;
  EXPECT_EQ(DDS::RETCODE_ERROR,
            OpenDDS::XTypes::load_xml_type(
              type, INVALID_XML_TYPE_FILE, "XmlTypeProviderInvalid::WideFlagUnion"));
  EXPECT_FALSE(type);
}

#endif
