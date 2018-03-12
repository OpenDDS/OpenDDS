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
    privkey_("file:../certs/mock_participant_1/opendds_participant_private_key.pem"),
    hello_(),
    world_(),
    empty_()
  {
    size_t hello_len = std::strlen("hello");
    hello_.length(hello_len);
    std::memcpy(hello_.get_buffer(), "hello", hello_len);

    size_t world_len = std::strlen("world");
    world_.length(world_len);
    std::memcpy(world_.get_buffer(), "world", world_len);
  }

  ~PrivateKeyTest()
  {

  }

  Certificate ca_;
  Certificate pubkey_;
  PrivateKey privkey_;

  DDS::OctetSeq hello_;
  DDS::OctetSeq world_;
  const DDS::OctetSeq empty_;
};

TEST_F(PrivateKeyTest, SignAndVerify_Success)
{
  std::vector<const DDS::OctetSeq*> sign_these;
  sign_these.push_back(&hello_);
  sign_these.push_back(&world_);

  DDS::OctetSeq tmp;
  privkey_.sign(sign_these, tmp);

  int verify_result = pubkey_.verify_signature(tmp, sign_these);

  ASSERT_EQ(0, verify_result);
}

TEST_F(PrivateKeyTest, SignAndVerify_WrongData_Failure)
{
  std::vector<const DDS::OctetSeq*> sign_these;
  sign_these.push_back(&hello_);
  sign_these.push_back(&world_);

  std::vector<const DDS::OctetSeq*> verify_these;
  verify_these.push_back(&hello_);

  DDS::OctetSeq tmp;
  privkey_.sign(sign_these, tmp);

  int verify_result = pubkey_.verify_signature(tmp, verify_these);

  ASSERT_EQ(1, verify_result);
}

TEST_F(PrivateKeyTest, SignAndVerify_DoesNotUseEmptyData)
{
  std::vector<const DDS::OctetSeq*> sign_these;
  sign_these.push_back(&hello_);
  sign_these.push_back(&empty_);

  std::vector<const DDS::OctetSeq*> verify_these;
  verify_these.push_back(&hello_);

  DDS::OctetSeq tmp;
  privkey_.sign(sign_these, tmp);

  int verify_result = pubkey_.verify_signature(tmp, verify_these);

  ASSERT_EQ(0, verify_result);
}


