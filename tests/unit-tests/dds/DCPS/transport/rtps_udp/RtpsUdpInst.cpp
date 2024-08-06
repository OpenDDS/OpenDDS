#include <tests/Utils/GtestRc.h>

#include <dds/DCPS/transport/rtps_udp/RtpsUdpInst.h>

using namespace OpenDDS::RTPS;
using namespace OpenDDS::DCPS;

namespace {
  struct RtpsUdpType {
    RcHandle<ConfigStoreImpl> store;
    RcHandle<RtpsUdpInst> rtps_udp;

    RtpsUdpType()
    : store(make_rch<ConfigStoreImpl>(TheServiceParticipant->config_topic()))
    , rtps_udp(make_rch<RtpsUdpInst>("RTPS_UDP_INST_UNIT_TEST", true))
    {
      store->unset_section(rtps_udp->config_prefix());
    }

    ~RtpsUdpType()
    {
      store->unset_section(rtps_udp->config_prefix());
    }
  };

  struct AddressTest : public RtpsUdpType {
    NetworkAddress addr;
    bool fixed;

    AddressTest()
    : addr()
    , fixed(false)
    {
    }
  };

  const NetworkAddress fake_ipv4_addr(1234, "1.2.3.4");
#ifdef ACE_HAS_IPV6
  const NetworkAddress fake_ipv6_addr(1234, "::1:2:3:4");
#endif
}

TEST(dds_DCPS_RTPS_RtpsUdpInst, multicast_address)
{
  const char* const default_addr = "239.255.0.2";

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps_udp->multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7651, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps_udp->multicast_address(t.addr, 2));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7901, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->pb(7500);
    t.rtps_udp->dg(260);
    t.rtps_udp->d2(17);
    ASSERT_TRUE(t.rtps_udp->multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->pb(9999);
    t.rtps_udp->dg(9999);
    t.rtps_udp->d2(9999);
    LogRestore lr;
    log_level.set(LogLevel::None);
    ASSERT_TRUE(t.rtps_udp->multicast_address(t.addr, 9999));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(57599, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->multicast_group_address(fake_ipv4_addr);
    ASSERT_TRUE(t.rtps_udp->multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, fake_ipv4_addr);
    EXPECT_FALSE(t.fixed);
  }

  // add_domain_id_to_port template with fixed port
  {
    AddressTest t;
    t.store->set_string(t.rtps_udp->config_key("CUSTOMIZATION").c_str(), "CUST");
    t.store->set_string("CUSTOMIZATION_CUST_MULTICAST_GROUP_ADDRESS", "add_domain_id_to_port");
    t.rtps_udp->multicast_group_address(fake_ipv4_addr);
    ASSERT_TRUE(t.rtps_udp->multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(1235, "1.2.3.4"));
    EXPECT_FALSE(t.fixed);
  }

  // add_domain_id_to_port template with spec port
  {
    AddressTest t;
    t.store->set_string(t.rtps_udp->config_key("CUSTOMIZATION").c_str(), "CUST");
    t.store->set_string("CUSTOMIZATION_CUST_MULTICAST_GROUP_ADDRESS", "add_domain_id_to_port");
    t.rtps_udp->multicast_group_address(NetworkAddress(0, "1.2.3.4"));
    ASSERT_TRUE(t.rtps_udp->multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7652, "1.2.3.4"));
    EXPECT_FALSE(t.fixed);
  }
}

