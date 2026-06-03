#if defined OPENDDS_RAPIDJSON && defined OPENDDS_XERCES3 && !defined OPENDDS_SAFETY_PROFILE

#include <dds/DCPS/XTypes/DynamicDataBase.h>
#include <dds/DCPS/XTypes/DynamicDataFactory.h>
#include <dds/DCPS/XTypes/DynamicDataImpl.h>
#include <dds/DCPS/XTypes/DynamicDataJson.h>
#include <dds/DCPS/XTypes/DynamicDataXcdrReadImpl.h>
#include <dds/DCPS/XTypes/TypeObject.h>
#include <dds/DCPS/XTypes/Utils.h>
#include <dds/DCPS/XTypes/XmlTypeProvider.h>
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/debug.h>

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

OpenDDS::XTypes::DynamicDataBase* dynamic_data_base(DDS::DynamicData_ptr data)
{
  OpenDDS::XTypes::DynamicDataBase* base =
    dynamic_cast<OpenDDS::XTypes::DynamicDataBase*>(data);
  EXPECT_TRUE(base);
  return base;
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

TEST(dds_DCPS_XTypes_DynamicDataJson, UnionEqualityDetectsDifference)
{
  DDS::DynamicData_var lhs = create_data("XmlTypeProviderTest::Choice");
  DDS::DynamicData_var rhs = create_data("XmlTypeProviderTest::Choice");
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(lhs, "{\"b\":\"hello\"}"));
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(rhs, "{\"a\":9}"));
  EXPECT_FALSE(OpenDDS::XTypes::dynamic_data_equal(lhs, rhs));
  EXPECT_FALSE(lhs->equals(rhs));

  DDS::DynamicData_var same = create_data("XmlTypeProviderTest::Choice");
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(same, "{\"b\":\"hello\"}"));
  EXPECT_TRUE(OpenDDS::XTypes::dynamic_data_equal(lhs, same));
  EXPECT_TRUE(lhs->equals(same));
}

TEST(dds_DCPS_XTypes_DynamicDataJson, ArrayEqualityDetectsDifference)
{
  DDS::DynamicData_var lhs = create_data("XmlTypeProviderTest::Sample");
  DDS::DynamicData_var rhs = create_data("XmlTypeProviderTest::Sample");
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(lhs, "{\"values\":[1,2,3]}"));
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(rhs, "{\"values\":[1,2,4]}"));
  EXPECT_FALSE(OpenDDS::XTypes::dynamic_data_equal(lhs, rhs));
  EXPECT_FALSE(lhs->equals(rhs));

  DDS::DynamicData_var same = create_data("XmlTypeProviderTest::Sample");
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(same, "{\"values\":[1,2,3]}"));
  EXPECT_TRUE(OpenDDS::XTypes::dynamic_data_equal(lhs, same));
  EXPECT_TRUE(lhs->equals(same));
}

