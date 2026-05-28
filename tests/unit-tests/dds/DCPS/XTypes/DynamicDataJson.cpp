#if defined OPENDDS_RAPIDJSON && defined OPENDDS_XERCES3 && !defined OPENDDS_SAFETY_PROFILE

#include <dds/DCPS/XTypes/DynamicDataFactory.h>
#include <dds/DCPS/XTypes/DynamicDataImpl.h>
#include <dds/DCPS/XTypes/DynamicDataJson.h>
#include <dds/DCPS/XTypes/DynamicDataXcdrReadImpl.h>
#include <dds/DCPS/XTypes/TypeObject.h>
#include <dds/DCPS/XTypes/Utils.h>
#include <dds/DCPS/XTypes/XmlTypeProvider.h>
#include <dds/DCPS/Serializer.h>

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

DDS::DynamicData_var create_data(const char* type_name)
{
  DDS::DynamicType_var type = load_type(type_name);
  return DDS::DynamicDataFactory::get_instance()->create_data(type);
}

} // namespace

TEST(dds_DCPS_XTypes_DynamicDataJson, StructPopulateAndRoundTrip)
{
  DDS::DynamicType_var type = load_type("XmlTypeProviderTest::Sample");
  DDS::DynamicData_var data = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_TRUE(data);

  const char* const json =
    "{"
    "\"id\":7,"
    "\"names\":[\"a\",\"b\"],"
    "\"inner_value\":{\"value\":42},"
    "\"kind_value\":\"KIND_B\","
    "\"values\":[1,2,3]"
    "}";
  EXPECT_EQ(DDS::RETCODE_OK, OpenDDS::XTypes::dynamic_data_from_json(data, json));

  DDS::MemberDescriptor_var id = member_descriptor(type, "id");
  DDS::UInt32 id_value = 0;
  EXPECT_EQ(DDS::RETCODE_OK, data->get_uint32_value(id_value, id->id()));
  EXPECT_EQ(7u, id_value);

  DDS::MemberDescriptor_var names = member_descriptor(type, "names");
  DDS::DynamicData_var names_data;
  ASSERT_EQ(DDS::RETCODE_OK, data->get_complex_value(names_data, names->id()));
  ASSERT_EQ(2u, names_data->get_item_count());
  CORBA::String_var name0;
  ASSERT_EQ(DDS::RETCODE_OK, names_data->get_string_value(name0, 0));
  EXPECT_STREQ("a", name0.in());

  DDS::MemberDescriptor_var kind = member_descriptor(type, "kind_value");
  DDS::Int32 kind_value = -1;
  EXPECT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::get_enum_value(kind_value, kind->type(), data, kind->id()));
  EXPECT_EQ(1, kind_value);

  std::string round_trip;
  ASSERT_EQ(DDS::RETCODE_OK, OpenDDS::XTypes::dynamic_data_to_json(round_trip, data));
  DDS::DynamicData_var copy = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(copy, round_trip.c_str()));
  EXPECT_TRUE(OpenDDS::XTypes::dynamic_data_equal(data, copy));
  EXPECT_TRUE(data->equals(copy));
  EXPECT_TRUE(copy->equals(data));
}

TEST(dds_DCPS_XTypes_DynamicDataJson, UnionActiveMemberJson)
{
  DDS::DynamicType_var type = load_type("XmlTypeProviderTest::Choice");
  DDS::DynamicData_var data = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_TRUE(data);

  EXPECT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(data, "{\"b\":\"hello\"}"));

  DDS::MemberDescriptor_var b = member_descriptor(type, "b");
  CORBA::String_var value;
  ASSERT_EQ(DDS::RETCODE_OK, data->get_string_value(value, b->id()));
  EXPECT_STREQ("hello", value.in());

  DDS::Int32 discriminator = -1;
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::get_enum_value(discriminator, data, OpenDDS::XTypes::DISCRIMINATOR_ID));
  EXPECT_EQ(1, discriminator);

  std::string serialized;
  ASSERT_EQ(DDS::RETCODE_OK, OpenDDS::XTypes::dynamic_data_to_json(serialized, data));
  EXPECT_NE(std::string::npos, serialized.find("\"$discriminator\""));

  OpenDDS::XTypes::DynamicDataJsonOptions options;
  options.discriminator_format = OpenDDS::XTypes::DYNAMIC_DATA_JSON_DISCRIMINATOR_ACTIVE_MEMBER;
  serialized.clear();
  ASSERT_EQ(DDS::RETCODE_OK, OpenDDS::XTypes::dynamic_data_to_json(serialized, data, options));
  EXPECT_EQ(std::string::npos, serialized.find("\"$discriminator\""));
}

