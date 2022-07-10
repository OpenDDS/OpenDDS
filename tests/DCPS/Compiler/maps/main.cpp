#include "testTypeSupportImpl.h"

#include <dds/DCPS/Definitions.h>

#include <gtest/gtest.h>

const OpenDDS::DCPS::Encoding encoding(OpenDDS::DCPS::Encoding::KIND_UNALIGNED_CDR);

TEST(Maps, MapMarshalling) 
{
  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer strm(b.get(), encoding);

  Data expectedData;
  expectedData.intTest(10);
  expectedData.intIntMap()[10] = 10;

  strm << expectedData;

  Data testData;
  strm >> testData;

  EXPECT_EQ(testData.intIntMap(), expectedData.intIntMap());
  EXPECT_EQ(testData.intIntMap()[10], expectedData.intIntMap()[10]);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
