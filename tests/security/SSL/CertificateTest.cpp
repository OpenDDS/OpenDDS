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
    signed_("file:../certs/mock_participant_1/opendds_participant_cert.pem"),
    not_signed_("file:../certs/opendds_not_signed.pem")
  {

  }

  ~CertificateTest()
  {

  }

  Certificate ca_;
  Certificate signed_;
  Certificate not_signed_;
};

TEST_F(CertificateTest, Validate_Success)
{
  ASSERT_EQ(signed_.validate(ca_), X509_V_OK);
}

TEST_F(CertificateTest, Validate_Failure_LoadingWrongKeyType)
{
  Certificate wrong_key_type("file:../certs/mock_participant_1/opendds_participant_private_key.pem");
  ASSERT_NE(wrong_key_type.validate(ca_), X509_V_OK);
}

TEST_F(CertificateTest, Validate_Failure_SelfSigned)
{
  ASSERT_NE(not_signed_.validate(ca_), X509_V_OK);
}

#if 0 /* TODO */
TEST_F(CertificateTest, SubjectNameDigest)
{
  typedef std::vector<unsigned char> hash_vec;

  hash_vec hash;
  not_signed_.subject_name_digest(hash);

  hash_vec::const_iterator i;
  for (i = hash.cbegin(); i != hash.cend(); ++i) {
    /* Do something with the bytes... */
  }
}
#endif

TEST_F(CertificateTest, Algorithm_RSA_2048_Success)
{
  std::string algo;
  ASSERT_EQ(signed_.algorithm(algo), 0 /* success! */);
  ASSERT_EQ(std::string("RSA-2048"), algo);
}

TEST_F(CertificateTest, SubjectNameToString_Success)
{
  /* From this cmd:  openssl x509 -noout -subject -in ../certs/opendds_participant_cert.pem */
  std::string expected("C = US, ST = MO, O = Object Computing (Test Identity CA), CN = Object Computing (Test Identity CA), emailAddress = info@objectcomputing.com");
  std::string name;
  ASSERT_EQ(signed_.subject_name_to_str(name), 0 /* success! */);
  ASSERT_EQ(name, expected);
}

TEST_F(CertificateTest, SerializeDeserialize_Success)
{
  DDS::OctetSeq tmp;
  ASSERT_EQ(0, signed_.serialize(tmp));

  Certificate copy(tmp);
  ASSERT_EQ(copy, signed_);
}