TEST(dds_DCPS_RTPS_RtpsUdpInst, unicast_address)
{
  const char* const default_addr = "0.0.0.0";

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps_udp->unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(0, default_addr));
    EXPECT_TRUE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->port_mode(PortMode_System);
    ASSERT_TRUE(t.rtps_udp->unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(0, default_addr));
    EXPECT_TRUE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps_udp->unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7661, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps_udp->unicast_address(t.addr, t.fixed, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7667, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->port_mode(PortMode_Probe);
    t.rtps_udp->pb(7500);
    t.rtps_udp->dg(260);
    t.rtps_udp->pg(3);
    t.rtps_udp->d3(8);
    ASSERT_TRUE(t.rtps_udp->unicast_address(t.addr, t.fixed, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->port_mode(PortMode_Probe);
    t.rtps_udp->pb(9999);
    t.rtps_udp->dg(9999);
    t.rtps_udp->pg(9999);
    t.rtps_udp->d3(9999);
    LogRestore lr;
    log_level.set(LogLevel::None);
    ASSERT_TRUE(t.rtps_udp->unicast_address(t.addr, t.fixed, 9999, 9999));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(29664, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->local_address(fake_ipv4_addr);
    ASSERT_TRUE(t.rtps_udp->unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, fake_ipv4_addr);
    EXPECT_TRUE(t.fixed);
  }
}

#ifdef ACE_HAS_IPV6
TEST(dds_DCPS_RTPS_RtpsUdpInst, ipv6_multicast_address)
{
  const char* const default_addr = "ff03::2";

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps_udp->ipv6_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7651, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps_udp->ipv6_multicast_address(t.addr, 2));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7901, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->pb(7500);
    t.rtps_udp->dg(260);
    t.rtps_udp->d2(17);
    ASSERT_TRUE(t.rtps_udp->ipv6_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->pb(9999);
    t.rtps_udp->dg(9999);
    t.rtps_udp->d2(9999);
    LogRestore lr;
    log_level.set(LogLevel::None);
    ASSERT_TRUE(t.rtps_udp->ipv6_multicast_address(t.addr, 9999));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(57599, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->ipv6_multicast_group_address(fake_ipv6_addr);
    ASSERT_TRUE(t.rtps_udp->ipv6_multicast_address(t.addr, 1));
    EXPECT_ADDR_EQ(t.addr, fake_ipv6_addr);
    EXPECT_FALSE(t.fixed);
  }
}

TEST(dds_DCPS_RTPS_RtpsUdpInst, ipv6_unicast_address)
{
  const char* const default_addr = "::";

  {
    AddressTest t;
    ASSERT_TRUE(t.rtps_udp->ipv6_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(0, default_addr));
    EXPECT_TRUE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->port_mode(PortMode_System);
    ASSERT_TRUE(t.rtps_udp->ipv6_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(0, default_addr));
    EXPECT_TRUE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps_udp->ipv6_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7661, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->port_mode(PortMode_Probe);
    ASSERT_TRUE(t.rtps_udp->ipv6_unicast_address(t.addr, t.fixed, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7667, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->port_mode(PortMode_Probe);
    t.rtps_udp->pb(7500);
    t.rtps_udp->dg(260);
    t.rtps_udp->pg(3);
    t.rtps_udp->d3(8);
    ASSERT_TRUE(t.rtps_udp->ipv6_unicast_address(t.addr, t.fixed, 1, 3));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(7777, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->port_mode(PortMode_Probe);
    t.rtps_udp->pb(9999);
    t.rtps_udp->dg(9999);
    t.rtps_udp->pg(9999);
    t.rtps_udp->d3(9999);
    LogRestore lr;
    log_level.set(LogLevel::None);
    ASSERT_TRUE(t.rtps_udp->ipv6_unicast_address(t.addr, t.fixed, 9999, 9999));
    EXPECT_ADDR_EQ(t.addr, NetworkAddress(29664, default_addr));
    EXPECT_FALSE(t.fixed);
  }

  {
    AddressTest t;
    t.rtps_udp->ipv6_local_address(fake_ipv6_addr);
    ASSERT_TRUE(t.rtps_udp->ipv6_unicast_address(t.addr, t.fixed, 1, 0));
    EXPECT_ADDR_EQ(t.addr, fake_ipv6_addr);
    EXPECT_TRUE(t.fixed);
  }
}
#endif
