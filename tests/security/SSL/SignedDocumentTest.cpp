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
    doc_("file:../certs/mock_participant_1/opendds_participant_cert.pem")
  {

  }

  ~SignedDocumentTest()
  {

  }

  Certificate ca_;
  SignedDocument doc_;
};

TEST_F(SignedDocumentTest, GetName_CompareName_Success)
{
  // TODO
}
