/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifdef OPENDDS_SECURITY

#include "gtest/gtest.h"
#include "dds/DCPS/security/OpenSSL_init.h"
#include "dds/DCPS/security/SSL/Certificate.h"
#include <iostream>

using namespace OpenDDS::Security::SSL;

class dds_DCPS_security_SSL_Certificate : public ::testing::Test
{
public:
  dds_DCPS_security_SSL_Certificate() :
    ca_("file:../security/certs/identity/identity_ca_cert.pem"),
    ca_data_("data:,-----BEGIN CERTIFICATE-----\n"
      "MIID6DCCAtACCQCSMMZ5KQ7ffTANBgkqhkiG9w0BAQsFADCBtTELMAkGA1UEBhMC\n"
      "VVMxCzAJBgNVBAgMAk1PMRQwEgYDVQQHDAtTYWludCBMb3VpczEsMCoGA1UECgwj\n"
      "T2JqZWN0IENvbXB1dGluZyAoVGVzdCBJZGVudGl0eSBDQSkxLDAqBgNVBAMMI09i\n"
      "amVjdCBDb21wdXRpbmcgKFRlc3QgSWRlbnRpdHkgQ0EpMScwJQYJKoZIhvcNAQkB\n"
      "FhhpbmZvQG9iamVjdGNvbXB1dGluZy5jb20wHhcNMTgwNjEzMDQxMTAzWhcNMjgw\n"
      "NjEwMDQxMTAzWjCBtTELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAk1PMRQwEgYDVQQH\n"
      "DAtTYWludCBMb3VpczEsMCoGA1UECgwjT2JqZWN0IENvbXB1dGluZyAoVGVzdCBJ\n"
      "ZGVudGl0eSBDQSkxLDAqBgNVBAMMI09iamVjdCBDb21wdXRpbmcgKFRlc3QgSWRl\n"
      "bnRpdHkgQ0EpMScwJQYJKoZIhvcNAQkBFhhpbmZvQG9iamVjdGNvbXB1dGluZy5j\n"
      "b20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDR3CBzI5ch2hTdvpFD\n"
      "U3bsa90po9VqyHdT3GTvk0njWD6WUcA2YUptG+i1MX9vwEtjsxHlwxpMp41J/TNn\n"
      "8kWe770zBW8lhnKfzJfAENnqWso3j01v/CUPERqxms6NYHJQucGZW11Y4vthGURd\n"
      "XGDxeiRyDVrk87G+cv5rFmPDtLEqkgSoOpJkLMOUXfWFzO/v/ZP66vhNtNu7iwc8\n"
      "kyTQPHoWS8P7+gUu3dwr0sHUrpRffssBaPrSKxPxlXrtll1x4vPoMHDTddHweJWy\n"
      "1tq5MMTve7slG44YoMbQcAYSE5m6YMHx0Ht81jWF+v/OMW7iZY2Pvv9n9A4wS3p9\n"
      "kkFZAgMBAAEwDQYJKoZIhvcNAQELBQADggEBADM5PKagItR5cStb99lqukz8uOx2\n"
      "3rexro5FITdLSEgANu4uaB4rosdL2/L8Mft6TD2HVh5Wl5uwhDxjnLTGeVGQd1HN\n"
      "bvEzWyFjAfwzoRptTZiqsZhuQgSbtEcdUotdx7GbIc/xWNMAQ6t+ruRbzFDDdq4f\n"
      "7Ob7iAVWXUV0Sm4R7bgVCIiIKztSiYoQOwiLjV4DmNGcCFs4jbLXFPQWrqzTpAPB\n"
      "cPsocF+pCa71fQnlW8L9cyn2E/6zJkzj4aYZWwc2KVJm5JBuaIIcu1oeI5IdZ5sl\n"
      "w5GeIeP0nXVHuhQf4mWrMi+KvQhorQEGftxnDhS0TWWEFVOUigRao7blBXY=\n"
      "-----END CERTIFICATE-----"),
    signed_("file:../security/certs/identity/test_participant_01_cert.pem"),
    signed_data_("data:,-----BEGIN CERTIFICATE-----\n"
      "MIIDhjCCAm4CAQEwDQYJKoZIhvcNAQELBQAwgbUxCzAJBgNVBAYTAlVTMQswCQYD\n"
      "VQQIDAJNTzEUMBIGA1UEBwwLU2FpbnQgTG91aXMxLDAqBgNVBAoMI09iamVjdCBD\n"
      "b21wdXRpbmcgKFRlc3QgSWRlbnRpdHkgQ0EpMSwwKgYDVQQDDCNPYmplY3QgQ29t\n"
      "cHV0aW5nIChUZXN0IElkZW50aXR5IENBKTEnMCUGCSqGSIb3DQEJARYYaW5mb0Bv\n"
      "YmplY3Rjb21wdXRpbmcuY29tMB4XDTE4MDYxMzA0MTMxNloXDTI4MDYxMDA0MTMx\n"
      "NlowXDELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM\n"
      "GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDEVMBMGA1UEAwwMT3p6aWUgT3ptYW5u\n"
      "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsnrguqg0qT/phhwsT7I0\n"
      "QfqK3GdYtgdjzk1y2qkMvsAACSwUxqEDxwQq3onpZ2wOSKfCK2dnAtj4l39zA7Pk\n"
      "Jg3syE98/cFxWLDznjvti73xXoHJOZ7q/V2d6/jRFixYWHLa2+nUJ6gCcvC9Z+Kk\n"
      "fxfv5QHw4/y6WKzG+f8+wvThT2x/+80n62MvTpyGtOV1TP2bhEqhgrxQHJkv3N4K\n"
      "rnC+niiy4ShHXytwnM8tell3IBFmuGIrmGn1OI9RiljjSt2nKc8vAbxcvnifjk15\n"
      "PIwRlVXk1jVNu1FmWlTxAMd5VumyfjmZzk/1t0n8xDAerDfvmr33ycDmo9h7OBnW\n"
      "bQIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQBH9Cdn8L93xmre0hTkDAeq08NEBIll\n"
      "Cf7BpreOg7tDoT59AqomBNEP5X9yl/WFI0ywcTsPqKAIIyQVmFRf4fM5PxLwn3Ii\n"
      "YeJVBhmZ7+6Ul5h0Oxtjbnrtl9gb2Pu4EFDh1ulCUow9p/H5ceNk2jGtogEcodzJ\n"
      "9U1WpJDneVqLw9GPlSiOaG22kQ8z+3Bhxgx5zJXa9gxzVwPQ1DfTYF2004UYeOk+\n"
      "1Duu88crRp00ghesoFKx5OURjTrImIo5cQEazVQ7sX3BAki3egwpaKXNiWQ7VIuZ\n"
      "NFqBYfNRpivWQhcdG7g0MTK/AF6ddad6GdaknYlmBhlYpSd/K9DYIkMx\n"
      "-----END CERTIFICATE-----"),
    signed_ec_("file:../security/certs/identity/test_participant_03_cert.pem"),
    not_signed_("file:../security/certs/identity/not_signed.pem")
  {

  }

