/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifdef OPENDDS_SECURITY

#include "../sec_doc.h"

#include <dds/DCPS/security/OpenSSL_init.h>
#include <dds/DCPS/security/SSL/SignedDocument.h>
#include <dds/DCPS/security/SSL/Certificate.h>
#include <dds/DCPS/SequenceIterator.h>

#include <gtest/gtest.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace OpenDDS::Security::SSL;

namespace {

class dds_DCPS_security_SSL_SignedDocument : public ::testing::Test
{
public:
  dds_DCPS_security_SSL_SignedDocument() :
    ca_(sec_doc_prop("certs/permissions/permissions_ca_cert.pem")),
    ca_data_("data:,-----BEGIN CERTIFICATE-----\n"
      "MIID9DCCAtwCCQCkjopvwK438jANBgkqhkiG9w0BAQsFADCBuzELMAkGA1UEBhMC\n"
      "VVMxCzAJBgNVBAgMAk1PMRQwEgYDVQQHDAtTYWludCBMb3VpczEvMC0GA1UECgwm\n"
      "T2JqZWN0IENvbXB1dGluZyAoVGVzdCBQZXJtaXNzaW9ucyBDQSkxLzAtBgNVBAMM\n"
      "Jk9iamVjdCBDb21wdXRpbmcgKFRlc3QgUGVybWlzc2lvbnMgQ0EpMScwJQYJKoZI\n"
      "hvcNAQkBFhhpbmZvQG9iamVjdGNvbXB1dGluZy5jb20wHhcNMTgwNjEzMDQyMDEz\n"
      "WhcNMjgwNjEwMDQyMDEzWjCBuzELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAk1PMRQw\n"
      "EgYDVQQHDAtTYWludCBMb3VpczEvMC0GA1UECgwmT2JqZWN0IENvbXB1dGluZyAo\n"
      "VGVzdCBQZXJtaXNzaW9ucyBDQSkxLzAtBgNVBAMMJk9iamVjdCBDb21wdXRpbmcg\n"
      "KFRlc3QgUGVybWlzc2lvbnMgQ0EpMScwJQYJKoZIhvcNAQkBFhhpbmZvQG9iamVj\n"
      "dGNvbXB1dGluZy5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCd\n"
      "3osCHskwiWPkgQ+FiUJEPj9lGAV6gqnG9XcTHPzOsv+hrWcklq4WcTcu5ERxjvwz\n"
      "rfB9MV2Jj1mhnAQfp0sIuTJe4QoXigyf0IyezsSA1oeofkJuBlA6cR+5ATzfNEcJ\n"
      "JG3sVaEaa0L92CXb147LczMMY+6I/jD9H/Kamoph1hCgdh2lGnYN97ETMxX5qINt\n"
      "hO17/qZ55R+H5nE2Op1f4Y0LhjKu3WztEjIZeAJDgAksoYRynVhfDsshdZWUMSO0\n"
      "jHJGPwEvxwhTsAknWdthuE/xgZQqDP3aXj3MFJcZkydS+8xvnX0cuHsr/7MqVK0o\n"
      "OmjWS7pi7cMBY9DtB3KVAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAE9QWa1xNjxL\n"
      "WIw88eVrQxOBCIlqCkAiTx2pAurEdiDtz8ZQdDMQQmoAuppT6LWVVtOWc1bP3a+I\n"
      "HBolNAimXOm+B9fMSvQnqRbriJZ8Hc5+Y5TXlJ3iyqJDEyPiWhUFLfQfnjE8hRL5\n"
      "oKPkhk2gRC6K5x+10cZMclgEmZONANtAuSJurMhwgqLxwgGw51aIpL6LTxtdZ33L\n"
      "IPM8AN51Tgj5t2VM/49iNq9HdqAl7VQuyHEc/eCAIp7p69nqcpS9VBJAJoHN8lmD\n"
      "DHYxM+pYtQAgmBKLBxTyDrgJZ+3j3FVOp0orRxarE3XjJ+0bIVnO6yhjunPOpgsy\n"
      "EcxH9/7Enm8=\n"
      "-----END CERTIFICATE-----"),
    doc_()
  {
    DDS::Security::SecurityException ex;
    if (!doc_.load(sec_doc_prop("governance/governance_SC1_ProtectedDomain1_signed.p7s"), ex)) {
      throw std::runtime_error("Could not load governance file");
    }

  }

  ~dds_DCPS_security_SSL_SignedDocument()
  {
  }

  Certificate ca_;
  Certificate ca_data_;
  SignedDocument doc_;
};

}

TEST_F(dds_DCPS_security_SSL_SignedDocument, GetContent_Success)
{
  const DDS::OctetSeq& content =  doc_.original();
  ASSERT_TRUE(0 < content.length());
}

TEST_F(dds_DCPS_security_SSL_SignedDocument, VerifySignature_Success)
{
  ASSERT_TRUE(doc_.verify(ca_));
}

TEST_F(dds_DCPS_security_SSL_SignedDocument, VerifySignature_Data_Success)
{
  ASSERT_TRUE(doc_.verify(ca_data_));
}

TEST_F(dds_DCPS_security_SSL_SignedDocument, LoadFromMemory)
{
  const std::string path = sec_doc_path("governance/governance_SC1_ProtectedDomain1_signed.p7s");
  std::ifstream file(path.c_str(), std::ifstream::binary);
  std::ostringstream mem;
  mem << file.rdbuf();
  const std::string str = mem.str();
  DDS::OctetSeq seq(static_cast<CORBA::ULong>(str.size()),
                    static_cast<CORBA::ULong>(str.size()),
                    reinterpret_cast<unsigned char*>(
                      const_cast<char*>(str.c_str())));

  *OpenDDS::DCPS::back_inserter(seq) = 0u;

  SignedDocument doc(seq);
  ASSERT_EQ(doc, doc_);
}

TEST_F(dds_DCPS_security_SSL_SignedDocument, CopyAssignment)
{
  SignedDocument doc = doc_;
  ASSERT_EQ(doc, doc_);
}

TEST_F(dds_DCPS_security_SSL_SignedDocument, Assignment)
{
  SignedDocument doc;
  doc = doc_;
  ASSERT_EQ(doc, doc_);
}

TEST_F(dds_DCPS_security_SSL_SignedDocument, CopyConstruct)
{
  SignedDocument doc(doc_);
  ASSERT_EQ(doc, doc_);
}

TEST_F(dds_DCPS_security_SSL_SignedDocument, content)
{
  SignedDocument doc;
  doc.content("my content");
  EXPECT_EQ(doc.content(), "my content");
}

#endif
