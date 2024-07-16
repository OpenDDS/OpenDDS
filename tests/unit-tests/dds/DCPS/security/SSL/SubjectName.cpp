/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include <dds/OpenDDSConfigWrapper.h>

#if OPENDDS_CONFIG_SECURITY

#include "gtest/gtest.h"
#include "dds/DCPS/security/OpenSSL_init.h"
#include "dds/DCPS/security/SSL/SubjectName.h"

#include <iostream>
#include <fstream>

using namespace OpenDDS::Security::SSL;

TEST(dds_DCPS_security_SSL_SubjectName_Parser, EmptyDistinguishedName)
{
  Parser parser("");
  Parser::RDNVec store;
  ASSERT_TRUE(parser.parse(store));
  ASSERT_TRUE(store.empty());
}

TEST(dds_DCPS_security_SSL_SubjectName_Parser, EmptyRelativeDistinguishedName)
{
  Parser parser(",OU=Engineering,ST=MO");
  Parser::RDNVec store;
  ASSERT_FALSE(parser.parse(store));

  parser.reset("+CN=David Luis");
  ASSERT_FALSE(parser.parse(store));
}

TEST(dds_DCPS_security_SSL_SubjectName_Parser, InvalidAttributeType)
{
  // Must start with an alphabet
  Parser parser("1CN=David Luis");
  Parser::RDNVec store;
  ASSERT_FALSE(parser.parse(store));

  // Can't have characters other than alphabets, digits, hyphens
  parser.reset("C_N=David Luis");
  ASSERT_FALSE(parser.parse(store));

  // Can't be empty
  parser.reset("=David Luis");
  ASSERT_FALSE(parser.parse(store));
}

TEST(dds_DCPS_security_SSL_SubjectName_Parser, ExtraSpacesAllowed)
{
  Parser parser(" CN = David Luis   , ST =  \\ Missouri  +  L = Ladue");
  Parser::RDNVec store;
  ASSERT_TRUE(parser.parse(store));
  ASSERT_EQ((size_t)2, store.size());
  ASSERT_EQ((size_t)1, store[0].size());
  ASSERT_STREQ("CN", store[0].begin()->first.c_str());
  ASSERT_STREQ("David Luis", store[0].begin()->second.c_str());
  ASSERT_EQ((size_t)2, store[1].size());
  ASSERT_TRUE(store[1].count("ST") == 1);
  // Has a leading space.
  ASSERT_STREQ(" Missouri", store[1]["ST"].c_str());
  ASSERT_TRUE(store[1].count("L") == 1);
  ASSERT_STREQ("Ladue", store[1]["L"].c_str());
}

TEST(dds_DCPS_security_SSL_SubjectName_Parser, SimpleSuccess)
{
  Parser parser("CN=David Luis,OU=Engineering Department,O=Awesome Inc.,L=Creve Coeur,ST=MO,C=US");
  Parser::RDNVec store;
  ASSERT_TRUE(parser.parse(store));
  ASSERT_EQ((size_t)6, store.size());
  ASSERT_EQ((size_t)1, store[0].size());
  ASSERT_STREQ("CN", store[0].begin()->first.c_str());
  ASSERT_STREQ("David Luis", store[0].begin()->second.c_str());
  ASSERT_STREQ("OU", store[1].begin()->first.c_str());
  ASSERT_STREQ("Engineering Department", store[1].begin()->second.c_str());
  ASSERT_STREQ("O", store[2].begin()->first.c_str());
  ASSERT_STREQ("Awesome Inc.", store[2].begin()->second.c_str());
  ASSERT_STREQ("L", store[3].begin()->first.c_str());
  ASSERT_STREQ("Creve Coeur", store[3].begin()->second.c_str());
  ASSERT_STREQ("ST", store[4].begin()->first.c_str());
  ASSERT_STREQ("MO", store[4].begin()->second.c_str());
  ASSERT_STREQ("C", store[5].begin()->first.c_str());
  ASSERT_STREQ("US", store[5].begin()->second.c_str());
}

TEST(dds_DCPS_security_SSL_SubjectName_Parser, SimpleEscapeSuccess)
{
  Parser parser("CN= David Luis\\, Fernandez ,OU=Engineering Department\\;,O=Black \\+ White Inc.,ST=MO,C=US");
  Parser::RDNVec store;
  ASSERT_TRUE(parser.parse(store));
  ASSERT_EQ((size_t)5, store.size());
  ASSERT_STREQ("CN", store[0].begin()->first.c_str());
  ASSERT_STREQ("David Luis, Fernandez", store[0].begin()->second.c_str());
  ASSERT_STREQ("OU", store[1].begin()->first.c_str());
  ASSERT_STREQ("Engineering Department;", store[1].begin()->second.c_str());
  ASSERT_STREQ("O", store[2].begin()->first.c_str());
  ASSERT_STREQ("Black + White Inc.", store[2].begin()->second.c_str());
}

