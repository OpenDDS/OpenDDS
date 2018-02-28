/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "gtest/gtest.h"
#include "dds/DCPS/security/SSL/Certificate.h"
#include <iostream>

using namespace OpenDDS::Security::SSL;

class CertificateTest : public ::testing::Test
{
public:
  CertificateTest() :
    ca_("file:../certs/opendds_identity_ca_cert.pem"),
    signed_("file:../certs/opendds_participant_cert.pem")
  {

  }

  ~CertificateTest()
  {

  }

  Certificate ca_;
  Certificate signed_;
};

TEST_F(CertificateTest, Validate_Success)
{
  ASSERT_EQ(signed_.validate(ca_), X509_V_OK);
}

TEST_F(CertificateTest, Validate_Failure_LoadingWrongKeyType)
{
  Certificate wrong_key_type("file:../certs/opendds_participant_private_key.pem");
  ASSERT_NE(wrong_key_type.validate(ca_), X509_V_OK);
}

TEST_F(CertificateTest, Validate_Failure_SelfSigned)
{
  Certificate not_signed("file:../certs/opendds_not_signed.pem");
  ASSERT_NE(not_signed.validate(ca_), X509_V_OK);
}
