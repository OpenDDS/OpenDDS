#include <dds/DCPS/JsonValueReader.h>
#include <dds/DCPS/JsonValueWriter.h>
#include <dds/DCPS/Xcdr2ValueWriter.h>
#include "testTypeSupportImpl.h"

#include <tests/Utils/DataView.h>

#include <gtest/gtest.h>

const OpenDDS::DCPS::Encoding test_encoding(OpenDDS::DCPS::Encoding::KIND_XCDR2);

template<typename T>
T TestMarshalling(OpenDDS::DCPS::Serializer strm, T original)
{
  strm << original;
  T t;
  strm >> t;
  return t;
}

template<typename T>
bool CheckMaps(T map1, T map2)
{
  if (map1.size() != map2.size()) {
    return false;
  }
  for (auto it = map1.begin(); it != map1.end(); ++it) {
    if (map2.find(it->first) == map2.end()) {
      return false;
    }
    if (it->second != map2[it->first]) {
      return false;
    }
  }
  return true;
}

// equals operator for TestStruct
bool operator==(const TestStruct& lhs, const TestStruct& rhs)
{
  return lhs.id() == rhs.id() && lhs.msg() == rhs.msg();
}

bool operator!=(const TestStruct& lhs, const TestStruct& rhs)
{
  return !(lhs == rhs);
}

bool CheckData(Data test, Data expected)
{
  if (!CheckMaps(test.intIntMap(), expected.intIntMap())) return false;
  if (!CheckMaps(test.stringStringMap(), expected.stringStringMap())) return false;
  if (!CheckMaps(test.enumIntMap(), expected.enumIntMap())) return false;
  if (!CheckMaps(test.intEnumMap(), expected.intEnumMap())) return false;
  if (!CheckMaps(*test.stringStructsMap(), *expected.stringStructsMap())) return false;
  if (!CheckMaps(test.stringSequenceMap(), expected.stringSequenceMap())) return false;
  if (!CheckMaps(test.stringMapMap(), expected.stringMapMap())) return false;

  return true;
}

TEST(MapsTests, Marshalling)
{
  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), test_encoding);

  Data expectedData;
  expectedData.intIntMap()[10] = 10;
  expectedData.stringStringMap()["Hello"] = "World";
  expectedData.enumIntMap()[TEST_ENUM::TEST1] = 10;
  expectedData.intEnumMap()[10] = TEST_ENUM::TEST1;

  TestStruct stru;
  stru.msg("World");
  expectedData.stringStructsMap() = std::map<std::string, TestStruct>{};
  (*expectedData.stringStructsMap())["Hello"] = stru;

  std::map<int32_t, TestStruct> testMap;
  TestStruct t;
  t.id(190);
  t.msg("Hello World");
  testMap[10] = t;

  expectedData.stringMapMap()["Hello World"] = testMap;

  Data testData = TestMarshalling(strm, expectedData);

  CheckData(testData, expectedData);
}

TEST(MapsTests, SerializedSize)
{
  Data expectedData;

  // intIntMap
  auto size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.intIntMap());
  EXPECT_EQ(size, 4);
  expectedData.intIntMap()[10] = 10;
  size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.intIntMap());
  EXPECT_EQ(size, 12);
  expectedData.intIntMap()[20] = 10;
  size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.intIntMap());
  EXPECT_EQ(size, 20);

  // stringStringMap
  size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.stringStringMap());
  EXPECT_EQ(size, 4);
  expectedData.stringStringMap()["Hello"] = "World!";
  size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.stringStringMap());
  EXPECT_EQ(size, 4 + 4 + 8 + 4 + 7);

  // enumIntMap
  size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.enumIntMap());
  EXPECT_EQ(size, 4);
  expectedData.enumIntMap()[TEST_ENUM::TEST1] = 10;
  size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.enumIntMap());
  EXPECT_EQ(size, 12);

  // intEnumMap
  size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.intEnumMap());
  EXPECT_EQ(size, 4);
  expectedData.intEnumMap()[10] = TEST_ENUM::TEST1;
  size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.intEnumMap());
  EXPECT_EQ(size, 12);

  // stringStructsMap
  TestStruct stru;
  stru.msg("World");

  expectedData.stringStructsMap() = std::map<std::string, TestStruct>{};
  size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.stringStructsMap());
  EXPECT_EQ(size, 4 /*optional*/ + 4 /*length*/);
  (*expectedData.stringStructsMap())["Hello"] = stru;
  size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.stringStructsMap());
  EXPECT_EQ(size, 8 /* see above */ + 12 /* key w/ trailing padding */
                  + 12 /* dheader; id; len of msg */ + 6 /* World<nul> */);

  // stringMapMap
  std::map<int32_t, TestStruct> testMap;
  TestStruct t;
  t.id(190);
  t.msg("Hello World");
  testMap[10] = t;

  size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.stringMapMap());
  EXPECT_EQ(size, 4);
  expectedData.stringMapMap()["Hello World"] = testMap;
  size = (int32_t) OpenDDS::DCPS::serialized_size(test_encoding, expectedData.stringMapMap());
  EXPECT_EQ(size, 4 /* see above */ + 16 /* string key */ + 8 /* inner map len; int key*/
            + 12 /* dheader; id; len of msg */ + 12 /* msg contents */);
}

