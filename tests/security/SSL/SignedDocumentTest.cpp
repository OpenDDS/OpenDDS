/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "gtest/gtest.h"
#include "dds/DCPS/security/SSL/SignedDocument.h"
#include "dds/DCPS/security/SSL/Certificate.h"
#include <iostream>
#include <fstream>

using namespace OpenDDS::Security::SSL;

class SignedDocumentTest : public ::testing::Test
{
public:
  SignedDocumentTest() :
    ca_("file:../certs/opendds_identity_ca_cert.pem"),
    doc_("file:../governance/Governance_SC1_ProtectedDomain1.p7s")
  {
  }

  ~SignedDocumentTest()
  {
  }

  Certificate ca_;
  SignedDocument doc_;
};

TEST_F(SignedDocumentTest, GetContent_Success)
{
  std::string content;
  doc_.get_content(content);
  ASSERT_TRUE(0 < content.length());
}

TEST_F(SignedDocumentTest, VerifySignature_Success)
{
  ASSERT_EQ(0, doc_.verify_signature(ca_));
}

TEST(SignedDocumentTestNoFixture, LoadFromMemory)
{
  const char fname[] = "../governance/Governance_SC1_ProtectedDomain1.p7s";
  std::ifstream file(fname);
  std::ostringstream mem;
  mem << file.rdbuf();
  const std::string str = mem.str();
  DDS::OctetSeq seq(str.size(), str.size(),
                    reinterpret_cast<unsigned char*>(
                      const_cast<char*>(str.c_str())));
  SignedDocument doc(seq);
  ASSERT_EQ(doc, SignedDocument(std::string("file:") + fname));
}