TEST(dds_DCPS_XTypes_DynamicDataJson, UnionDiscriminatorJson)
{
  DDS::DynamicType_var type = load_type("XmlTypeProviderTest::Choice");
  DDS::DynamicData_var data = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_TRUE(data);

  EXPECT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(data, "{\"$discriminator\":\"KIND_A\",\"a\":9}"));

  DDS::MemberDescriptor_var a = member_descriptor(type, "a");
  DDS::Int32 value = 0;
  ASSERT_EQ(DDS::RETCODE_OK, data->get_int32_value(value, a->id()));
  EXPECT_EQ(9, value);

  OpenDDS::XTypes::DynamicDataJsonOptions options;
  options.discriminator_format = OpenDDS::XTypes::DYNAMIC_DATA_JSON_DISCRIMINATOR_FIELD;
  std::string serialized;
  ASSERT_EQ(DDS::RETCODE_OK, OpenDDS::XTypes::dynamic_data_to_json(serialized, data, options));

  DDS::DynamicData_var copy = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(copy, serialized.c_str(), options));
  EXPECT_TRUE(OpenDDS::XTypes::dynamic_data_equal(data, copy));
  EXPECT_TRUE(data->equals(copy));
  EXPECT_TRUE(copy->equals(data));
}

TEST(dds_DCPS_XTypes_DynamicDataJson, EqualityDetectsDifference)
{
  DDS::DynamicData_var lhs = create_data("XmlTypeProviderTest::Sample");
  DDS::DynamicData_var rhs = create_data("XmlTypeProviderTest::Sample");
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(lhs, "{\"id\":7}"));
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(rhs, "{\"id\":8}"));
  EXPECT_FALSE(OpenDDS::XTypes::dynamic_data_equal(lhs, rhs));
  EXPECT_FALSE(lhs->equals(rhs));
  EXPECT_FALSE(rhs->equals(lhs));
}

TEST(dds_DCPS_XTypes_DynamicDataJson, EqualsSerializedBackingStore)
{
  DDS::DynamicType_var type = load_type("XmlTypeProviderTest::Sample");
  DDS::DynamicData_var data = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_TRUE(data);

  const char* const json =
    "{"
    "\"id\":7,"
    "\"names\":[\"a\",\"b\"],"
    "\"inner_value\":{\"value\":42},"
    "\"kind_value\":\"KIND_B\","
    "\"values\":[1,2,3]"
    "}";
  ASSERT_EQ(DDS::RETCODE_OK, OpenDDS::XTypes::dynamic_data_from_json(data, json));

  ACE_Message_Block buffer(4096);
  const OpenDDS::DCPS::Encoding encoding(
    OpenDDS::DCPS::Encoding::KIND_XCDR2, OpenDDS::DCPS::ENDIAN_BIG);
  OpenDDS::DCPS::Serializer serializer(&buffer, encoding);
  ASSERT_TRUE(serializer << data.in());

  DDS::DynamicData_var backing =
    new OpenDDS::XTypes::DynamicDataXcdrReadImpl(&buffer, encoding, type);
  DDS::DynamicData_var wrapped =
    new OpenDDS::XTypes::DynamicDataImpl(type, backing);

  EXPECT_TRUE(data->equals(backing));
  EXPECT_TRUE(backing->equals(data));
  EXPECT_TRUE(data->equals(wrapped));
  EXPECT_TRUE(wrapped->equals(data));
}

#endif
