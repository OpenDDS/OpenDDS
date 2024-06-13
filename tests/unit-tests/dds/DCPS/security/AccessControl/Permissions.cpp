#include <dds/OpenDDSConfigWrapper.h>

#if OPENDDS_CONFIG_SECURITY

#include <dds/DCPS/security/AccessControl/Permissions.h>
#include <dds/DCPS/debug.h>

#include <gtest/gtest.h>

using namespace OpenDDS::Security;

TEST(dds_DCPS_security_AccessControl_Permissions, Permissions_Validity_t_ctor)
{
  {
    Permissions::Validity_t v;
    EXPECT_EQ(v.not_before, 0);
    EXPECT_EQ(v.not_after, 0);
  }
  {
    Permissions::Validity_t v(1,2);
    EXPECT_EQ(v.not_before, 1);
    EXPECT_EQ(v.not_after, 2);
  }
  {
    Permissions::Validity_t v1(1,2);
    Permissions::Validity_t v2(1,2);
    Permissions::Validity_t v3(3,4);
    EXPECT_EQ(v1, v2);
    EXPECT_NE(v1, v3);
    EXPECT_NE(v2, v3);
  }
}

TEST(dds_DCPS_security_AccessControl_Permissions, Permissions_Action_valid)
{
  Permissions::Action a;
  a.validity.not_before = 1;
  a.validity.not_after = 3;
  EXPECT_FALSE(a.valid(0));
  EXPECT_TRUE(a.valid(1));
  EXPECT_TRUE(a.valid(2));
  EXPECT_TRUE(a.valid(3));
  EXPECT_FALSE(a.valid(4));
}

TEST(dds_DCPS_security_AccessControl_Permissions, Permissions_load)
{
  Permissions p;
  OpenDDS::Security::SSL::SignedDocument sd;
  sd.content(
             "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
             "<dds xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://www.omg.org/spec/DDS-SECURITY/20170901/omg_shared_ca_permissions.xsd\">"
             "  <permissions>"
             "    <grant name=\"TheGrant\">"
             "      <subject_name>CN=Ozzie Ozmann,O=Internet Widgits Pty Ltd,ST=Some-State,C=AU</subject_name>"
             "      <validity>"
             "        <not_before>2015-09-15T01:00:00</not_before>"
             "        <not_after>2025-09-15T01:00:00</not_after>"
             "      </validity>"
             "      <allow_rule>"
             "        <domains>"
             "          <id>0</id>"
             "        </domains>"
             "        <publish>"
             "          <topics>"
             "            <topic>*</topic>"
             "          </topics>"
             "          <validity>"
             "            <not_before>2015-10-15T01:00:00</not_before>"
             "            <not_after>2025-10-15T01:00:00</not_after>"
             "          </validity>"
             "        </publish>"
             "      </allow_rule>"
             "      <default>DENY</default>"
             "    </grant>"
             "  </permissions>"
             "</dds>"
             );
  EXPECT_EQ(p.load(sd), 0);
  ASSERT_EQ(p.grants_.size(), 1U);
  Permissions::Grant_rch grant = p.grants_[0];
  EXPECT_EQ(grant->name, "TheGrant");
  EXPECT_EQ(grant->subject, OpenDDS::Security::SSL::SubjectName("CN=Ozzie Ozmann,O=Internet Widgits Pty Ltd,ST=Some-State,C=AU"));
  EXPECT_EQ(grant->validity, Permissions::Validity_t(1442278800, 1757898000));
  EXPECT_EQ(grant->default_permission, Permissions::DENY);
  ASSERT_EQ(grant->rules.size(), 1U);
  const Permissions::Rule& rule = grant->rules[0];
  EXPECT_EQ(rule.ad_type, Permissions::ALLOW);
  ASSERT_EQ(rule.domains.size(), 1U);
  EXPECT_TRUE(rule.domains.has(0));
  EXPECT_FALSE(rule.domains.has(1));
  ASSERT_EQ(rule.actions.size(), 1U);
  const Permissions::Action& action = rule.actions[0];
  EXPECT_EQ(action.ps_type, Permissions::PUBLISH);
  ASSERT_EQ(action.topics.size(), 1U);
  EXPECT_EQ(action.topics[0], "*");
  EXPECT_TRUE(action.partitions.empty());
  EXPECT_EQ(action.validity, Permissions::Validity_t(1444870800, 1760490000));
}

#endif
