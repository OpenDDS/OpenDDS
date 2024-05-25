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
}

TEST(dds_DCPS_RTPS_RtpsDiscoveryConfig, sedp_unicast_address)
{
  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.sedp_unicast_address(t.addr, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress());
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_port_mode(PortMode_System);
    ASSERT_TRUE(t.rtps.sedp_unicast_address(t.addr, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress());
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps.sedp_unicast_address(t.addr, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7662, "0.0.0.0"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps.sedp_unicast_address(t.addr, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7668, "0.0.0.0"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_port_mode(PortMode_Probe);
    t.rtps.pb(7500);
    t.rtps.dg(260);
    t.rtps.pg(3);
    t.rtps.dy(8);
    ASSERT_TRUE(t.rtps.sedp_unicast_address(t.addr, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, "0.0.0.0"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_port_mode(PortMode_Probe);
    t.rtps.pb(9999);
    t.rtps.dg(9999);
    t.rtps.pg(9999);
    t.rtps.dy(9999);
    LogRestore lr;
    log_level.set(LogLevel::None);
    ASSERT_TRUE(t.rtps.sedp_unicast_address(t.addr, 9999, 9999));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(29664, "0.0.0.0"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_local_address(NetworkAddress(1234, "1.2.3.4"));
    ASSERT_TRUE(t.rtps.sedp_unicast_address(t.addr, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(1234, "1.2.3.4"));
    EXPECT_FALSE(t.fixed);
  }
}

TEST(dds_DCPS_RTPS_RtpsDiscoveryConfig, spdp_unicast_address)
{
  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7660, "0.0.0.0"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps.spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7660, "0.0.0.0"));
    EXPECT_FALSE(t.fixed);
  }


  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_System);
    ASSERT_TRUE(t.rtps.spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress());
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps.spdp_unicast_address(t.addr, t.fixed, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7666, "0.0.0.0"));
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
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, "0.0.0.0"));
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
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(29664, "0.0.0.0"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_local_address(NetworkAddress(1234, "1.2.3.4"));
    ASSERT_TRUE(t.rtps.spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(1234, "1.2.3.4"));
    EXPECT_TRUE(t.fixed);
  }
}

TEST(dds_DCPS_RTPS_RtpsDiscoveryConfig, spdp_multicast_address)
{
  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.spdp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7650, "239.255.0.1"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.spdp_multicast_address(t.addr, 2));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7900, "239.255.0.1"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.pb(7500);
    t.rtps.dg(260);
    t.rtps.d0(17);
    ASSERT_TRUE(t.rtps.spdp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, "239.255.0.1"));
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
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(57599, "239.255.0.1"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_multicast_address(NetworkAddress(1234, "1.2.3.4"));
    ASSERT_TRUE(t.rtps.spdp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(1234, "1.2.3.4"));
    EXPECT_FALSE(t.fixed);
  }
}

TEST(dds_DCPS_RTPS_RtpsDiscoveryConfig, sedp_multicast_address)
{
  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.sedp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7652, "239.255.0.1"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.sedp_multicast_address(t.addr, 2));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7902, "239.255.0.1"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.pb(7500);
    t.rtps.dg(260);
    t.rtps.dx(17);
    ASSERT_TRUE(t.rtps.sedp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, "239.255.0.1"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.pb(9999);
    t.rtps.dg(9999);
    t.rtps.dx(9999);
    LogRestore lr;
    log_level.set(LogLevel::None);
    ASSERT_TRUE(t.rtps.sedp_multicast_address(t.addr, 9999));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(57599, "239.255.0.1"));

    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_multicast_address(NetworkAddress(1234, "1.2.3.4"));
    ASSERT_TRUE(t.rtps.sedp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(1234, "1.2.3.4"));
    EXPECT_FALSE(t.fixed);
  }
}

