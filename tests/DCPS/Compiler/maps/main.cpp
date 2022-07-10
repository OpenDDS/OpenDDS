#include "testTypeSupportImpl.h"

#include <dds/DCPS/Definitions.h>

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

TEST(MapsMarshalling, MapStringStrct)
{
  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), encoding);

  Data expectedData;

  TestStruct stru;
  stru.msg("World");
  expectedData.stringStructsMap()["Hello"] = stru;

  auto testData = TestMarshalling(strm, expectedData);

  // EXPECT_EQ(testData.stringStructsMap(), expectedData.stringStructsMap());
  EXPECT_EQ(testData.stringStructsMap()["Hello"].msg(), expectedData.stringStructsMap()["Hello"].msg());
}

TEST(MapsMarshalling, MapEnumInt)
{
  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), encoding);

  Data expectedData;

  expectedData.enumIntMap()[TEST_ENUM::TEST1] = 10;

  auto testData = TestMarshalling(strm, expectedData);

  // EXPECT_EQ(testData.stringStructsMap(), expectedData.stringStructsMap());
  EXPECT_EQ(testData.enumIntMap()[TEST_ENUM::TEST1], expectedData.enumIntMap()[TEST_ENUM::TEST1]);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