TEST(dds_DCPS_XTypes_DynamicDataJson, MapJsonRoundTrip)
{
  DDS::DynamicType_var type = load_type("XmlTypeProviderTest::MapSample");
  DDS::DynamicData_var data = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_TRUE(data);

  const char* const json =
    "{"
    "\"lookup\":["
    "{\"key\":7,\"value\":70},"
    "{\"key\":9,\"value\":90}"
    "],"
    "\"inner_lookup\":["
    "{\"key\":42,\"value\":{\"value\":420}}"
    "]"
    "}";
  ASSERT_EQ(DDS::RETCODE_OK, OpenDDS::XTypes::dynamic_data_from_json(data, json));

  DDS::MemberDescriptor_var lookup = member_descriptor(type, "lookup");
  DDS::DynamicData_var lookup_data;
  ASSERT_EQ(DDS::RETCODE_OK, data->get_complex_value(lookup_data, lookup->id()));
  ASSERT_EQ(2u, lookup_data->get_item_count());
  OpenDDS::XTypes::DynamicDataBase* lookup_base = dynamic_data_base(lookup_data);
  ASSERT_TRUE(lookup_base);

  DDS::DynamicData_var key;
  DDS::DynamicData_var value;
  ASSERT_EQ(DDS::RETCODE_OK, lookup_base->get_map_key(key, 0));
  ASSERT_EQ(DDS::RETCODE_OK, lookup_base->get_map_value(value, 0));
  DDS::UInt32 key_value = 0;
  ASSERT_EQ(DDS::RETCODE_OK,
            key->get_uint32_value(key_value, OpenDDS::XTypes::MEMBER_ID_INVALID));
  EXPECT_EQ(7u, key_value);
  DDS::UInt32 element_value = 0;
  ASSERT_EQ(DDS::RETCODE_OK,
            value->get_uint32_value(element_value, OpenDDS::XTypes::MEMBER_ID_INVALID));
  EXPECT_EQ(70u, element_value);

  DDS::MemberDescriptor_var inner_lookup = member_descriptor(type, "inner_lookup");
  DDS::DynamicData_var inner_lookup_data;
  ASSERT_EQ(DDS::RETCODE_OK, data->get_complex_value(inner_lookup_data, inner_lookup->id()));
  ASSERT_EQ(1u, inner_lookup_data->get_item_count());
  OpenDDS::XTypes::DynamicDataBase* inner_lookup_base = dynamic_data_base(inner_lookup_data);
  ASSERT_TRUE(inner_lookup_base);
  ASSERT_EQ(DDS::RETCODE_OK, inner_lookup_base->get_map_value(value, 0));
  DDS::DynamicType_var value_type = dynamic_data_base(value)->type();
  DDS::MemberDescriptor_var inner_value =
    member_descriptor(value_type, "value");
  DDS::Int32 inner = 0;
  ASSERT_EQ(DDS::RETCODE_OK, value->get_int32_value(inner, inner_value->id()));
  EXPECT_EQ(420, inner);

  std::string serialized;
  ASSERT_EQ(DDS::RETCODE_OK, OpenDDS::XTypes::dynamic_data_to_json(serialized, data));
  DDS::DynamicData_var copy = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(copy, serialized.c_str()));
  EXPECT_TRUE(OpenDDS::XTypes::dynamic_data_equal(data, copy));
  EXPECT_TRUE(data->equals(copy));
  EXPECT_TRUE(copy->equals(data));
}

TEST(dds_DCPS_XTypes_DynamicDataJson, MapJsonRejectsBound)
{
  DDS::DynamicData_var data = create_data("XmlTypeProviderTest::MapSample");
  ASSERT_TRUE(data);

  OpenDDS::DCPS::LogRestore log_restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::None);

  const char* const json =
    "{"
    "\"lookup\":["
    "{\"key\":1,\"value\":10},"
    "{\"key\":2,\"value\":20},"
    "{\"key\":3,\"value\":30},"
    "{\"key\":4,\"value\":40},"
    "{\"key\":5,\"value\":50}"
    "]"
    "}";
  EXPECT_EQ(DDS::RETCODE_PRECONDITION_NOT_MET,
            OpenDDS::XTypes::dynamic_data_from_json(data, json));
}

TEST(dds_DCPS_XTypes_DynamicDataJson, MapSerializedBackingStore)
{
  DDS::DynamicType_var type = load_type("XmlTypeProviderTest::MapSample");
  DDS::DynamicData_var data = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_TRUE(data);

  const char* const json =
    "{"
    "\"lookup\":["
    "{\"key\":7,\"value\":70},"
    "{\"key\":9,\"value\":90}"
    "],"
    "\"inner_lookup\":["
    "{\"key\":42,\"value\":{\"value\":420}}"
    "]"
    "}";
  ASSERT_EQ(DDS::RETCODE_OK, OpenDDS::XTypes::dynamic_data_from_json(data, json));

  ACE_Message_Block buffer(4096);
  const OpenDDS::DCPS::Encoding encoding(
    OpenDDS::DCPS::Encoding::KIND_XCDR2, OpenDDS::DCPS::ENDIAN_BIG);
  OpenDDS::DCPS::Serializer serializer(&buffer, encoding);
  ASSERT_TRUE(serializer << data.in());

  DDS::DynamicData_var backing =
    new OpenDDS::XTypes::DynamicDataXcdrReadImpl(&buffer, encoding, type);
  DDS::MemberDescriptor_var lookup = member_descriptor(type, "lookup");
  DDS::DynamicData_var lookup_data;
  ASSERT_EQ(DDS::RETCODE_OK, backing->get_complex_value(lookup_data, lookup->id()));
  ASSERT_EQ(2u, lookup_data->get_item_count());
  OpenDDS::XTypes::DynamicDataBase* lookup_base = dynamic_data_base(lookup_data);
  ASSERT_TRUE(lookup_base);

  DDS::DynamicData_var key;
  DDS::DynamicData_var value;
  ASSERT_EQ(DDS::RETCODE_OK, lookup_base->get_map_key(key, 1));
  ASSERT_EQ(DDS::RETCODE_OK, lookup_base->get_map_value(value, 1));
  DDS::UInt32 key_value = 0;
  ASSERT_EQ(DDS::RETCODE_OK,
            key->get_uint32_value(key_value, OpenDDS::XTypes::MEMBER_ID_INVALID));
  EXPECT_EQ(9u, key_value);
  DDS::UInt32 element_value = 0;
  ASSERT_EQ(DDS::RETCODE_OK,
            value->get_uint32_value(element_value, OpenDDS::XTypes::MEMBER_ID_INVALID));
  EXPECT_EQ(90u, element_value);

  DDS::DynamicData_var expected_lookup;
  ASSERT_EQ(DDS::RETCODE_OK, data->get_complex_value(expected_lookup, lookup->id()));
  EXPECT_TRUE(expected_lookup->equals(lookup_data));
  EXPECT_TRUE(lookup_data->equals(expected_lookup));
}

