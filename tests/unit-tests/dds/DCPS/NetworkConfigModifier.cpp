#include <dds/DCPS/NetworkConfigModifier.h>
#include <gtest/gtest.h>

#ifdef OPENDDS_NETWORK_CONFIG_MODIFIER

using namespace OpenDDS::DCPS;

TEST(network_config_modifier_test, add_remove)
{
  NetworkConfigModifier mod;
  mod.add_interface("eth0");
  mod.add_address("eth0", ACE_INET_Addr(42));
  mod.remove_address("eth0", ACE_INET_Addr(42));
  mod.remove_interface("eth0");
}

#endif