  ~dds_DCPS_security_SSL_Certificate()
  {
  }

  Certificate ca_;
  Certificate ca_data_;
  Certificate signed_;
  Certificate signed_data_;
  Certificate signed_ec_;
  Certificate not_signed_;
};

TEST_F(dds_DCPS_security_SSL_Certificate, Validate_RSASSA_PSS_SHA256_Success)
{
  ASSERT_EQ(signed_.validate(ca_), X509_V_OK);
}

TEST_F(dds_DCPS_security_SSL_Certificate, Validate_RSASSA_PSS_SHA256_Data_Success)
{
  ASSERT_EQ(signed_.validate(ca_data_), X509_V_OK);
}

TEST_F(dds_DCPS_security_SSL_Certificate, Validate_ECDSA_SHA256_Data_Success)
{
  ASSERT_EQ(signed_ec_.validate(ca_data_), X509_V_OK);
}

TEST_F(dds_DCPS_security_SSL_Certificate, Validate_RSASSA_PSS_SHA256_Failure_LoadingWrongKeyType)
{
  Certificate wrong_key_type("file:../security/certs/identity/test_participant_01_private_key.pem");
  ASSERT_NE(wrong_key_type.validate(ca_), X509_V_OK);
}

TEST_F(dds_DCPS_security_SSL_Certificate, Validate_RSASSA_PSS_SHA256_Failure_SelfSigned)
{
  ASSERT_NE(not_signed_.validate(ca_), X509_V_OK);
}