#if OPENDDS_HAS_JSON_VALUE_WRITER
typedef rapidjson::Writer<rapidjson::StringBuffer> Writer;

TEST(MapsTests, ValueWriterReader)
{
  rapidjson::StringBuffer buffer;
  Writer writer(buffer);
  OpenDDS::DCPS::JsonValueWriter<Writer> testWriter(writer);

  Data expectedData;
  expectedData.intIntMap()[10] = 10;
  expectedData.stringStringMap()["Hello"] = "World";
  expectedData.enumIntMap()[TEST_ENUM::TEST1] = 10;
  expectedData.intEnumMap()[10] = TEST_ENUM::TEST1;

  TestStruct stru;
  stru.msg("World");
  expectedData.stringStructsMap() = std::map<std::string, TestStruct>{};
  (*expectedData.stringStructsMap())["Hello"] = stru;

  std::map<int32_t, TestStruct> testMap;
  TestStruct t;
  t.id(190);
  t.msg("Hello World");
  testMap[10] = t;

  expectedData.stringMapMap()["Hello World"] = testMap;

  EXPECT_TRUE(vwrite(testWriter, expectedData));

  rapidjson::StringStream stream = buffer.GetString();
  OpenDDS::DCPS::JsonValueReader<> testReader(stream);

  Data testData;
  EXPECT_TRUE(vread(testReader, testData));

  EXPECT_TRUE(CheckData(testData, expectedData));
}
#endif

TEST(MapsTests, RecursiveTypes)
{
  using namespace OpenDDS::DCPS;
  using namespace OpenDDS::XTypes;

  const TypeIdentifier& tiNodeMap = getMinimalTypeIdentifier<NodeMap_xtag>();
  EXPECT_EQ(TI_STRONGLY_CONNECTED_COMPONENT, tiNodeMap.kind());
  const TypeIdentifier& tiTreeNode = getMinimalTypeIdentifier<TreeNode_xtag>();
  EXPECT_EQ(TI_STRONGLY_CONNECTED_COMPONENT, tiTreeNode.kind());

  const TypeObjectHashId& hashNodeMap = tiNodeMap.sc_component_id().sc_component_id;
  const TypeObjectHashId& hashTreeNode = tiTreeNode.sc_component_id().sc_component_id;
  EXPECT_FALSE(hashNodeMap < hashTreeNode);
  EXPECT_FALSE(hashTreeNode < hashNodeMap);
}

