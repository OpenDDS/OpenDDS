/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifdef OPENDDS_SECURITY

#include "gtest/gtest.h"
#include "dds/DCPS/security/OpenSSL_init.h"
#include "dds/DCPS/security/SSL/SubjectName.h"

#include <iostream>
#include <fstream>

using namespace OpenDDS::Security::SSL;

class SubjectNameTest : public ::testing::Test
{
public:
  SubjectNameTest()
   : sn_ldap_single_("CN=DDS Shapes Demo")
   , sn_ldap_nom_("emailAddress=cto@acme.com, CN=DDS Shapes Demo, OU=CTO Office, O=ACME Inc., L=Sunnyvale, ST=CA, C=US")
   , sn_dce_nom_("/C=US/ST=MO/O=Object Computing/CN=CN_TEST_DDS-SECURITY_OCI_OPENDDS/emailAddress=support@objectcomputing.com")
   , sn_ldap_cmp_("emailAddress=joe@shmoe.com, CN=Everything Goes, OU=Messy Art Gallery, O=Daytime Productions, L=Ladue, ST=MO, C=US")
   , sn_dce_cmp_("/C=US/ST=MO/L=Ladue/O=Daytime Productions/OU=Messy Art Gallery/CN=Everything Goes/emailAddress=joe@shmoe.com")
  {
  }

  ~SubjectNameTest()
  {
  }

  SubjectName sn_ldap_single_;
  SubjectName sn_ldap_nom_;
  SubjectName sn_dce_nom_;
  SubjectName sn_ldap_cmp_;
  SubjectName sn_dce_cmp_;
};

TEST_F(SubjectNameTest, Parse_LDAPv3_Single_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("CN = DDS Shapes Demo"));
  ASSERT_EQ(sn, sn_ldap_single_);
}

TEST_F(SubjectNameTest, Parse_LDAPv3_Single_NoTrim_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("CN=DDS Shapes Demo"));
  ASSERT_EQ(sn == sn_ldap_single_, true);
}

TEST_F(SubjectNameTest, Parse_LDAPv3_Single_ExtraTrim_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("  CN  =  DDS Shapes Demo "));
  ASSERT_EQ(sn == sn_ldap_single_, true);
}

TEST_F(SubjectNameTest, Parse_LDAPv3_Nominal_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("emailAddress=cto@acme.com, CN=DDS Shapes Demo, OU=CTO Office, O=ACME Inc., L=Sunnyvale, ST=CA, C=US"));
  ASSERT_EQ(sn, sn_ldap_nom_);
}

TEST_F(SubjectNameTest, Parse_LDAPv3_Nominal_NoTrim_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("emailAddress=cto@acme.com,CN=DDS Shapes Demo,OU=CTO Office,O=ACME Inc.,L=Sunnyvale,ST=CA,C=US"));
  ASSERT_EQ(sn, sn_ldap_nom_);
}

TEST_F(SubjectNameTest, Parse_LDAPv3_Nominal_ExtraTrim_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("  emailAddress =  cto@acme.com , CN  = DDS Shapes Demo , OU = CTO Office , O = ACME Inc. , L=Sunnyvale  , ST  = CA , C = US  "));
  ASSERT_EQ(sn, sn_ldap_nom_);
}

TEST_F(SubjectNameTest, Parse_DCE_Nominal_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("/C=US/ST=MO/O=Object Computing/CN=CN_TEST_DDS-SECURITY_OCI_OPENDDS/emailAddress=support@objectcomputing.com"));
  ASSERT_EQ(sn, sn_dce_nom_);
}

TEST_F(SubjectNameTest, Parse_DCE_Nominal_NoTrim_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("/C=US/ST=MO/O=Object Computing/CN=CN_TEST_DDS-SECURITY_OCI_OPENDDS/emailAddress=support@objectcomputing.com"));
  ASSERT_EQ(sn, sn_dce_nom_);
}

TEST_F(SubjectNameTest, Parse_DCE_Nominal_ExtraTrim_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("/ C = US / ST = MO / O = Object Computing / CN = CN_TEST_DDS-SECURITY_OCI_OPENDDS / emailAddress = support@objectcomputing.com "));
  ASSERT_EQ(sn, sn_dce_nom_);
}

TEST_F(SubjectNameTest, LDAP_DCE_CMP)
{
  SubjectName sn_ldap_cmp_backwards("C=US, ST=MO, L=Ladue, O=Daytime Productions, OU=Messy Art Gallery, CN=Everything Goes, emailAddress=joe@shmoe.com");
  SubjectName sn_dce_cmp_backwards("/emailAddress=joe@shmoe.com/CN=Everything Goes/OU=Messy Art Gallery/O=Daytime Productions/L=Ladue/ST=MO/C=US");

  ASSERT_EQ(sn_ldap_cmp_, sn_dce_cmp_);
  ASSERT_EQ(sn_ldap_cmp_backwards, sn_dce_cmp_backwards);

  /*
  // The importance of this test is that attribute order matters (so we shouldn't ignore it), but that DCE format intentionally lists attributes backwards

  // Enable these if we get strict about attribute ordering
  ASSERT_NE(sn_ldap_cmp_, sn_ldap_cmp_backwards);
  ASSERT_NE(sn_ldap_cmp_, sn_dce_cmp_backwards);
  ASSERT_NE(sn_dce_cmp_, sn_ldap_cmp_backwards);
  ASSERT_NE(sn_dce_cmp_, sn_dce_cmp_backwards);
  */
}

#endif
