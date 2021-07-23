/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifdef OPENDDS_SECURITY

#include <dds/DCPS/security/Authentication/LocalAuthCredentialData.h>
#include <dds/DCPS/security/TokenWriter.h>
#include <dds/DCPS/security/OpenSSL_init.h>

#include <gtest/gtest.h>

#include <iterator>
#include <fstream>
#include <cstring>

using DDS::Property_t;
using DDS::PropertySeq;
using OpenDDS::Security::TokenWriter;

struct LocalAuthCredentialDataTest : public ::testing::Test
{
  PropertySeq properties;
  OpenDDS::Security::LocalAuthCredentialData credential_data;

  LocalAuthCredentialDataTest()
  {

  }

  ~LocalAuthCredentialDataTest()
  {
  }

  void add_property(Property_t p) {
    PropertySeq& seq = properties;
    const CORBA::ULong len = seq.length();
    seq.length(len + 1);
    seq[len] = p;
  }
};


TEST_F(LocalAuthCredentialDataTest, LoadAccessPermissions_Success)
{
  std::string path("../security/permissions/permissions_test_participant_01_JoinDomain_signed.p7s");
  DDS::Security::SecurityException ex;

  DDS::OctetSeq expected_bytes;
  OpenDDS::DCPS::SequenceBackInsertIterator<DDS::OctetSeq> back_inserter(expected_bytes);
  std::ifstream expected_file(path.c_str(), std::ios::binary);

  std::copy((std::istreambuf_iterator<char>(expected_file)),
            std::istreambuf_iterator<char>(),
            back_inserter);

  *back_inserter = 0u;

  DDS::Security::PermissionsCredentialToken t;
  TokenWriter(t).add_property("dds.perm.cert", expected_bytes);
  credential_data.load_access_permissions(t, ex);

  const DDS::OctetSeq& access_bytes = credential_data.get_access_permissions();

  ASSERT_EQ(access_bytes.length(), expected_bytes.length());
  ASSERT_EQ(0, std::memcmp(access_bytes.get_buffer(),
                           expected_bytes.get_buffer(),
                           access_bytes.length()));
}

TEST_F(LocalAuthCredentialDataTest, LoadIdentityCa_Success)
{
  Property_t idca;
  idca.name = "dds.sec.auth.identity_ca";
  idca.value = "file:../security/certs/identity/identity_ca_cert.pem";
  idca.propagate = false;
  add_property(idca);

  DDS::Security::SecurityException ex;
  credential_data.load_credentials(properties, ex);
  ASSERT_TRUE(1); // TODO
}

TEST_F(LocalAuthCredentialDataTest, LoadPrivateKey_Success)
{
  Property_t pkey;
  pkey.name = "dds.sec.auth.private_key";
  pkey.value = "file:../security/certs/identity/test_participant_01_private_key.pem";
  pkey.propagate = false;
  add_property(pkey);

  Property_t pass;
  pass.name = "dds.sec.auth.password";
  pass.value = "";
  pass.propagate = false;
  add_property(pass);

  DDS::Security::SecurityException ex;
  credential_data.load_credentials(properties, ex);
  ASSERT_TRUE(1); // TODO
}

TEST_F(LocalAuthCredentialDataTest, LoadIdentityCert_Success)
{
  Property_t idcert;
  idcert.name = "dds.sec.auth.identity_certificate";
  idcert.value = "file:../security/certs/identity/test_participant_01_cert.pem";
  idcert.propagate = false;
  add_property(idcert);

  DDS::Security::SecurityException ex;
  credential_data.load_credentials(properties, ex);
  ASSERT_TRUE(1); // TODO
}

#endif