TEST(MapsTest, Xcdr2ValueWriter)
{
  using namespace OpenDDS::DCPS;

  Data value;
  value.intIntMap()[10] = 10;
  value.intIntMap()[20] = 20;
  value.stringStringMap()["Hello"] = "World";
  value.stringStringMap()["World"] = "Hello";
  value.intStringMap() = TestIntStringMap{};
  value.intStringMap()->insert(std::make_pair(3, "three"));
  value.intStringMap()->insert(std::make_pair(2, "two"));
  value.enumIntMap()[TEST_ENUM::TEST1] = 1;
  value.enumIntMap()[TEST_ENUM::TEST2] = 2;
  value.enumIntMap()[TEST_ENUM::TEST3] = 3;
  value.intEnumMap()[1] = TEST_ENUM::TEST1;
  value.intEnumMap()[2] = TEST_ENUM::TEST2;
  value.intEnumMap()[3] = TEST_ENUM::TEST3;
  value.intStructsMap()[4] = TestStruct{4, "four"};
  value.intStructsMap()[5] = TestStruct{5, "five"};
  value.anonSequenceOfMaps().push_back(value.intStructsMap());
  value.anonSequenceOfMaps().push_back(value.intStructsMap());

  const Encoding test_encoding_le(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  const size_t control_size = serialized_size(test_encoding_le, value);
  ACE_Message_Block control_buffer(control_size);
  Serializer control_ser(&control_buffer, test_encoding_le);
  EXPECT_TRUE(control_ser << value);

  Xcdr2ValueWriter value_writer(test_encoding_le);
  EXPECT_TRUE(vwrite(value_writer, value));
  EXPECT_EQ(control_size, value_writer.get_serialized_size());
  ACE_Message_Block buffer(value_writer.get_serialized_size());
  Serializer ser(&buffer, test_encoding_le);
  value_writer.set_serializer(&ser);
  EXPECT_TRUE(vwrite(value_writer, value));
  EXPECT_PRED_FORMAT2(assert_DataView, control_buffer, buffer);

  static const unsigned char expected_cdr[] = {
    116, 1, 0, 0, // dheader 256 + 116 = 372
    2, 0, 0, 0, // intIntMap length
    10, 0, 0, 0, // intIntMap key0
    10, 0, 0, 0, // intIntMap val0
    20, 0, 0, 0, // intIntMap key1
    20, 0, 0, 0, // intIntMap val1
    46, 0, 0, 0, // stringStringMap dheader
    6, 0, 0, 0, // stringStringMap key0
    'H', 'e', 'l', 'l',
    'o', 0, 0, 0,
    6, 0, 0, 0, // stringStringMap val0
    'W', 'o', 'r', 'l',
    'd', 0, 0, 0,
    6, 0, 0, 0, // stringStringMap key1
    'W', 'o', 'r', 'l',
    'd', 0, 0, 0,
    6, 0, 0, 0, // stringStringMap val1
    'H', 'e', 'l', 'l',
    'o', 0, 1, 0, // end of val1; optional intStringMap is present
    26, 0, 0, 0, // intStringMap dheader
    2, 0, 0, 0, // intStringMap key0 (ordered)
    4, 0, 0, 0, // intStringMap val0
    't', 'w', 'o', 0,
    3, 0, 0, 0, // intStringMap key1
    6, 0, 0, 0, // intStringMap val1
    't', 'h', 'r', 'e',
    'e', 0, 0, 0,
    24, 0, 0, 0, // enumIntMap dheader
    0, 0, 0, 0, // enumIntMap key0
    1, 0, 0, 0, // enumIntMap val0
    1, 0, 0, 0, // enumIntMap key1
    2, 0, 0, 0, // enumIntMap val1
    2, 0, 0, 0, // enumIntMap key2
    3, 0, 0, 0, // enumIntMap val2
    24, 0, 0, 0, // intEnumMap dheader
    1, 0, 0, 0, // intEnumMap key0
    0, 0, 0, 0, // intEnumMap val0
    2, 0, 0, 0, // intEnumMap key1
    1, 0, 0, 0, // intEnumMap val1
    3, 0, 0, 0, // intEnumMap key2
    2, 0, 0, 0, // intEnumMap val2
    0, 0, 0, 0, // optional stringStructsMap not present
    45, 0, 0, 0, // intStructsMap dheader
    4, 0, 0, 0, // intStructsMap key0
    13, 0, 0, 0, // intStructsMap val0 dheader
    4, 0, 0, 0, // intStructsMap val0 id
    5, 0, 0, 0, // intStructsMap val0 msg
    'f', 'o', 'u', 'r',
    0, 0, 0, 0,
    5, 0, 0, 0, // intStructsMap key1
    13, 0, 0, 0, // intStructsMap val1 dheader
    5, 0, 0, 0, // intStructsMap val1 id
    5, 0, 0, 0, // intStructsMap val1 msg
    'f', 'i', 'v', 'e',
    0, 0, 0, 0,
    4, 0, 0, 0, // sequenceOfMaps dheader
    0, 0, 0, 0, // sequenceOfMaps length
    105, 0, 0, 0, // anonSequenceOfMaps dheader
    2, 0, 0, 0, // anonSequenceOfMaps length
    45, 0, 0, 0, // anonSequenceOfMaps[0] dheader
    4, 0, 0, 0, // anonSequenceOfMaps[0] key0
    13, 0, 0, 0, // anonSequenceOfMaps[0] val0 dheader
    4, 0, 0, 0, // anonSequenceOfMaps[0] val0 id
    5, 0, 0, 0, // anonSequenceOfMaps[0] val0 msg
    'f', 'o', 'u', 'r',
    0, 0, 0, 0,
    5, 0, 0, 0, // anonSequenceOfMaps[0] key1
    13, 0, 0, 0, // anonSequenceOfMaps[0] val1 dheader
    5, 0, 0, 0, // anonSequenceOfMaps[0] val1 id
    5, 0, 0, 0, // anonSequenceOfMaps[0] val1 msg
    'f', 'i', 'v', 'e',
    0, 0, 0, 0,
    45, 0, 0, 0, // anonSequenceOfMaps[1] dheader
    4, 0, 0, 0, // anonSequenceOfMaps[1] key0
    13, 0, 0, 0, // anonSequenceOfMaps[1] val0 dheader
    4, 0, 0, 0, // anonSequenceOfMaps[1] val0 id
    5, 0, 0, 0, // anonSequenceOfMaps[1] val0 msg
    'f', 'o', 'u', 'r',
    0, 0, 0, 0,
    5, 0, 0, 0, // anonSequenceOfMaps[1] key1
    13, 0, 0, 0, // anonSequenceOfMaps[1] val1 dheader
    5, 0, 0, 0, // anonSequenceOfMaps[1] val1 id
    5, 0, 0, 0, // anonSequenceOfMaps[1] val1 msg
    'f', 'i', 'v', 'e',
    0, 0, 0, 0,
    12, 0, 0, 0, // arrayOfMaps dheader
    0, 0, 0, 0, // arrayOfMaps[0]
    0, 0, 0, 0, // arrayOfMaps[1]
    0, 0, 0, 0, // arrayOfMaps[2]
    8, 0, 0, 0, // anonArrayOfMaps dheader
    0, 0, 0, 0, // anonArrayOfMaps[0]
    0, 0, 0, 0, // anonArrayOfMaps[1]
    0, 0, 0, 0, // stringSequenceMap dheader
    0, 0, 0, 0, // stringMapMap dheader
  };
  EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, control_buffer);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