TEST_F(dds_DCPS_security_SSL_Certificate, SubjectNameDigest_RSASSA_PSS_SHA256_Success)
{
  typedef std::vector<unsigned char> hash_vec;

  hash_vec hash1, hash2;
  signed_.subject_name_digest(hash1);
  signed_data_.subject_name_digest(hash2);

  ASSERT_EQ(32u, hash1.size());
  ASSERT_EQ(hash1, hash2);
}

TEST_F(dds_DCPS_security_SSL_Certificate, Algorithm_RSASSA_PSS_SHA256_Success)
{
  std::string algorithm(signed_.dsign_algo());
  ASSERT_EQ(std::string("RSASSA-PSS-SHA256"), algorithm);
}

TEST_F(dds_DCPS_security_SSL_Certificate, Algorithm_ECDSA_SHA256_Success)
{
    std::string algorithm(signed_ec_.dsign_algo());
    ASSERT_EQ(std::string("ECDSA-SHA256"), algorithm);
}

TEST_F(dds_DCPS_security_SSL_Certificate, SubjectNameToString_Success)
{
  /* From this cmd: openssl x509 -noout -subject -in ./security/certs/identity/test_participant_01_cert.pem */
  std::string expected("C = AU, ST = Some-State, O = Internet Widgits Pty Ltd, CN = Ozzie Ozmann");
  std::string name;
  ASSERT_EQ(signed_.subject_name_to_str(name), 0 /* success! */);
  ASSERT_EQ(name, expected);
}

TEST_F(dds_DCPS_security_SSL_Certificate, SerializeDeserialize_RSASSA_PSS_SHA256_Success)
{
  DDS::OctetSeq tmp;
  ASSERT_EQ(0, signed_.serialize(tmp));

  Certificate copy(tmp);
  ASSERT_EQ(copy, signed_);
}

TEST_F(dds_DCPS_security_SSL_Certificate, SerializeDeserialize_ECDSA_SHA256_Success)
{
  DDS::OctetSeq tmp;
  ASSERT_EQ(0, signed_ec_.serialize(tmp));

  Certificate copy(tmp);
  ASSERT_EQ(copy, signed_ec_);
}

TEST_F(dds_DCPS_security_SSL_Certificate, AssignmentOperator_RSASSA_PSS_SHA256)
{
  Certificate c;
  c = signed_;
  ASSERT_EQ(c, signed_);
}

TEST_F(dds_DCPS_security_SSL_Certificate, AssignmentOperator_ECDSA_SHA256)
{
  Certificate c;
  c = signed_ec_;
  ASSERT_EQ(c, signed_ec_);
}

TEST_F(dds_DCPS_security_SSL_Certificate, CopyConstruct_RSASSA_PSS_SHA256)
{
  Certificate c(signed_);
  ASSERT_EQ(c, signed_);
}

TEST_F(dds_DCPS_security_SSL_Certificate, CopyConstruct_ECDSA_SHA256)
{
  Certificate c(signed_ec_);
  ASSERT_EQ(c, signed_ec_);
}

#endif
