
#include "gtest/gtest.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#include <cstring>

using DDS::Property_t;
using OpenDDS::DCPS::Serializer;


class SerializePropertyTest : public ::testing::Test
{
public:

  SerializePropertyTest() : buffer(1024),
			    serializer(&buffer,
				       false,
				       Serializer::ALIGN_CDR){}
  virtual ~SerializePropertyTest(){}

  Property_t in;
  Property_t out;
  ACE_Message_Block buffer;
  Serializer serializer;
};

TEST_F(SerializePropertyTest, Basic_Serialize)
{
  in.name = "com.objectcomputing.param";
  in.value = "com.objectcomputing.value";
  in.propagate = true;

  out.name = "none";
  out.value = "none";
  out.propagate = false;
  
  serializer << in;
  serializer >> out;

  ASSERT_TRUE(std::strcmp(out.name.in(), in.name.in()) == 0);
  ASSERT_TRUE(std::strcmp(out.value.in(), in.value.in()) == 0);
  ASSERT_TRUE(out.propagate);
}

/* TODO: Fix segfault
TEST_F(SerializePropertyTest, When_Propagate_False)
{
  in.name = "com.objectcomputing.param";
  in.value = "com.objectcomputing.value";
  in.propagate = false;

  out.name = "none";
  out.value = "none";
  out.propagate = false;

  serializer << in;
  serializer >> out;

  ASSERT_TRUE(std::strcmp(out.name.in(), in.name.in()) == 0);
  ASSERT_TRUE(std::strcmp(out.value.in(), in.value.in()) == 0);
  ASSERT_TRUE(in.propagate);
}
*/
