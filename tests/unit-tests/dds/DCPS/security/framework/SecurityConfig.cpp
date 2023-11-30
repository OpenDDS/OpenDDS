#ifdef OPENDDS_SECURITY

#include "../sec_doc.h"

#include <dds/DCPS/security/framework/SecurityConfig.h>
#include <dds/DCPS/security/framework/SecurityRegistry.h>
#include <dds/DCPS/security/framework/SecurityPluginInst_rch.h>
#if defined (ACE_AS_STATIC_LIBS)
#  include <dds/DCPS/security/BuiltInPlugins.h>
#  include <dds/DCPS/security/BuiltInPluginLoader.h>
#  include <ace/Dynamic_Service.h>
#endif

#include <ace/Configuration.h>
#include <ace/Configuration_Import_Export.h>

// These are just used to meet signature requirements for a test
#include <tests/Utils/gtestWrapper.h>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::Security;
using namespace testing;

namespace {

class dds_DCPS_security_framework_SecurityConfig : public Test
{
public:
   dds_DCPS_security_framework_SecurityConfig()
  {
  }

  ~dds_DCPS_security_framework_SecurityConfig()
  {
  }

  static void SetUpTestCase()
  {
#if defined(ACE_AS_STATIC_LIBS)
    BuiltInPluginLoader *loader =
    ACE_Dynamic_Service<BuiltInPluginLoader>::instance("OpenDDS_Security");
    if (loader == 0) {
      ACE_ERROR ((LM_ERROR,"could not locate BuiltIn plugin loader\n"));
    } else {
      loader->init(0,0);
    }
#endif
    cf_.open();
    ACE_Ini_ImpExp import(cf_);
    ASSERT_EQ(0, import.import_config(ACE_TEXT_CHAR_TO_TCHAR(
      sec_doc_path("../unit-tests/dds/DCPS/security/framework/test1.ini").c_str())));
    ASSERT_EQ(0, TheSecurityRegistry->load_security_configuration(cf_));
  }

  static ACE_Configuration_Heap cf_;
};

}

ACE_Configuration_Heap dds_DCPS_security_framework_SecurityConfig::cf_;

TEST_F(dds_DCPS_security_framework_SecurityConfig, UnknownSecurityConfig)
{
  // Ask it for a configuration that it does not have
  SecurityConfig_rch config = TheSecurityRegistry->create_config("TestConfig");
  ASSERT_TRUE(config.is_nil());
}

TEST_F(dds_DCPS_security_framework_SecurityConfig, TestConfig1)
{
  SecurityConfig_rch config = TheSecurityRegistry->create_config("test_config_1");
  ASSERT_FALSE(config.is_nil());

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

TEST_F(dds_DCPS_security_framework_SecurityConfig, TestConfig2)
{
  SecurityConfig_rch config = TheSecurityRegistry->create_config("test_config_2");
  ASSERT_FALSE(config.is_nil());

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

TEST_F(dds_DCPS_security_framework_SecurityConfig, TestConfig_NoProperties)
{
  SecurityConfig_rch config = TheSecurityRegistry->create_config("test_config_empty");
  ASSERT_FALSE(config.is_nil());

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

#endif