TEST(dds_DCPS_security_SSL_SubjectName_Parser, MultiValue)
{
  Parser parser("CN=\\ David Luis+OU=Engineering \\+ Managing,O=Apple Inc.");
  Parser::RDNVec store;
  ASSERT_TRUE(parser.parse(store));
  ASSERT_EQ((size_t)2, store.size());
  ASSERT_TRUE(store[0].count("CN") == 1);
  ASSERT_STREQ(" David Luis", store[0]["CN"].c_str());
  ASSERT_TRUE(store[0].count("OU") == 1);
  ASSERT_STREQ("Engineering + Managing", store[0]["OU"].c_str());
  ASSERT_STREQ("O", store[1].begin()->first.c_str());
  ASSERT_STREQ("Apple Inc.", store[1].begin()->second.c_str());
}

namespace {

class dds_DCPS_security_SSL_SubjectName : public ::testing::Test {
public:
  dds_DCPS_security_SSL_SubjectName()
   : sn_ldap_single_("CN=DDS Shapes Demo")
   , sn_ldap_nom_("emailAddress=cto@acme.com,CN=DDS Shapes Demo,OU=CTO Office,O=ACME Inc.,L=Sunnyvale,ST=CA,C=US")
   , sn_dce_nom_("/C=US/ST=MO/O=Object Computing/CN=CN_TEST_DDS-SECURITY_OCI_OPENDDS/emailAddress=support@objectcomputing.com")
   , sn_ldap_cmp_("emailAddress=joe@shmoe.com,CN=Everything Goes,OU=Messy Art Gallery,O=Daytime Productions,L=Ladue,ST=MO,C=US")
   , sn_dce_cmp_("/C=US/ST=MO/L=Ladue/O=Daytime Productions/OU=Messy Art Gallery/CN=Everything Goes/emailAddress=joe@shmoe.com")
  {
  }

  ~dds_DCPS_security_SSL_SubjectName()
  {
  }

  SubjectName sn_ldap_single_;
  SubjectName sn_ldap_nom_;
  SubjectName sn_dce_nom_;
  SubjectName sn_ldap_cmp_;
  SubjectName sn_dce_cmp_;
};

}

TEST_F(dds_DCPS_security_SSL_SubjectName, Parse_LDAPv3_Single_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("CN=DDS Shapes Demo"));
  ASSERT_EQ(sn, sn_ldap_single_);
  ASSERT_EQ(0, sn.parse("CN = DDS Shapes Demo "));
  ASSERT_EQ(sn, sn_ldap_single_);
}

TEST_F(dds_DCPS_security_SSL_SubjectName, Parse_LDAPv3_Single_Failure)
{
  SubjectName sn;
  ASSERT_EQ(1, sn.parse("  =DDS Shapes Demos"));
  ASSERT_EQ(0, sn.parse("CN = DDS Shapes Demosss"));
  ASSERT_NE(sn, sn_ldap_single_);
}

TEST_F(dds_DCPS_security_SSL_SubjectName, Parse_LDAPv3_Nominal_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("emailAddress=cto@acme.com,CN= DDS Shapes Demo,OU= CTO Office,O=ACME Inc.,L=Sunnyvale,ST=CA,C=US"));
  ASSERT_EQ(sn, sn_ldap_nom_);
}

TEST_F(dds_DCPS_security_SSL_SubjectName, Parse_DCE_Nominal_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("/C=US/ST=MO/O=Object Computing/CN=CN_TEST_DDS-SECURITY_OCI_OPENDDS/emailAddress=support@objectcomputing.com"));
  ASSERT_EQ(sn, sn_dce_nom_);
}

TEST_F(dds_DCPS_security_SSL_SubjectName, Parse_DCE_Nominal_NoTrim_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("/C=US/ST=MO/O=Object Computing/CN=CN_TEST_DDS-SECURITY_OCI_OPENDDS/emailAddress=support@objectcomputing.com"));
  ASSERT_EQ(sn, sn_dce_nom_);
}

TEST_F(dds_DCPS_security_SSL_SubjectName, Parse_DCE_Nominal_ExtraTrim_Success)
{
  SubjectName sn;
  ASSERT_EQ(0, sn.parse("/ C = US / ST = MO / O = Object Computing / CN = CN_TEST_DDS-SECURITY_OCI_OPENDDS / emailAddress = support@objectcomputing.com "));
  ASSERT_EQ(sn, sn_dce_nom_);
}

TEST_F(dds_DCPS_security_SSL_SubjectName, LDAP_DCE_CMP)
{
  SubjectName sn_ldap_cmp_backwards("C=US,ST=MO,L=Ladue,O=Daytime Productions,OU=Messy Art Gallery,CN=Everything Goes,emailAddress=joe@shmoe.com");
  SubjectName sn_dce_cmp_backwards("/emailAddress=joe@shmoe.com/CN=Everything Goes/OU=Messy Art Gallery/O=Daytime Productions/L=Ladue/ST=MO/C=US");

  // The importance of this test is that attribute order matters (so we shouldn't ignore it), but that DCE format intentionally lists attributes backwards
  ASSERT_EQ(sn_ldap_cmp_, sn_dce_cmp_backwards);
  ASSERT_NE(sn_ldap_cmp_, sn_ldap_cmp_backwards);
  ASSERT_NE(sn_ldap_cmp_, sn_dce_cmp_);
  ASSERT_EQ(sn_dce_cmp_, sn_ldap_cmp_backwards);
  ASSERT_NE(sn_dce_cmp_, sn_dce_cmp_backwards);
}

#endif
