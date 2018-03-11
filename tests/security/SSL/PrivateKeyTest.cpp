/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "gtest/gtest.h"
#include "dds/DCPS/security/SSL/PrivateKey.h"
#include "dds/DCPS/security/SSL/Certificate.h"
#include <cstring>

using namespace OpenDDS::Security::SSL;

class PrivateKeyTest : public ::testing::Test
{
public:
  PrivateKeyTest() :
    ca_("file:../certs/opendds_identity_ca_cert.pem"),
    pubkey_("file:../certs/mock_participant_1/opendds_participant_cert.pem"),
    privkey_("file:../certs/mock_participant_1/opendds_participant_private_key.pem")
  {

  }

  ~PrivateKeyTest()
  {

  }

  Certificate ca_;
  Certificate pubkey_;
  PrivateKey privkey_;
};

TEST_F(PrivateKeyTest, SignAndVerify_Success)
{
  DDS::OctetSeq hello;
  size_t hello_len = std::strlen("hello");
  hello.length(hello_len);
  std::memcpy(hello.get_buffer(), "hello", hello_len);

  DDS::OctetSeq world;
  size_t world_len = std::strlen("world");
  world.length(world_len);
  std::memcpy(world.get_buffer(), "world", world_len);

  std::vector<const DDS::OctetSeq*> sign_these;
  sign_these.push_back(&hello);
  sign_these.push_back(&world);

  DDS::OctetSeq tmp;
  privkey_.sign(sign_these, tmp);

  int verify_result = pubkey_.verify_signature(tmp, sign_these);

  ASSERT_EQ(0, verify_result);
}

TEST_F(PrivateKeyTest, SignAndVerify_WrongData_Failure)
{
  DDS::OctetSeq hello;
  size_t hello_len = std::strlen("hello");
  hello.length(hello_len);
  std::memcpy(hello.get_buffer(), "hello", hello_len);

  DDS::OctetSeq world;
  size_t world_len = std::strlen("world");
  world.length(world_len);
  std::memcpy(world.get_buffer(), "world", world_len);

  std::vector<const DDS::OctetSeq*> sign_these;
  sign_these.push_back(&hello);
  sign_these.push_back(&world);

  std::vector<const DDS::OctetSeq*> verify_these;
  verify_these.push_back(&hello);

  DDS::OctetSeq tmp;
  privkey_.sign(sign_these, tmp);

  int verify_result = pubkey_.verify_signature(tmp, verify_these);

  ASSERT_EQ(1, verify_result);
}
