/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "gtest/gtest.h"
#include "dds/DCPS/security/SSL/PrivateKey.h"
#include "dds/DCPS/security/SSL/Certificate.h"
#include <cstring>

using namespace OpenDDS::Security::SSL;

class PrivateKeyTest : public ::testing::Test
{
public:
  PrivateKeyTest() :
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
	pubkey_("file:../certs/mock_participant_1/opendds_participant_cert.pem"),
	pubkey_data_("data:,-----BEGIN CERTIFICATE-----\n"
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
		"-----END CERTIFICATE----- "),
    pubkey_ec_("file:../certs/ecdsa/opendds_participant_cert.pem"),
    privkey_("file:../certs/mock_participant_1/opendds_participant_private_key.pem"),
	privkey_data_("data:,-----BEGIN RSA PRIVATE KEY-----\n"
		"MIIEpAIBAAKCAQEAxSDtEZPm + 8FwxIT7Xn / w6ypoGKQ0DmU / 1EGyCaOKF6afeV0w\n"
		"843VGvySfx7OHGS / D8vVHCkPMuV1GLG47x1jjgeFLKLlwiLcyx9LjK6Dp3LrY2HT\n"
		"RluU4E1lTuKly / PSw5iF2eXNBDSoC7OKrnil7q4CtNo6BLTxpvpMXOCDFOkVrQVP\n"
		"S2lfoouN0 / AOMwX5PbLYzxlQmcKh + mZZ + u6wzio2Hf0CtxI2wPpappjIDTuy6XmA\n"
		"HOReyl5jjUavsaUsVSq5pO0REf0RWJ / L6m92ddghwFz0bp7PbWDa2A3ZIYUnqROB\n"
		"xzm3iAsg0 + v2nfEY5OMdH0IOPV + waoJa6KKrZwIDAQABAoIBAEF5ESRAcnx1oEW1\n"
		"QWP + rH7 / A7oDd8K8QDn4Y / Ctn6UBU4BUwnhySDCBaQrNqXlmwMq3UZ3byU + HRKAi\n"
		"DyFkKne26bWMmTEksR7J0ybzo4iPM707dZetvUHOkvJjdtvJrNwIIZTgrXpTHulc\n"
		"BZizO7GNnBqDTLfZfVWPVqOWchMUTW19GKdpIesYEbbmbcJQqV9QdUKOJ041j / Lz\n"
		"F9jagbL7AXxE + y670BPHBicsH / s9PYKv1XcgfX + G + KyAVNNPgqr5AgLaabbYcb6o\n"
		"Z + 6q3j0HIxTIkA / Z3HZ / HEyjjdjBPn47BSRlA6RTXyHQXwBOx2v4I2cs / HjEtmQQ\n"
		"xlklK7ECgYEA + b5aUs872qiA / aqqSuXI + LbvwQrVzs3qgI / 4jdH8ObMmsTeNyAc4\n"
		"pEEPQOPrS5u + Bk9UgmtRAuJBF1dS / F952dmqIysv / B9xCa61Qm3hSF / Bp7jXmMoa\n"
		"HQBJcyA0JEtMrdNXP / gog3u42jmH + uuv01tC269vdgdEbF84aXMOapkCgYEAyhEl\n"
		"EB6uVTyJpGPvLti6ye64B + OmE7Kwb4CS / +ufG + 3a80LqubJL1PQmfn2OM0M4iOKT\n"
		"cqIKOyb + nHAuIvu++xfVERKyOEYzWHvMwF + 4Twqm2jwSqtL2kAJsxLDyz2yzVuA4\n"
		"V5jS8MiOBxSFlhj0NJrWX//vuAEG/POxyYsthf8CgYEAolNUWAC0kbH2bWpRV281\n"
		"ils1SI7nW4zBwCBT2LJEs6g7HPLT59Cxwyk6Zd3 + oN0wzN8hOcfRFwPCdQ5gNQNF\n"
		"qxBsIoG + paw2B6oTzIKo7Ca5M2 / USk4KXRFDrF2hJnn + 8 / iq / Dwq8RMomkbMmI46\n"
		"cTEfKrVDyD5 + / cWCYB / VnfkCgYBqqkA5USjPn8Q8vfANd6SqYdRNfdM2RLY0Ndfj\n"
		"NlroIFfa37EOU1sKT9NeJCMDVnGqeIhDE9x4uy3eIK2KFAANhdgYShk / 8Xa7N6au\n"
		"yhh9yO6o9tsXx4MWI + GMtqeF7SiCLJwxSV / YcNXgUOnvgL6wYifVx0GgjRJGRtHL\n"
		"xSdDuwKBgQCqYKLAsI5nDKUoyUunoaCi51RZFlhrWO4fLA / tCGugmx4 / et06x1s4\n"
		"9j1LwufwEDDgdY5M34PUeMjA980b8OiESXE2IfoaLiZEqANI6vQ6dRl / KMfGWUY6\n"
		"EMUPFMlvTkIa38Nyutkth / P / rZZiaBapEkb / mtclm1ra4nDP0o1TqA == \n"
		"-----END RSA PRIVATE KEY----- "),
    privkey_ec_("file:../certs/ecdsa/private_key.pem"),
    hello_(),
    world_(),
    empty_()
  {
    size_t hello_len = std::strlen("hello");
    hello_.length(hello_len);
    std::memcpy(hello_.get_buffer(), "hello", hello_len);

    size_t world_len = std::strlen("world");
    world_.length(world_len);
    std::memcpy(world_.get_buffer(), "world", world_len);
  }

  ~PrivateKeyTest()
  {

  }

  Certificate ca_;
  Certificate ca_data_;
  Certificate pubkey_ec_;
  Certificate pubkey_;
  Certificate pubkey_data_;
  PrivateKey privkey_;
  PrivateKey privkey_data_;
  PrivateKey privkey_ec_;
//  PrivateKey pubkey_ec_;

  DDS::OctetSeq hello_;
  DDS::OctetSeq world_;
  const DDS::OctetSeq empty_;
};

TEST_F(PrivateKeyTest, SignAndVerify_Success)
{
  std::vector<const DDS::OctetSeq*> sign_these;
  sign_these.push_back(&hello_);
  sign_these.push_back(&world_);

  DDS::OctetSeq tmp;
  privkey_.sign(sign_these, tmp);

  int verify_result = pubkey_.verify_signature(tmp, sign_these);

  ASSERT_EQ(0, verify_result);
}

TEST_F(PrivateKeyTest, SignAndVerify_Data_Success)
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

TEST_F(PrivateKeyTest, SignAndVerify_EC_Success)
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

TEST_F(PrivateKeyTest, SignAndVerify_WrongData_Failure)
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

TEST_F(PrivateKeyTest, SignAndVerify_DoesNotUseEmptyData)
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
