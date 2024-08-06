#include <tests/Utils/GtestRc.h>

#include <dds/DCPS/RTPS/RtpsDiscoveryConfig.h>

using namespace OpenDDS::RTPS;
using namespace OpenDDS::DCPS;

namespace {
  struct AddressTest {
    RcHandle<ConfigStoreImpl> store;
    RtpsDiscoveryConfig rtps;
    NetworkAddress addr;
    bool fixed;

    AddressTest()
    : store(make_rch<ConfigStoreImpl>(TheServiceParticipant->config_topic()))
    , rtps("ADDRESS_TEST")
    , addr()
    , fixed(false)
    {
      store->unset_section(rtps.config_prefix());
    }

    ~AddressTest()
    {
      store->unset_section(rtps.config_prefix());
    }
  };

  const NetworkAddress fake_ipv4_addr(1234, "1.2.3.4");
#ifdef ACE_HAS_IPV6
  const NetworkAddress fake_ipv6_addr(1234, "::1:2:3:4");
#endif
}

TEST(dds_DCPS_RTPS_RtpsDiscoveryConfig, spdp_unicast_address)
{
  const char* const default_addr = "0.0.0.0";

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7660, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps.spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7660, default_addr));
    EXPECT_FALSE(t.fixed);
  }


  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_System);
    ASSERT_TRUE(t.rtps.spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(0, default_addr));
    EXPECT_TRUE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps.spdp_unicast_address(t.addr, t.fixed, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7666, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_Probe);
    t.rtps.pb(7500);
    t.rtps.dg(260);
    t.rtps.pg(3);
    t.rtps.d1(8);
    ASSERT_TRUE(t.rtps.spdp_unicast_address(t.addr, t.fixed, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_Probe);
    t.rtps.pb(9999);
    t.rtps.dg(9999);
    t.rtps.pg(9999);
    t.rtps.d1(9999);
    LogRestore lr;
    log_level.set(LogLevel::None);
    ASSERT_TRUE(t.rtps.spdp_unicast_address(t.addr, t.fixed, 9999, 9999));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(29664, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_local_address(fake_ipv4_addr);
    ASSERT_TRUE(t.rtps.spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, fake_ipv4_addr);
    EXPECT_TRUE(t.fixed);
  }
}

TEST(dds_DCPS_RTPS_RtpsDiscoveryConfig, spdp_multicast_address)
{
  const char* const default_addr = "239.255.0.1";

  DDS::UInt16 d0 = 0;

#if !defined ACE_LACKS_GETENV && !defined ACE_LACKS_ENV
  const char* const from_env = ACE_OS::getenv("OPENDDS_RTPS_DEFAULT_D0");
  if (from_env) {
    d0 = static_cast<DDS::UInt16>(std::atoi(from_env));
  }
#endif

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.spdp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7650 + d0, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.spdp_multicast_address(t.addr, 2));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7900 + d0, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.pb(7500);
    t.rtps.dg(260);
    t.rtps.d0(17);
    ASSERT_TRUE(t.rtps.spdp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.pb(9999);
    t.rtps.dg(9999);
    t.rtps.d0(9999);
    LogRestore lr;
    log_level.set(LogLevel::None);
    ASSERT_TRUE(t.rtps.spdp_multicast_address(t.addr, 9999));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(57599, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_multicast_address(fake_ipv4_addr);
    ASSERT_TRUE(t.rtps.spdp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, fake_ipv4_addr);
    EXPECT_FALSE(t.fixed);
  }
}


#ifdef ACE_HAS_IPV6
TEST(dds_DCPS_RTPS_RtpsDiscoveryConfig, ipv6_spdp_unicast_address)
{
  const char* const default_addr = "::";

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.ipv6_spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7660, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps.ipv6_spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7660, default_addr));
    EXPECT_FALSE(t.fixed);
  }


  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_System);
    ASSERT_TRUE(t.rtps.ipv6_spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress());
    EXPECT_TRUE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps.ipv6_spdp_unicast_address(t.addr, t.fixed, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7666, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_Probe);
    t.rtps.pb(7500);
    t.rtps.dg(260);
    t.rtps.pg(3);
    t.rtps.d1(8);
    ASSERT_TRUE(t.rtps.ipv6_spdp_unicast_address(t.addr, t.fixed, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_Probe);
    t.rtps.pb(9999);
    t.rtps.dg(9999);
    t.rtps.pg(9999);
    t.rtps.d1(9999);
    LogRestore lr;
    log_level.set(LogLevel::None);
    ASSERT_TRUE(t.rtps.ipv6_spdp_unicast_address(t.addr, t.fixed, 9999, 9999));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(29664, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.ipv6_spdp_local_address(fake_ipv6_addr);
    ASSERT_TRUE(t.rtps.ipv6_spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, fake_ipv6_addr);
    EXPECT_TRUE(t.fixed);
  }
}

TEST(dds_DCPS_RTPS_RtpsDiscoveryConfig, ipv6_spdp_multicast_address)
{
  const char* const default_addr = "ff03::1";

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.ipv6_spdp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7650, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.ipv6_spdp_multicast_address(t.addr, 2));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7900, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.pb(7500);
    t.rtps.dg(260);
    t.rtps.d0(17);
    ASSERT_TRUE(t.rtps.ipv6_spdp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.pb(9999);
    t.rtps.dg(9999);
    t.rtps.d0(9999);
    LogRestore lr;
    log_level.set(LogLevel::None);
    ASSERT_TRUE(t.rtps.ipv6_spdp_multicast_address(t.addr, 9999));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(57599, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.ipv6_spdp_multicast_address(fake_ipv6_addr);
    ASSERT_TRUE(t.rtps.ipv6_spdp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, fake_ipv6_addr);
    EXPECT_FALSE(t.fixed);
  }
}
#endif
