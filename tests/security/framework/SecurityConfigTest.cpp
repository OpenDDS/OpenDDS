

#include "dds/DCPS/security/framework/SecurityConfig.h"
#include "dds/DCPS/security/framework/SecurityRegistry.h"
#include "dds/DCPS/security/framework/SecurityPluginInst_rch.h"
#include "ace/Configuration.h"
#include "ace/Configuration_Import_Export.h"

// These are just used to meet signature requirements for a test
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace OpenDDS::DCPS;
using namespace OpenDDS::Security;
using namespace testing;


class SecurityConfigTest : public Test
{
public:
   SecurityConfigTest()
  {
  }

  ~SecurityConfigTest()
  {
  }

  static void SetUpTestCase()
  {
    cf_.open();
    ACE_Ini_ImpExp import(cf_);
    ASSERT_EQ(0, import.import_config(ACE_TEXT("test1.ini")));
    ASSERT_EQ(0, TheSecurityRegistry->load_security_configuration(cf_));
  }

  static ACE_Configuration_Heap cf_;
};

ACE_Configuration_Heap SecurityConfigTest::cf_;

TEST_F(SecurityConfigTest, UnknownSecurityConfig)
{
        // Ask it for a configuration that it does not have
        SecurityConfig_rch config = TheSecurityRegistry->create_config("TestConfig");
        EXPECT_TRUE(config.is_nil());
}

TEST_F(SecurityConfigTest, TestConfig1)
{
        SecurityConfig_rch config = TheSecurityRegistry->create_config("test_config_1");
        EXPECT_FALSE(config.is_nil());

        // Interface check
        EXPECT_TRUE(config->get_access_control());
        EXPECT_TRUE(config->get_authentication());
        EXPECT_TRUE(config->get_crypto_key_exchange());
        EXPECT_TRUE(config->get_crypto_key_factory());
        EXPECT_TRUE(config->get_crypto_transform());

        //Check the properties
        DDS::Security::PropertyQosPolicy property_data;
        config->get_properties(property_data);
        EXPECT_EQ(0U, property_data.binary_value.length());
        ASSERT_EQ(2U, property_data.value.length());

        EXPECT_STREQ("prop1", property_data.value[0].name);
        EXPECT_STREQ("prop1_value", property_data.value[0].value);
        EXPECT_STREQ("prop2", property_data.value[1].name);
        EXPECT_STREQ("prop2_value", property_data.value[1].value);
}

TEST_F(SecurityConfigTest, TestConfig2)
{
        SecurityConfig_rch config = TheSecurityRegistry->create_config("test_config_2");
        EXPECT_FALSE(config.is_nil());

        // Interface check
        EXPECT_TRUE(config->get_access_control());
        EXPECT_TRUE(config->get_authentication());
        EXPECT_TRUE(config->get_crypto_key_exchange());
        EXPECT_TRUE(config->get_crypto_key_factory());
        EXPECT_TRUE(config->get_crypto_transform());

        //Check the properties
        DDS::Security::PropertyQosPolicy property_data;
        config->get_properties(property_data);
        EXPECT_EQ(0U, property_data.binary_value.length());
        ASSERT_EQ(3U, property_data.value.length());

        EXPECT_STREQ("prop1", property_data.value[0].name);
        EXPECT_STREQ("A", property_data.value[0].value);
        EXPECT_STREQ("prop2", property_data.value[1].name);
        EXPECT_STREQ("B", property_data.value[1].value);
        EXPECT_STREQ("propX", property_data.value[2].name);
        EXPECT_STREQ("C", property_data.value[2].value);
}

TEST_F(SecurityConfigTest, TestConfig_NoProperties)
{
        SecurityConfig_rch config = TheSecurityRegistry->create_config("test_config_empty");
        EXPECT_FALSE(config.is_nil());

        // Interface check
        EXPECT_TRUE(config->get_access_control());
        EXPECT_TRUE(config->get_authentication());
        EXPECT_TRUE(config->get_crypto_key_exchange());
        EXPECT_TRUE(config->get_crypto_key_factory());
        EXPECT_TRUE(config->get_crypto_transform());

        //Check the properties
        DDS::Security::PropertyQosPolicy property_data;
        config->get_properties(property_data);
        EXPECT_EQ(0U, property_data.binary_value.length());
        EXPECT_EQ(0U, property_data.value.length());
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
