/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifdef OPENDDS_SECURITY

#include "../sec_doc.h"

#include <dds/DCPS/security/OpenSSL_init.h>
#include <dds/DCPS/security/SSL/PrivateKey.h>
#include <dds/DCPS/security/SSL/Certificate.h>

#include <gtest/gtest.h>

#include <cstring>

using namespace OpenDDS::Security::SSL;

namespace {

class dds_DCPS_security_SSL_PrivateKey : public ::testing::Test
{
public:
  dds_DCPS_security_SSL_PrivateKey() :
    ca_(sec_doc_prop("certs/identity/identity_ca_cert.pem")),
    ca_data_("data:,-----BEGIN CERTIFICATE-----\n"
      "MIIEJDCCAwwCCQDpnuwIVmSK5jANBgkqhkiG9w0BAQsFADCB0zELMAkGA1UEBhMC\n"
      "VVMxCzAJBgNVBAgMAk1PMRQwEgYDVQQHDAtTYWludCBMb3VpczE7MDkGA1UECgwy\n"
      "T2JqZWN0IENvbXB1dGluZyAoU2VjdXJpdHkgQXR0cmlidXRlcyBJZGVudGl0eSBD\n"
      "QSkxOzA5BgNVBAMMMk9iamVjdCBDb21wdXRpbmcgKFNlY3VyaXR5IEF0dHJpYnV0\n"
      "ZXMgSWRlbnRpdHkgQ0EpMScwJQYJKoZIhvcNAQkBFhhpbmZvQG9iamVjdGNvbXB1\n"
      "dGluZy5jb20wHhcNMTgwNDI1MTkwODIwWhcNMjgwNDIyMTkwODIwWjCB0zELMAkG\n"
      "A1UEBhMCVVMxCzAJBgNVBAgMAk1PMRQwEgYDVQQHDAtTYWludCBMb3VpczE7MDkG\n"
      "A1UECgwyT2JqZWN0IENvbXB1dGluZyAoU2VjdXJpdHkgQXR0cmlidXRlcyBJZGVu\n"
      "dGl0eSBDQSkxOzA5BgNVBAMMMk9iamVjdCBDb21wdXRpbmcgKFNlY3VyaXR5IEF0\n"
      "dHJpYnV0ZXMgSWRlbnRpdHkgQ0EpMScwJQYJKoZIhvcNAQkBFhhpbmZvQG9iamVj\n"
      "dGNvbXB1dGluZy5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCg\n"
      "Nxm9DYJoHz9cmiZX8+4opxXX84aSWTRk2/bjKi7BjXlgf8yPiA8HnabTn2SiiA84\n"
      "SN68BUXuVhZKn2H7oKa68qMvcdEIDVXZiRVOSn8YTDYVS7xFP5PWfhQb0Z8FLy26\n"
      "K4GdMYB3Ou9ba1c9DG5AA4VcdPG41k/8rnAIWdXVlaxR+NKTCivFRxoDAoO+IB3v\n"
      "ONpZgkf35jh9Wlc79WcjWgOSbpk6kVDnaMKxEdlDAFyGqYTdzF+DlE5UNpK4wjWv\n"
      "34CYHJBYPDIe1ctDizQ3ydzfViO0IyV++obJwV5FcduByygXb5CFnNRhUyppbQkh\n"
      "CkKDQMu8W93i9KecD7WJAgMBAAEwDQYJKoZIhvcNAQELBQADggEBACyu2mOP4DKN\n"
      "g43ZbDwPjMufHYGa8lZ8AT87oMGhoYH68ELqN2tIF3XoS9G02EJnCpGNi3RSM/yu\n"
      "Ov4UE6katTrmiSzmD43I55lvzslPxajKOL9OWiEoTcRMUyw3EW5tEQQHNQHb+Ac4\n"
      "j+HCqBeawHOmEx07OXfoT7TvI5jcaUzs2iRBCDliO5EuwqFJj578sYRDI5FbM+xw\n"
      "AT++K8uwXx1wRiSxZMMAPnwGFLhplLKh+jvO/l1hmhDR3DVdzqJ4cfjX1j8A0Xat\n"
      "LULdD3nLP7M1yNJ0HE7/eXCTWgw5dBLik6Ig4d7QjWVa4osvPz3m4frCB8d2lkRr\n"
      "a2FBDC7mCEo=\n"
      "-----END CERTIFICATE-----"),
    pubkey_(sec_doc_prop("certs/identity/test_participant_01_cert.pem")),
    pubkey_data_("data:,-----BEGIN CERTIFICATE-----\n"
      "MIIDpDCCAowCAQEwDQYJKoZIhvcNAQELBQAwgdMxCzAJBgNVBAYTAlVTMQswCQYD\n"
      "VQQIDAJNTzEUMBIGA1UEBwwLU2FpbnQgTG91aXMxOzA5BgNVBAoMMk9iamVjdCBD\n"
      "b21wdXRpbmcgKFNlY3VyaXR5IEF0dHJpYnV0ZXMgSWRlbnRpdHkgQ0EpMTswOQYD\n"
      "VQQDDDJPYmplY3QgQ29tcHV0aW5nIChTZWN1cml0eSBBdHRyaWJ1dGVzIElkZW50\n"
      "aXR5IENBKTEnMCUGCSqGSIb3DQEJARYYaW5mb0BvYmplY3Rjb21wdXRpbmcuY29t\n"
      "MB4XDTE4MDQyNTE5MDkwNVoXDTI4MDQyMjE5MDkwNVowXDELMAkGA1UEBhMCQVUx\n"
      "EzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0IFdpZGdpdHMg\n"
      "UHR5IEx0ZDEVMBMGA1UEAwwMT3p6aWUgT3ptYW5uMIIBIjANBgkqhkiG9w0BAQEF\n"
      "AAOCAQ8AMIIBCgKCAQEAxo3qIy0JSC9DmlenQVMXdcstrwfVmidkjPH6LNdMlxL3\n"
      "ICHRBKGrIYh/Z67o0OEMQe4hnCoh4YH8I/yY9SAVztRUUvj9n5+41rPw7a4S18eI\n"
      "qdGH8QMSiuGIPCXm/QrzWaltp/guHylPIruqp60knHC3L3b58MN1Di8efySEoa0z\n"
      "FXbAc1GBhqncJ66M5qvZxGZfI+IJboJ3kp6TExP91SjXAbw5oIX11HOSKo8GghL8\n"
      "6rleP5Uk0aPjGqaPUsGb0XYBu6OtcrF3AtTtuRNX45AyTc+ccK671H7n3I1+zpNd\n"
      "Fd1lnN0LayE6ZIr+XmMCV5kr5anl8qi7GbWXZ/HkFwIDAQABMA0GCSqGSIb3DQEB\n"
      "CwUAA4IBAQAGJow4WQl7MiJjeEI8Q9G6MKNmYCqBwP/C1J+qwLoLaogtLSa+3+58\n"
      "czw+6S+yepXbsHqh7BKeew4fd6kR8JqliI1PC3y9DZE2dK8/fmzMZKK7sDosRP+p\n"
      "G9ZNz9S0Ih5Un1OA37o2iIU70usR5f+qza7PiNwV1FDRBCDPjUR4V8JML+Nlgg8u\n"
      "MMT1u51S23eps00HOskkjyUkDsTHyGpKwZsbFHZ1nYr8/O3caXpK94d8UiV+yPYH\n"
      "9AdjUsAs4dBaNa1tK0vdJ0vqf7kRoHqPw511+1K4XTpca9cTwCVfeh0h4nNOpeRg\n"
      "xEKmvR/EewryBDJPaU31JQdVyqlEMtgm\n"
      "-----END CERTIFICATE-----"),
    pubkey_ec_(sec_doc_prop("certs/identity/test_participant_03_cert.pem")),
    privkey_(sec_doc_prop("certs/identity/test_participant_01_private_key.pem")),
    privkey_data_("data:,-----BEGIN RSA PRIVATE KEY-----\n"
      "MIIEpQIBAAKCAQEAxo3qIy0JSC9DmlenQVMXdcstrwfVmidkjPH6LNdMlxL3ICHR\n"
      "BKGrIYh/Z67o0OEMQe4hnCoh4YH8I/yY9SAVztRUUvj9n5+41rPw7a4S18eIqdGH\n"
      "8QMSiuGIPCXm/QrzWaltp/guHylPIruqp60knHC3L3b58MN1Di8efySEoa0zFXbA\n"
      "c1GBhqncJ66M5qvZxGZfI+IJboJ3kp6TExP91SjXAbw5oIX11HOSKo8GghL86rle\n"
      "P5Uk0aPjGqaPUsGb0XYBu6OtcrF3AtTtuRNX45AyTc+ccK671H7n3I1+zpNdFd1l\n"
      "nN0LayE6ZIr+XmMCV5kr5anl8qi7GbWXZ/HkFwIDAQABAoIBAQCwfIJKpARlGkXf\n"
      "0dvEL+RheqjvtGoD7NHuikOSSgk1G9F3yTcuA7nGQ7rjYVBmIjOwAsfzNGwLM2v4\n"
      "XNVkxRE9V/RgEBv4H5O/nBAiZCExlZV/RNStwMphkhhzRklsUKh/4R6sN+hfIGvL\n"
      "9r247yjkuTfKw9lkTYglhHJ6Fu6heW4fOMY2jFOHwhgw1YfuSZYPWc2WgC20XM3n\n"
      "Tye7LAOqwBq5lyYNXLGj39GLRHrX3k1a+Zy4uQe28yTgR+/9GmnkGDKkbk0ccV2v\n"
      "jAGF9Eer9y8rYD+6EqsMh4MycYDvHHSUn9wVHXP4T96s2i0+2aue9rGUKlmvZDXE\n"
      "z3BNEFmBAoGBAOpHjBbawp9ZTLfqT4TO0fnG/9nJsOIUSvqYWEHv6Iwit7M8f7kO\n"
      "PV/EZZ50ZZngjleMziQUiWocJQWScYf0Y6oM4regpnmtnQ0PNXZ07+3Jk38qEi5O\n"
      "dPdCPjW7ATpGGrDrjck3gy3W+2pBGPX/1Z1sdvPVMHP0R7s3cVw7Q29BAoGBANj2\n"
      "dR85s8Rpe+L2WdaUahZfo3PfoaIZxHHPdkVZfQWxAGOhTDOOy5pjkEJw3OZ4Z+L1\n"
      "8BSJF4OHuMdJ/AeCUm9WbZkS9Lj9VU3n0f/VaqqHrpNxcIB61kymYuwfaLfK5hdZ\n"
      "i/6MyBmTO/0Fn8oBT9zu0KKEFGVeegI2gZNA9tVXAoGBAN4W6z9zLBKBRhAHKFC+\n"
      "+yCsvDgKUgtPr3B14RzIwAuwkbJxBKc6T4YsmSSoNPS1glqOmZBsg4O6oIHyvHtk\n"
      "euJmiroGqiopIfuSUl00J4qkj5V6HXgWWh1Xh7/JwjUq5fyXoHaCkHFxHi8w/Oef\n"
      "GjK88trdW6xgg2t/+I+1gozBAoGBAI6kIkaAOkckjrWd/1yLQOAqypyw52RaioPE\n"
      "wN5BQmgOgdH+xsmS1RtQ4BQ+fYzslmhqBwiJRTNNdNdZNeB99tKKQQ4Fn21L6NyK\n"
      "T89iMXmiMM1xJGTj3aaRMbJJyCAlvnaVgeu+BQSDf0oPe4lkqWv4eqSOL5ahsZdo\n"
      "3HHCoUWLAoGAWol4/g+aV35C6dyCaI/qPnrxxKA7L/+QcTJ4bkXXL+3C57G0m11m\n"
      "gXSdff/CEvcyC7AP3NhNHQDqUWuI9UedCaGHY/kf3blu1/D0xL1OfGE1ZeBuoG9B\n"
      "Zvsf1uTb+JGPR5x+2BTXo4VZXzknBqM3S8XZDHSyQtKKxFekMYfNkkQ=\n"
      "-----END RSA PRIVATE KEY-----\n"),
    privkey_ec_(sec_doc_prop("certs/identity/test_participant_03_private_key.pem")),
    hello_(),
    world_(),
    empty_()
  {
    size_t hello_len = std::strlen("hello");
    hello_.length(static_cast<CORBA::ULong>(hello_len));
    std::memcpy(hello_.get_buffer(), "hello", hello_len);

    size_t world_len = std::strlen("world");
    world_.length(static_cast<CORBA::ULong>(world_len));
    std::memcpy(world_.get_buffer(), "world", world_len);
  }

