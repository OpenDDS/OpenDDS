
#include "gtest/gtest.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#include <cstring>

using DDS::BinaryProperty_t;
using OpenDDS::DCPS::Serializer;


class SerializeBinaryPropertyTest : public ::testing::Test
{
public:

  SerializeBinaryPropertyTest()
    : buffer(1024)
    , serializer(&buffer, OpenDDS::DCPS::Encoding::KIND_XCDR1)
  {
  }

  virtual ~SerializeBinaryPropertyTest()
  {
  }

  BinaryProperty_t in;
  BinaryProperty_t out;
  ACE_Message_Block buffer;
  Serializer serializer;
};


TEST_F(SerializeBinaryPropertyTest, Round_Trip)
{
  const unsigned char indata[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  const size_t inlen = sizeof(indata);

  const unsigned char outdata[] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
  const size_t outlen = sizeof(outdata);

  in.name = "com.objectcomputing.param";
  in.value.length(inlen);
  std::memcpy(in.value.get_buffer(), &indata, inlen);
  in.propagate = true;

  out.name = "none";
  out.value.length(outlen);
  std::memcpy(out.value.get_buffer(), &outdata, outlen);
  out.propagate = false;

  ASSERT_FALSE(std::strcmp(out.name.in(), in.name.in()) == 0);
  ASSERT_FALSE(std::memcmp(out.value.get_buffer(), in.value.get_buffer(), outlen) == 0);

  ASSERT_TRUE(buffer.length() == 0);
  ASSERT_TRUE(serializer << in);
  ASSERT_TRUE(buffer.length() > 0);
  ASSERT_TRUE(serializer >> out);
  ASSERT_TRUE(buffer.length() == 0);

  ASSERT_TRUE(std::strcmp(out.name.in(), in.name.in()) == 0);
  ASSERT_TRUE(std::memcmp(out.value.get_buffer(), in.value.get_buffer(), outlen) == 0);

  ASSERT_TRUE(out.propagate);
}

TEST_F(SerializeBinaryPropertyTest, When_Propagate_False)
{
  const unsigned char indata[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  const size_t inlen = sizeof(indata);

  in.name = "com.objectcomputing.param";
  in.value.length(inlen);
  std::memcpy(in.value.get_buffer(), &indata, inlen);
  in.propagate = false;

  out.name = in.name;
  out.value = in.value;
  out.propagate = in.propagate;

  ASSERT_TRUE(std::strcmp(out.name.in(), in.name.in()) == 0);
  ASSERT_TRUE(std::memcmp(out.value.get_buffer(), in.value.get_buffer(), inlen) == 0);

  ASSERT_TRUE(buffer.length() == 0);
  serializer << in;
  ASSERT_TRUE(buffer.length() == 0);

  ASSERT_TRUE(std::strcmp(out.name.in(), in.name.in()) == 0);
  ASSERT_TRUE(std::memcmp(out.value.get_buffer(), in.value.get_buffer(), inlen) == 0);

  ASSERT_FALSE(in.propagate);
}