#ifdef ACE_HAS_IPV6
TEST(dds_DCPS_RTPS_RtpsDiscoveryConfig, ipv6_sedp_unicast_address)
{
  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.ipv6_sedp_unicast_address(t.addr, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress());
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_port_mode(PortMode_System);
    ASSERT_TRUE(t.rtps.ipv6_sedp_unicast_address(t.addr, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress());
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps.ipv6_sedp_unicast_address(t.addr, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7662, "[::]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps.ipv6_sedp_unicast_address(t.addr, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7668, "[::]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_port_mode(PortMode_Probe);
    t.rtps.pb(7500);
    t.rtps.dg(260);
    t.rtps.pg(3);
    t.rtps.dy(8);
    ASSERT_TRUE(t.rtps.ipv6_sedp_unicast_address(t.addr, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, "[::]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_port_mode(PortMode_Probe);
    t.rtps.pb(9999);
    t.rtps.dg(9999);
    t.rtps.pg(9999);
    t.rtps.dy(9999);
    LogRestore lr;
    log_level.set(LogLevel::None);
    ASSERT_TRUE(t.rtps.ipv6_sedp_unicast_address(t.addr, 9999, 9999));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(29664, "[::]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.sedp_local_address(NetworkAddress(1234, "[1:2:3:4]"));
    ASSERT_TRUE(t.rtps.ipv6_sedp_unicast_address(t.addr, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(1234, "[1:2:3:4]"));
    EXPECT_FALSE(t.fixed);
  }
}

TEST(dds_DCPS_RTPS_RtpsDiscoveryConfig, ipv6_spdp_unicast_address)
{
  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.ipv6_spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7660, "[::]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps.ipv6_spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7660, "[::]"));
    EXPECT_FALSE(t.fixed);
  }


  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_System);
    ASSERT_TRUE(t.rtps.ipv6_spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress());
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps.ipv6_spdp_unicast_address(t.addr, t.fixed, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7666, "[::]"));
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
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, "[::]"));
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
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(29664, "[::]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.spdp_local_address(NetworkAddress(1234, "[1:2:3:4]"));
    ASSERT_TRUE(t.rtps.ipv6_spdp_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(1234, "[1:2:3:4]"));
    EXPECT_TRUE(t.fixed);
  }
}

TEST(dds_DCPS_RTPS_RtpsDiscoveryConfig, ipv6_spdp_multicast_address)
{
  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.ipv6_spdp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7650, "[FF03::1]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.ipv6_spdp_multicast_address(t.addr, 2));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7900, "[FF03::1]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.pb(7500);
    t.rtps.dg(260);
    t.rtps.d0(17);
    ASSERT_TRUE(t.rtps.ipv6_spdp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, "[FF03::1]"));
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
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(57599, "[FF03::1]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.ipv6_spdp_multicast_address(NetworkAddress(1234, "[1:2:3:4]"));
    ASSERT_TRUE(t.rtps.ipv6_spdp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(1234, "[1:2:3:4]"));
    EXPECT_FALSE(t.fixed);
  }
}

TEST(dds_DCPS_RTPS_RtpsDiscoveryConfig, ipv6_sedp_multicast_address)
{
  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.ipv6_sedp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7652, "[FF03::1]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps.ipv6_sedp_multicast_address(t.addr, 2));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7902, "[FF03::1]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.pb(7500);
    t.rtps.dg(260);
    t.rtps.dx(17);
    ASSERT_TRUE(t.rtps.ipv6_sedp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, "[FF03::1]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.pb(9999);
    t.rtps.dg(9999);
    t.rtps.dx(9999);
    LogRestore lr;
    log_level.set(LogLevel::None);
    ASSERT_TRUE(t.rtps.ipv6_sedp_multicast_address(t.addr, 9999));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(57599, "[FF03::1]"));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps.ipv6_sedp_multicast_address(NetworkAddress(1234, "[1:2:3:4]"));
    ASSERT_TRUE(t.rtps.ipv6_sedp_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(1234, "[1:2:3:4]"));
    EXPECT_FALSE(t.fixed);
  }
}
#endif