TEST(dds_DCPS_XTypes_DynamicDataJson, BitmaskUnionDiscriminatorJson)
{
  DDS::DynamicType_var type = load_type("XmlTypeProviderTest::Flagged");
  DDS::DynamicData_var data = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_TRUE(data);

  // Discriminator via named bitmask flag, member value by name.
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(data, "{\"$discriminator\":\"FLAG_A\",\"x\":99}"));
  DDS::MemberDescriptor_var x = member_descriptor(type, "x");
  DDS::Int32 x_val = 0;
  ASSERT_EQ(DDS::RETCODE_OK, data->get_int32_value(x_val, x->id()));
  EXPECT_EQ(99, x_val);

  // Discriminator value for FLAG_A is 1 (bit position 0); Flags has bitBound=16 (TK_UINT16).
  DDS::UInt64 disc = 0;
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::get_uint_value(disc, data, OpenDDS::XTypes::DISCRIMINATOR_ID,
                                            OpenDDS::XTypes::TK_UINT16));
  EXPECT_EQ(1u, disc);

  // Round-trip via active-member format.
  OpenDDS::XTypes::DynamicDataJsonOptions opts;
  opts.discriminator_format = OpenDDS::XTypes::DYNAMIC_DATA_JSON_DISCRIMINATOR_ACTIVE_MEMBER;
  std::string serialized;
  ASSERT_EQ(DDS::RETCODE_OK, OpenDDS::XTypes::dynamic_data_to_json(serialized, data, opts));

  DDS::DynamicData_var copy = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(copy, serialized.c_str(), opts));
  EXPECT_TRUE(OpenDDS::XTypes::dynamic_data_equal(data, copy));
}

TEST(dds_DCPS_XTypes_DynamicDataJson, BitmaskDiscriminatorHighBit)
{
  // HighBitUnion has a bitBound=32 bitmask discriminator with BIT_31 at
  // position 31.  Its case label value is (uint32)1<<31 = 0x80000000,
  // stored as the signed Int32 -2147483648 in the TypeObject.
  // The old range check (> INT32_MAX) incorrectly rejected this discriminator
  // value when supplied as a numeric JSON integer or as the flag name.
  DDS::DynamicType_var type = load_type("XmlTypeProviderTest::HighBitUnion");
  DDS::DynamicData_var data = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_TRUE(data);

  // Via named flag — exercises bitmask_text_to_value + fixed range check.
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(data, "{\"$discriminator\":\"BIT_31\",\"b\":7}"));
  DDS::MemberDescriptor_var b = member_descriptor(type, "b");
  DDS::Int32 b_val = 0;
  ASSERT_EQ(DDS::RETCODE_OK, data->get_int32_value(b_val, b->id()));
  EXPECT_EQ(7, b_val);

  // Via integer value 2147483648 (= 0x80000000, above INT32_MAX).
  DDS::DynamicData_var data2 = DDS::DynamicDataFactory::get_instance()->create_data(type);
  ASSERT_EQ(DDS::RETCODE_OK,
            OpenDDS::XTypes::dynamic_data_from_json(data2, "{\"$discriminator\":2147483648,\"b\":7}"));
  EXPECT_TRUE(OpenDDS::XTypes::dynamic_data_equal(data, data2));
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
