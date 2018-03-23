/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "gtest/gtest.h"
#include "dds/DCPS/security/Authentication/LocalCredentialData.h"
#include <iterator>
#include <fstream>
#include <cstring>

using DDS::Property_t;
using DDS::PropertySeq;

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
    size_t len = seq.length();
    seq.length(len + 1);
    seq[len] = p;
  }
};


TEST_F(LocalAuthCredentialDataTest, LoadAccessPermissions_Success)
{
  std::string path("permissions/Permissions_JoinDomain_OCI.p7s");

  Property_t perms;
  perms.name = "dds.sec.access.permissions";
  perms.value = ("file:" + path).c_str();
  perms.propagate = false;
  add_property(perms);

  credential_data.load(properties);
  std::ifstream expected_file(path.c_str());

  std::vector<char> expected_bytes((std::istreambuf_iterator<char>(expected_file)),
                                   std::istreambuf_iterator<char>());

  const DDS::OctetSeq& access_bytes = credential_data.get_access_permissions();

  ASSERT_EQ(0, std::memcmp(access_bytes.get_buffer(),
                           reinterpret_cast<const CORBA::Octet*>(&expected_bytes[0]),
                           access_bytes.length()));
}

TEST_F(LocalAuthCredentialDataTest, LoadIdentityCa_Success)
{
  Property_t idca;
  idca.name = "dds.sec.auth.identity_ca";
  idca.value = "file:certs/opendds_identity_ca_cert.pem";
  idca.propagate = false;
  add_property(idca);

  credential_data.load(properties);
  ASSERT_TRUE(1); // TODO
}

TEST_F(LocalAuthCredentialDataTest, LoadPrivateKey_Success)
{
  Property_t pkey;
  pkey.name = "dds.sec.auth.private_key";
  pkey.value = "file:certs/mock_participant_1/opendds_participant_private_key.pem";
  pkey.propagate = false;
  add_property(pkey);

  Property_t pass;
  pass.name = "dds.sec.auth.password";
  pass.value = "";
  pass.propagate = false;
  add_property(pass);

  credential_data.load(properties);
  ASSERT_TRUE(1); // TODO
}

TEST_F(LocalAuthCredentialDataTest, LoadIdentityCert_Success)
{
  Property_t idcert;
  idcert.name = "dds.sec.auth.identity_certificate";
  idcert.value = "file:certs/mock_participant_1/opendds_participant_cert.pem";
  idcert.propagate = false;
  add_property(idcert);

  credential_data.load(properties);
  ASSERT_TRUE(1); // TODO
}
