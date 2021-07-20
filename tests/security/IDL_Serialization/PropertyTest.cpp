
#include "gtest/gtest.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#include <cstring>
#include <sstream>

using DDS::Property_t;
using DDS::PropertySeq;
using OpenDDS::DCPS::Serializer;


class SerializePropertyTest : public ::testing::Test
{
public:

  SerializePropertyTest()
    : buffer(1024)
    , serializer(&buffer, OpenDDS::DCPS::Encoding::KIND_XCDR1)
  {}

  virtual ~SerializePropertyTest(){}

  Property_t in;
  Property_t out;
  PropertySeq inseq;
  PropertySeq outseq;
  ACE_Message_Block buffer;
  Serializer serializer;
};

TEST_F(SerializePropertyTest, PropertySeq_OnePropagateFalse_TwoPropagateTrue)
{
  inseq.length(3);
  for (CORBA::ULong i = 0; i < inseq.length(); ++i) {

    std::stringstream name, value;
    name << "name " << i;
    value << "value " << i;

    Property_t p;
    p.name = name.str().c_str();
    p.value = name.str().c_str();
    p.propagate = true;

    inseq[i] = p;
  }

  inseq[0].propagate = false;

  ASSERT_TRUE(serializer << inseq);
  ASSERT_TRUE(serializer >> outseq);
  ASSERT_EQ(2u, outseq.length());
}

TEST_F(SerializePropertyTest, Property_Round_Trip)
{
  in.name = "com.objectcomputing.param";
  in.value = "com.objectcomputing.value";
  in.propagate = true;

  out.name = "none";
  out.value = "none";
  out.propagate = false;

  ASSERT_FALSE(std::strcmp(out.name.in(), in.name.in()) == 0);
  ASSERT_FALSE(std::strcmp(out.value.in(), in.value.in()) == 0);

  ASSERT_TRUE(buffer.length() == 0);
  ASSERT_TRUE(serializer << in);
  ASSERT_TRUE(buffer.length() > 0);
  ASSERT_TRUE(serializer >> out);
  ASSERT_TRUE(buffer.length() == 0);

  ASSERT_TRUE(std::strcmp(out.name.in(), in.name.in()) == 0);
  ASSERT_TRUE(std::strcmp(out.value.in(), in.value.in()) == 0);

  ASSERT_TRUE(out.propagate);
}


TEST_F(SerializePropertyTest, Property_When_Propagate_False)
{
  in.name = "com.objectcomputing.param";
  in.value = "com.objectcomputing.value";
  in.propagate = false;

  ASSERT_TRUE(buffer.length() == 0);
  serializer << in;
  ASSERT_TRUE(buffer.length() == 0);

  ASSERT_TRUE(std::strcmp(in.name.in(), "com.objectcomputing.param") == 0);
  ASSERT_TRUE(std::strcmp(in.value.in(), "com.objectcomputing.value") == 0);
  ASSERT_FALSE(in.propagate);
}
