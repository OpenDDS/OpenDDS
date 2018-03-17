/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "gtest/gtest.h"
#include "dds/DCPS/security/Authentication/LocalCredentialData.h"

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
  Property_t perms;
  perms.name = "dds.sec.access.permissions";
  perms.value = "file:permissions/Permissions_JoinDomain_OCI.p7s";
  perms.propagate = false;
  add_property(perms);

  credential_data.load(properties);
  ASSERT_TRUE(1); // TODO
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
