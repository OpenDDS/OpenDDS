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

TEST(MapsMarshalling, MapIntInt)
{
  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), encoding);

  Data expectedData;
  expectedData.intIntMap()[10] = 10;

  Data testData = TestMarshalling(strm, expectedData);

  EXPECT_EQ(testData.intIntMap(), expectedData.intIntMap());
  EXPECT_EQ(testData.intIntMap()[10], expectedData.intIntMap()[10]);
}

TEST(MapsMarshalling, MapStringString)
{
  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), encoding);

  Data expectedData;
  expectedData.stringStringMap()["Hello"] = "World";

  Data testData = TestMarshalling(strm, expectedData);

  EXPECT_EQ(testData.stringStringMap(), expectedData.stringStringMap());
  EXPECT_EQ(testData.stringStringMap()["Hello"], expectedData.stringStringMap()["Hello"]);
}

TEST(MapsMarshalling, MapStringStruct)
{
  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), encoding);

  Data expectedData;

  TestStruct stru;
  stru.msg("World");
  expectedData.stringStructsMap()["Hello"] = stru;

  auto testData = TestMarshalling(strm, expectedData);

  EXPECT_EQ(testData.stringStructsMap()["Hello"].msg(), expectedData.stringStructsMap()["Hello"].msg());
}

TEST(MapsMarshalling, MapEnumInt)
{
  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), encoding);

  Data expectedData;

  expectedData.enumIntMap()[TEST_ENUM::TEST1] = 10;

  auto testData = TestMarshalling(strm, expectedData);

  EXPECT_EQ(testData.enumIntMap()[TEST_ENUM::TEST1], expectedData.enumIntMap()[TEST_ENUM::TEST1]);
}

TEST(MapsMarshalling, MapStringMap)
{
  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), encoding);

  Data expectedData;

  std::map<int32_t, TestStruct> testMap;
  TestStruct t;
  t.id(190);
  t.msg("Hello World");
  testMap[10] = t;

  expectedData.stringMapMap()["Hello World"] = testMap;

  auto testData = TestMarshalling(strm, expectedData);

  EXPECT_EQ(testData.stringMapMap()["Hello World"][190].id(), expectedData.stringMapMap()["Hello World"][190].id());
  EXPECT_EQ(testData.stringMapMap()["Hello World"][190].msg(), expectedData.stringMapMap()["Hello World"][190].msg());
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
