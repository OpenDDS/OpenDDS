#include <dds/DCPS/JsonValueReader.h>
#include <dds/DCPS/JsonValueWriter.h>
#include "testTypeSupportImpl.h"

#include <gtest/gtest.h>

const OpenDDS::DCPS::Encoding encoding(OpenDDS::DCPS::Encoding::KIND_UNALIGNED_CDR);

template<typename T>
T TestMarshalling(OpenDDS::DCPS::Serializer strm, T original) {
  strm << original;
  T t;
  strm >> t;
  return t;
}

TEST(MapsTests, Marshalling)
{
  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), encoding);

  Data expectedData;
  expectedData.intIntMap()[10] = 10;
  expectedData.stringStringMap()["Hello"] = "World";
  expectedData.enumIntMap()[TEST_ENUM::TEST1] = 10;
  expectedData.intEnumMap()[10] = TEST_ENUM::TEST1;

  TestStruct stru;
  stru.msg("World");
  expectedData.stringStructsMap()["Hello"] = stru;

  std::map<int32_t, TestStruct> testMap;
  TestStruct t;
  t.id(190);
  t.msg("Hello World");
  testMap[10] = t;

  expectedData.stringMapMap()["Hello World"] = testMap;

  Data testData = TestMarshalling(strm, expectedData);

  EXPECT_EQ(testData.intIntMap(), expectedData.intIntMap());
  EXPECT_EQ(testData.intIntMap()[10], expectedData.intIntMap()[10]);

  EXPECT_EQ(testData.stringStringMap(), expectedData.stringStringMap());
  EXPECT_EQ(testData.stringStringMap()["Hello"], expectedData.stringStringMap()["Hello"]);

  EXPECT_EQ(testData.enumIntMap()[TEST_ENUM::TEST1], expectedData.enumIntMap()[TEST_ENUM::TEST1]);
  EXPECT_EQ(testData.intEnumMap()[10], expectedData.intEnumMap()[10]);

  EXPECT_EQ(testData.stringStructsMap()["Hello"].msg(), expectedData.stringStructsMap()["Hello"].msg());

  EXPECT_EQ(testData.stringMapMap()["Hello World"][190].id(), expectedData.stringMapMap()["Hello World"][190].id());
  EXPECT_EQ(testData.stringMapMap()["Hello World"][190].msg(), expectedData.stringMapMap()["Hello World"][190].msg());
}

TEST(MapsTests, SerializedSize)
{
  Data expectedData;

  // intIntMap
  auto size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.intIntMap());
  EXPECT_EQ(size, 4);
  expectedData.intIntMap()[10] = 10;
  size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.intIntMap());
  EXPECT_EQ(size, 12);
  expectedData.intIntMap()[20] = 10;
  size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.intIntMap());
  EXPECT_EQ(size, 20);

  // stringStringMap
  size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.stringStringMap());
  EXPECT_EQ(size, 4);
  expectedData.stringStringMap()["Hello"] = "World!";
  size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.stringStringMap());
  EXPECT_EQ(size, 25);

  // enumIntMap
  size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.enumIntMap());
  EXPECT_EQ(size, 4);
  expectedData.enumIntMap()[TEST_ENUM::TEST1] = 10;
  size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.enumIntMap());
  EXPECT_EQ(size, 12);

  // intEnumMap
  size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.intEnumMap());
  EXPECT_EQ(size, 4);
  expectedData.intEnumMap()[10] = TEST_ENUM::TEST1;
  size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.intEnumMap());
  EXPECT_EQ(size, 12);

  // stringStructsMap
  TestStruct stru;
  stru.msg("World");

  size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.stringStructsMap());
  EXPECT_EQ(size, 4);
  expectedData.stringStructsMap()["Hello"] = stru;
  size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.stringStructsMap());
  EXPECT_EQ(size, 28);

  // stringMapMap
  std::map<int32_t, TestStruct> testMap;
  TestStruct t;
  t.id(190);
  t.msg("Hello World");
  testMap[10] = t;

  size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.stringMapMap());
  EXPECT_EQ(size, 4);
  expectedData.stringMapMap()["Hello World"] = testMap;
  size = (int32_t) OpenDDS::DCPS::serialized_size(encoding, expectedData.stringMapMap());
  EXPECT_EQ(size, 48);
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
  expectedData.stringStructsMap()["Hello"] = stru;

  std::map<int32_t, TestStruct> testMap;
  TestStruct t;
  t.id(190);
  t.msg("Hello World");
  testMap[10] = t;

  expectedData.stringMapMap()["Hello World"] = testMap;

  vwrite(testWriter, expectedData);

  // std::cout << buffer.GetString() << std::endl;

  rapidjson::StringStream stream = R"(
    {
      "intIntMap":[
        {
          "key":10,
          "value":10
        }
      ],
      "stringStringMap":[
        {
          "key":"Hello",
          "value":"World"
        }
      ],
      "enumIntMap":[
        {
          "key":"TEST1",
          "value":10
        }
      ],
      "intEnumMap":[
        {
          "key":10,
          "value":"TEST1"
        }
      ],
      "stringStructsMap":[
        {
          "key":"Hello",
          "value":
            {
              "id":0,
              "msg":"World"
            }
        }
      ],
      "stringSequenceMap":[],
      "stringMapMap":[
        {
          "key":"Hello World",
          "value":[
            {
              "key":10,
              "value":{
                "id":190,
                "msg":"Hello World"
                }
            }
          ]
        }
      ]
    })";

  OpenDDS::DCPS::JsonValueReader<> testReader(stream);

  Data testData;
  vread(testReader, testData);

  // Test everything is the same
  EXPECT_EQ(testData.intIntMap(), expectedData.intIntMap());
  EXPECT_EQ(testData.intIntMap()[10], expectedData.intIntMap()[10]);

  EXPECT_EQ(testData.stringStringMap(), expectedData.stringStringMap());
  EXPECT_EQ(testData.stringStringMap()["Hello"], expectedData.stringStringMap()["Hello"]);

  EXPECT_EQ(testData.enumIntMap()[TEST_ENUM::TEST1], expectedData.enumIntMap()[TEST_ENUM::TEST1]);
  EXPECT_EQ(testData.intEnumMap()[10], expectedData.intEnumMap()[10]);

  EXPECT_EQ(testData.stringStructsMap()["Hello"].msg(), expectedData.stringStructsMap()["Hello"].msg());

  EXPECT_EQ(testData.stringMapMap()["Hello World"][190].id(), expectedData.stringMapMap()["Hello World"][190].id());
  EXPECT_EQ(testData.stringMapMap()["Hello World"][190].msg(), expectedData.stringMapMap()["Hello World"][190].msg());
}
#endif

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
