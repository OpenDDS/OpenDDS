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
	  signed_("file:../certs/mock_participant_1/opendds_participant_cert.pem"),
	  signed_data_("data:,-----BEGIN CERTIFICATE-----\n"
		  "MIIDxjCCAq4CAQEwDQYJKoZIhvcNAQELBQAwgbExCzAJBgNVBAYTAlVTMQswCQYD\n"
		  "VQQIDAJNTzEUMBIGA1UEBwwLU2FpbnQgTG91aXMxLDAqBgNVBAoMI09iamVjdCBD\n"
		  "b21wdXRpbmcgKFRlc3QgSWRlbnRpdHkgQ0EpMSgwJgYDVQQDDB9PYmplY3QgQ29t\n"
		  "cHV0aW5nIChUZXN0IElkZW4gQ0EpMScwJQYJKoZIhvcNAQkBFhhpbmZvQG9iamVj\n"
		  "dGNvbXB1dGluZy5jb20wHhcNMTgwMjIyMTU0MzIwWhcNMjgwMjIwMTU0MzIwWjCB\n"
		  "nzELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAk1PMSwwKgYDVQQKDCNPYmplY3QgQ29t\n"
		  "cHV0aW5nIChUZXN0IElkZW50aXR5IENBKTEsMCoGA1UEAwwjT2JqZWN0IENvbXB1\n"
		  "dGluZyAoVGVzdCBJZGVudGl0eSBDQSkxJzAlBgkqhkiG9w0BCQEWGGluZm9Ab2Jq\n"
		  "ZWN0Y29tcHV0aW5nLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n"
		  "AMUg7RGT5vvBcMSE + 15 / 8OsqaBikNA5lP9RBsgmjihemn3ldMPON1Rr8kn8ezhxk\n"
		  "vw / L1RwpDzLldRixuO8dY44HhSyi5cIi3MsfS4yug6dy62Nh00ZblOBNZU7ipcvz\n"
		  "0sOYhdnlzQQ0qAuziq54pe6uArTaOgS08ab6TFzggxTpFa0FT0tpX6KLjdPwDjMF\n"
		  "+ T2y2M8ZUJnCofpmWfrusM4qNh39ArcSNsD6WqaYyA07sul5gBzkXspeY41Gr7Gl\n"
		  "LFUquaTtERH9EVify + pvdnXYIcBc9G6ez21g2tgN2SGFJ6kTgcc5t4gLINPr9p3x\n"
		  "GOTjHR9CDj1fsGqCWuiiq2cCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAxT6JWPBo\n"
		  "mKALWJgyfo / uGwq3d4MOZpgIRF96094lHs6HJhtu8tSL4gOLyJAXdSUbAyDrEFLt\n"
		  "grDXVEJs1uUIA0Kw8dZp3dE / fLZUO3bsLZK1zJG9I / 5BSGJNdgm6ThLfHZ6Lh7y3\n"
		  "IFmglZI5H8iE3sa9w9FVlS0oxgJVdPIgMLNakjdnGeIOHKT4I / EJh00sbAOH + ao +\n"
		  "bKBM + XU2zeDBex2sFEHLIcqpF07DSQw / twzT4TvV / mOcSX8NcLkxMu / 5fEvh6n3l\n"
		  "BhJtym57Sl6WN / CtSxBcC3it3ZGjNeeOadQdHV3mDn6xRJzvbP7kxM10L8rUeAPL\n"
		  "WAzIXoHQIBML4w ==\n"
		  "-----END CERTIFICATE-----"),
      signed_ec_("file:../certs/ecdsa/opendds_participant_cert.pem"),
	  not_signed_("file:../certs/opendds_not_signed.pem")
  {

  }

  ~CertificateTest()
  {

  }

  Certificate ca_;
  Certificate ca_data_;
  Certificate signed_;
  Certificate signed_data_;
  Certificate signed_ec_;
  Certificate not_signed_;
};

TEST_F(CertificateTest, Validate_Success)
{
  ASSERT_EQ(signed_.validate(ca_), X509_V_OK);
}

TEST_F(CertificateTest, Validate_Data_Success)
{
	ASSERT_EQ(signed_.validate(ca_data_), X509_V_OK);
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

TEST_F(CertificateTest, Algorithm_EC_Prime_Success)
{
    std::string algo;
    ASSERT_EQ(signed_ec_.algorithm(algo), 0 /* success! */);
    ASSERT_EQ(std::string("EC-prime256v1"), algo);
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