  ~dds_DCPS_security_SSL_PrivateKey()
  {

  }

  Certificate ca_;
  Certificate ca_data_;
  Certificate pubkey_;
  Certificate pubkey_data_;
  Certificate pubkey_ec_;
  PrivateKey privkey_;
  PrivateKey privkey_data_;
  PrivateKey privkey_ec_;

  DDS::OctetSeq hello_;
  DDS::OctetSeq world_;
  const DDS::OctetSeq empty_;
};

}

TEST_F(dds_DCPS_security_SSL_PrivateKey, SignAndVerify_Success)
{
  std::vector<const DDS::OctetSeq*> sign_these;
  sign_these.push_back(&hello_);
  sign_these.push_back(&world_);

  DDS::OctetSeq tmp;
  privkey_.sign(sign_these, tmp);

  int verify_result = pubkey_.verify_signature(tmp, sign_these);

  ASSERT_EQ(0, verify_result);
}

TEST_F(dds_DCPS_security_SSL_PrivateKey, SignAndVerify_Data_Success)
{
  std::vector<const DDS::OctetSeq*> sign_these;
  sign_these.push_back(&hello_);
  sign_these.push_back(&world_);

  DDS::OctetSeq tmp;
  int sign_result = privkey_data_.sign(sign_these, tmp);

  ASSERT_EQ(0, sign_result);

  int verify_result = pubkey_data_.verify_signature(tmp, sign_these);

  ASSERT_EQ(0, verify_result);
}

TEST_F(dds_DCPS_security_SSL_PrivateKey, SignAndVerify_EC_Success)
{
  std::vector<const DDS::OctetSeq*> sign_these;
  sign_these.push_back(&hello_);
  sign_these.push_back(&world_);

  DDS::OctetSeq tmp;
  int sign_result = privkey_ec_.sign(sign_these, tmp);

  ASSERT_EQ(0, sign_result);

  int verify_result = pubkey_ec_.verify_signature(tmp, sign_these);

  ASSERT_EQ(0, verify_result);
}

TEST_F(dds_DCPS_security_SSL_PrivateKey, SignAndVerify_WrongData_Failure)
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

TEST_F(dds_DCPS_security_SSL_PrivateKey, SignAndVerify_DoesNotUseEmptyData)
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

#endif
