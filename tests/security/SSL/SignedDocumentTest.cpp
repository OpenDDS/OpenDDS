/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "gtest/gtest.h"
#include "dds/DCPS/security/SSL/SignedDocument.h"
#include "dds/DCPS/security/SSL/Certificate.h"
#include <iostream>

using namespace OpenDDS::Security::SSL;

class SignedDocumentTest : public ::testing::Test
{
public:
  SignedDocumentTest() :
    ca_("file:../certs/opendds_identity_ca_cert.pem"),
    doc_("file:../governance/Governance_SC0_SecurityDisabled.p7s")
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

TEST_F(SignedDocumentTest, SerializeDeserialize_Success)
{
  std::string tmp;
  doc_.serialize(tmp);

  SignedDocument copy;
  copy.deserialize(tmp);
  ASSERT_EQ(copy, doc_);
}
