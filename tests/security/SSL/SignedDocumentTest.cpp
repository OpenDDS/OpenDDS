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
	ca_data_("data:,-----BEGIN CERTIFICATE-----\n"
		  "MIID4DCCAsgCCQC6Fm9aR8tq2zANBgkqhkiG9w0BAQsFADCBsTELMAkGA1UEBhMC\n"
		  "VVMxCzAJBgNVBAgMAk1PMRQwEgYDVQQHDAtTYWludCBMb3VpczEsMCoGA1UECgwj\n"
		  "T2JqZWN0IENvbXB1dGluZyAoVGVzdCBJZGVudGl0eSBDQSkxKDAmBgNVBAMMH09i\n"
		  "amVjdCBDb21wdXRpbmcgKFRlc3QgSWRlbiBDQSkxJzAlBgkqhkiG9w0BCQEWGGlu\n"
		  "Zm9Ab2JqZWN0Y29tcHV0aW5nLmNvbTAeFw0xODAyMjIxNDU3NThaFw0yODAyMjAx\n"
		  "NDU3NThaMIGxMQswCQYDVQQGEwJVUzELMAkGA1UECAwCTU8xFDASBgNVBAcMC1Nh\n"
		  "aW50IExvdWlzMSwwKgYDVQQKDCNPYmplY3QgQ29tcHV0aW5nIChUZXN0IElkZW50\n"
		  "aXR5IENBKTEoMCYGA1UEAwwfT2JqZWN0IENvbXB1dGluZyAoVGVzdCBJZGVuIENB\n"
		  "KTEnMCUGCSqGSIb3DQEJARYYaW5mb0BvYmplY3Rjb21wdXRpbmcuY29tMIIBIjAN\n"
		  "BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA13849bph3Hd893P2JADbJ / a + ITlL\n"
		  "9RiCteEVJapvfZMTiPc7sSebLFCo3 / 3RuwszDQi72w6D0ksCJDc4HgNav5bvXCdW\n"
		  "6mZx6F08qxUsGTLmnSxCaq / jJloF3BQm39skg1E8P6KwB904sXj / MPARunk1bfqg\n"
		  "0ZMRs + uRyUcNpKK1vWaHxo0TLXxgcK8KdLsFZBCoNNEJ51WTjuV + 35dBQhax53z +\n"
		  "W3mkEuxpiG + Gu5ONIszl8 / nMcaq6TeJj8R + LPxaLZhrTlCXYaeal1dnpWivHC4kq\n"
		  "1Vj8JdMwmmA0uNF0m9mgtru9cNYdFHaGoMSrFW3boyb6M7W8e5GnKtVsFQIDAQAB\n"
		  "MA0GCSqGSIb3DQEBCwUAA4IBAQC1L + sl + bQnkCq7 / dn4oJLajCPxuwbFslv48yyX\n"
		  "rF / BPH5Ntef / 25fWqWaehY2Y5UVr37TeSntGMqOzJmfQ10mmX3eHcCsdlK3yediD\n"
		  "w1Uzocao6sPnaCyHVvlsGxaI42F3b + 6VFB7gyUBfYYDUZx / +y8tFTeqcISXmu9 / 0\n"
		  "MW8Q7crfHG / VHx7V8NRcqor3z21p3popBSVoUoWAFYITsumYnds19Z1DqGpsKxtF\n"
		  "KEC4MDmHz1OdXYJFB1cJlU1J00p5FtfH33crq / JLXoQyfNUtIX57a35OT1v9NqjV\n"
		  "zTgZIlsjwL4wEVprsRXQFs7u7hDLlnSXslddgnp51bXKmQpp\n"
		  "-----END CERTIFICATE-----"),
	doc_("file:../governance/Governance_SC1_ProtectedDomain1.p7s")
  {
  }

  ~SignedDocumentTest()
  {
  }

  Certificate ca_;
  Certificate ca_data_;
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

TEST_F(SignedDocumentTest, VerifySignature_Data_Success)
{
	ASSERT_EQ(0, doc_.verify_signature(ca_data_));
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
