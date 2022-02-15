/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include "dds/DCPS/NetworkAddress.h"

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_NetworkAddress, DefaultConstructor)
{
  const NetworkAddress sa;
  EXPECT_EQ(sa.get_port_number(), 0);
  EXPECT_EQ(sa, NetworkAddress());
}

TEST(dds_DCPS_NetworkAddress, AddrConstructorDefault)
{
  const ACE_INET_Addr ia;
  const NetworkAddress sa(ia);
  EXPECT_EQ(sa, NetworkAddress());
  EXPECT_EQ(sa.to_addr(), ia);
  EXPECT_EQ(sa.to_addr(), ACE_INET_Addr());
}

TEST(dds_DCPS_NetworkAddress, AddrConstructorPortStrIpFour)
{
  const ACE_INET_Addr ia(1234, "127.0.10.13");
  const NetworkAddress sa(ia);
  EXPECT_EQ(sa.get_type(), AF_INET);
  EXPECT_EQ(sa.get_port_number(), 1234);
  EXPECT_EQ(sa.to_addr(), ia);
}

TEST(dds_DCPS_NetworkAddress, AddrConstructorStrIpFour)
{
  const ACE_INET_Addr ia("127.0.10.13:4321");
  const NetworkAddress sa(ia);
  EXPECT_EQ(sa.get_type(), AF_INET);
  EXPECT_EQ(sa.get_port_number(), 4321);
  EXPECT_EQ(sa.to_addr(), ia);
}

#if defined (ACE_HAS_IPV6)

TEST(dds_DCPS_NetworkAddress, AddrConstructorPortStrIpSix)
{
  const ACE_INET_Addr ia(1234, "::FD01");
  const NetworkAddress sa(ia);
  EXPECT_EQ(sa.get_type(), AF_INET6);
  EXPECT_EQ(sa.get_port_number(), 1234);
  EXPECT_EQ(sa.to_addr(), ia);
}

TEST(dds_DCPS_NetworkAddress, AddrConstructortStrIpSix)
{
  const ACE_INET_Addr ia("::FE03:4321");
  const NetworkAddress sa(ia);
  EXPECT_EQ(sa.get_type(), AF_INET6);
  EXPECT_EQ(sa.get_port_number(), 4321);
  EXPECT_EQ(sa.to_addr(), ia);
}

#endif

TEST(dds_DCPS_NetworkAddress, PortStrConstructorIpFour)
{
  const NetworkAddress sa(1234, "127.0.10.13");
  EXPECT_EQ(sa.get_type(), AF_INET);
  EXPECT_EQ(sa.get_port_number(), 1234);
  EXPECT_EQ(sa.to_addr(), ACE_INET_Addr(1234, "127.0.10.13"));
  EXPECT_EQ(sa, NetworkAddress(1234, "127.0.10.13"));
}

TEST(dds_DCPS_NetworkAddress, StrConstructorIpFour)
{
  const NetworkAddress sa("127.0.10.13:4321");
  EXPECT_EQ(sa.get_type(), AF_INET);
  EXPECT_EQ(sa.get_port_number(), 4321);
  EXPECT_EQ(sa.to_addr(), ACE_INET_Addr(4321, "127.0.10.13"));
  EXPECT_EQ(sa, NetworkAddress(4321, "127.0.10.13"));
}

#if defined (ACE_HAS_IPV6)

TEST(dds_DCPS_NetworkAddress, PortStrConstructorIpSix)
{
  const NetworkAddress sa(1234, "::FD01");
  EXPECT_EQ(sa.get_type(), AF_INET6);
  EXPECT_EQ(sa.get_port_number(), 1234);
  EXPECT_EQ(sa.to_addr(), ACE_INET_Addr(1234, "::FD01"));
  EXPECT_EQ(sa, NetworkAddress(1234, "::FD01"));
}

TEST(dds_DCPS_NetworkAddress, StrConstructorIpSix)
{
  const NetworkAddress sa("::FE03:4321");
  EXPECT_EQ(sa.get_type(), AF_INET6);
  EXPECT_EQ(sa.get_port_number(), 4321);
  EXPECT_EQ(sa.to_addr(), ACE_INET_Addr(4321, "::FE03"));
  EXPECT_EQ(sa, NetworkAddress(4321, "::FE03"));
}

#endif

TEST(dds_DCPS_NetworkAddress, SetPortIpFour)
{
  NetworkAddress sa(1234, "127.0.255.255");
  EXPECT_EQ(sa.get_type(), AF_INET);
  EXPECT_EQ(sa.get_port_number(), 1234);
  sa.set_port_number(1515);
  EXPECT_EQ(sa.get_type(), AF_INET);
  EXPECT_EQ(sa.get_port_number(), 1515);
  EXPECT_EQ(sa, NetworkAddress(1515, "127.0.255.255"));
}

#if defined (ACE_HAS_IPV6)

TEST(dds_DCPS_NetworkAddress, SetPortIpSix)
{
  NetworkAddress sa("::FE03:4321");
  EXPECT_EQ(sa.get_type(), AF_INET6);
  EXPECT_EQ(sa.get_port_number(), 4321);
  sa.set_port_number(1515);
  EXPECT_EQ(sa.get_type(), AF_INET6);
  EXPECT_EQ(sa.get_port_number(), 1515);
  EXPECT_EQ(sa, NetworkAddress(1515, "::FE03"));
}

#endif

TEST(dds_DCPS_NetworkAddress, OperatorsIpFour)
{
  NetworkAddress sa1(1234, "1.1.1.1");
  NetworkAddress sa2(1234, "2.2.2.2");
  NetworkAddress sa3(5678, "1.1.1.1");
  NetworkAddress sa4(5678, "2.2.2.2");

  EXPECT_NE(sa1, sa2);
  EXPECT_NE(sa1, sa3);
  EXPECT_NE(sa1, sa4);
  EXPECT_NE(sa2, sa1);
  EXPECT_NE(sa2, sa3);
  EXPECT_NE(sa2, sa4);
  EXPECT_NE(sa3, sa1);
  EXPECT_NE(sa3, sa2);
  EXPECT_NE(sa3, sa4);
  EXPECT_NE(sa4, sa1);
  EXPECT_NE(sa4, sa2);
  EXPECT_NE(sa4, sa3);

  EXPECT_LT(sa1, sa2);
  EXPECT_LT(sa1, sa3);
  EXPECT_LT(sa1, sa4);
  EXPECT_LT(sa2, sa3);
  EXPECT_LT(sa2, sa4);
  EXPECT_LT(sa3, sa4);

  EXPECT_TRUE(sa1.addr_bytes_equal(sa3));
  EXPECT_TRUE(sa2.addr_bytes_equal(sa4));

  EXPECT_FALSE(sa1.addr_bytes_equal(sa2));
  EXPECT_FALSE(sa3.addr_bytes_equal(sa4));
}

#if defined (ACE_HAS_IPV6)

TEST(dds_DCPS_NetworkAddress, OperatorsIpSix)
{
  NetworkAddress sa1(1234, "1.1.1.1");
  NetworkAddress sa2(1234, "0101:0101:0101:0101:0101:0101:0101:0101");
  NetworkAddress sa3(1234, "0202:0202:0202:0202:0202:0202:0202:0202");
  NetworkAddress sa4(5678, "0101:0101:0101:0101:0101:0101:0101:0101");
  NetworkAddress sa5(5678, "0202:0202:0202:0202:0202:0202:0202:0202");

  EXPECT_NE(sa1, sa2);
  EXPECT_NE(sa1, sa3);
  EXPECT_NE(sa1, sa4);
  EXPECT_NE(sa1, sa5);
  EXPECT_NE(sa2, sa1);
  EXPECT_NE(sa2, sa3);
  EXPECT_NE(sa2, sa4);
  EXPECT_NE(sa2, sa5);
  EXPECT_NE(sa3, sa1);
  EXPECT_NE(sa3, sa2);
  EXPECT_NE(sa3, sa4);
  EXPECT_NE(sa3, sa5);
  EXPECT_NE(sa4, sa1);
  EXPECT_NE(sa4, sa2);
  EXPECT_NE(sa4, sa3);
  EXPECT_NE(sa4, sa5);
  EXPECT_NE(sa5, sa1);
  EXPECT_NE(sa5, sa2);
  EXPECT_NE(sa5, sa3);
  EXPECT_NE(sa5, sa4);

  EXPECT_LT(sa1, sa2);
  EXPECT_LT(sa1, sa3);
  EXPECT_LT(sa1, sa4);
  EXPECT_LT(sa1, sa5);
  EXPECT_LT(sa2, sa3);
  EXPECT_LT(sa2, sa4);
  EXPECT_LT(sa2, sa5);
  EXPECT_LT(sa3, sa4);
  EXPECT_LT(sa3, sa5);
  EXPECT_LT(sa4, sa5);

  EXPECT_TRUE(sa2.addr_bytes_equal(sa4));
  EXPECT_TRUE(sa3.addr_bytes_equal(sa5));

  EXPECT_FALSE(sa2.addr_bytes_equal(sa3));
  EXPECT_FALSE(sa4.addr_bytes_equal(sa5));

  EXPECT_FALSE(sa1.addr_bytes_equal(sa2));
  EXPECT_FALSE(sa3.addr_bytes_equal(sa1));
}

#endif

TEST(dds_DCPS_NetworkAddress, IsLoopbackIpFour)
{
  NetworkAddress sa1(1234, "1.2.3.4");
  NetworkAddress sa2(1234, "192.168.0.127");
  NetworkAddress sa3(1234, "127.0.0.1");
  NetworkAddress sa4(1234, "127.0.1.1");

  EXPECT_FALSE(sa1.is_loopback());
  EXPECT_FALSE(sa2.is_loopback());

  EXPECT_TRUE(sa3.is_loopback());
  EXPECT_TRUE(sa4.is_loopback());
}

#if defined (ACE_HAS_IPV6)

TEST(dds_DCPS_NetworkAddress, IsLoopbackIpSix)
{
  NetworkAddress sa1(1234, "::1");
  NetworkAddress sa2(1234, "::2");
  NetworkAddress sa3(1234, "0101:0101:0101:0101:0101:0101:0101:0101");
  NetworkAddress sa4(1234, "0202:0202:0202:0202:0202:0202:0202:0202");

  EXPECT_TRUE(sa1.is_loopback());

  EXPECT_FALSE(sa2.is_loopback());
  EXPECT_FALSE(sa3.is_loopback());
  EXPECT_FALSE(sa4.is_loopback());
}

#endif

TEST(dds_DCPS_NetworkAddress, IsPrivateIpFour)
{
  NetworkAddress sa1(1234, "7.8.9.10");
  NetworkAddress sa2(1234, "127.0.0.1");
  NetworkAddress sa3(1234, "127.0.168.192");
  NetworkAddress sa4(1234, "127.0.16.172");

  NetworkAddress sb1(1234, "9.255.255.255");
  NetworkAddress sb2(1234, "10.0.0.0");
  NetworkAddress sb3(1234, "10.255.255.255");
  NetworkAddress sb4(1234, "11.0.0.0");

  NetworkAddress sc1(1234, "172.15.255.255");
  NetworkAddress sc2(1234, "172.16.0.0");
  NetworkAddress sc3(1234, "172.31.255.255");
  NetworkAddress sc4(1234, "172.32.0.0");

  NetworkAddress sd1(1234, "192.167.255.255");
  NetworkAddress sd2(1234, "192.168.0.0");
  NetworkAddress sd3(1234, "192.168.255.255");
  NetworkAddress sd4(1234, "192.169.0.0");

  EXPECT_FALSE(sa1.is_private());
  EXPECT_FALSE(sa2.is_private());
  EXPECT_FALSE(sa3.is_private());
  EXPECT_FALSE(sa4.is_private());

  EXPECT_FALSE(sb1.is_private());
  EXPECT_TRUE(sb2.is_private());
  EXPECT_TRUE(sb3.is_private());
  EXPECT_FALSE(sb4.is_private());

  EXPECT_FALSE(sc1.is_private());
  EXPECT_TRUE(sc2.is_private());
  EXPECT_TRUE(sc3.is_private());
  EXPECT_FALSE(sc4.is_private());

  EXPECT_FALSE(sd1.is_private());
  EXPECT_TRUE(sd2.is_private());
  EXPECT_TRUE(sd3.is_private());
  EXPECT_FALSE(sd4.is_private());
}

#if defined (ACE_HAS_IPV6)

TEST(dds_DCPS_NetworkAddress, IsUniqueLocalIpSix)
{
  NetworkAddress sa1(1234, "::1");
  NetworkAddress sa2(1234, "0101::");
  NetworkAddress sa3(1234, "2600:6c40:6100:4f4:1def:d066:90c:3371");
  NetworkAddress sa4(1234, "fe80::1");
  NetworkAddress sa5(1234, "fe80:1::1");
  NetworkAddress sa6(1234, "fe80::1:1");
  NetworkAddress sa7(1234, "fec0::1");
  NetworkAddress sa8(1234, "fec2:1::1");
  NetworkAddress sa9(1234, "fed0::1:1");
  NetworkAddress sa10(1234, "fc03::1");
  NetworkAddress sa11(1234, "fd01::2");

  EXPECT_FALSE(sa1.is_uniquelocal());
  EXPECT_FALSE(sa2.is_uniquelocal());
  EXPECT_FALSE(sa3.is_uniquelocal());

  EXPECT_FALSE(sa4.is_uniquelocal());
  EXPECT_FALSE(sa5.is_uniquelocal());
  EXPECT_FALSE(sa6.is_uniquelocal());

  EXPECT_FALSE(sa7.is_uniquelocal());
  EXPECT_FALSE(sa8.is_uniquelocal());
  EXPECT_FALSE(sa9.is_uniquelocal());

  EXPECT_TRUE(sa10.is_uniquelocal());
  EXPECT_TRUE(sa11.is_uniquelocal());
}

TEST(dds_DCPS_NetworkAddress, IsLinkLocalIpSix)
{
  NetworkAddress sa1(1234, "::1");
  NetworkAddress sa2(1234, "0101::");
  NetworkAddress sa3(1234, "2600:6c40:6100:4f4:1def:d066:90c:3371");
  NetworkAddress sa4(1234, "fe80::1");
  NetworkAddress sa5(1234, "fe80:1::1");
  NetworkAddress sa6(1234, "fe80::1:1");
  NetworkAddress sa7(1234, "fec0::1");
  NetworkAddress sa8(1234, "fec2:1::1");
  NetworkAddress sa9(1234, "fed0::1:1");
  NetworkAddress sa10(1234, "fc03::1");
  NetworkAddress sa11(1234, "fd01::2");

  EXPECT_FALSE(sa1.is_linklocal());
  EXPECT_FALSE(sa2.is_linklocal());
  EXPECT_FALSE(sa3.is_linklocal());

  EXPECT_TRUE(sa4.is_linklocal());
  EXPECT_TRUE(sa5.is_linklocal());
  EXPECT_TRUE(sa6.is_linklocal());

  EXPECT_FALSE(sa7.is_linklocal());
  EXPECT_FALSE(sa8.is_linklocal());
  EXPECT_FALSE(sa9.is_linklocal());

  EXPECT_FALSE(sa10.is_linklocal());
  EXPECT_FALSE(sa11.is_linklocal());
}

TEST(dds_DCPS_NetworkAddress, IsSiteLocalIpSix)
{
  NetworkAddress sa1(1234, "::1");
  NetworkAddress sa2(1234, "0101::");
  NetworkAddress sa3(1234, "2600:6c40:6100:4f4:1def:d066:90c:3371");
  NetworkAddress sa4(1234, "fe80::1");
  NetworkAddress sa5(1234, "fe80:1::1");
  NetworkAddress sa6(1234, "fe80::1:1");
  NetworkAddress sa7(1234, "fec0::1");
  NetworkAddress sa8(1234, "fec2:1::1");
  NetworkAddress sa9(1234, "fed0::1:1");
  NetworkAddress sa10(1234, "fc03::1");
  NetworkAddress sa11(1234, "fd01::2");

  EXPECT_FALSE(sa1.is_sitelocal());
  EXPECT_FALSE(sa2.is_sitelocal());
  EXPECT_FALSE(sa3.is_sitelocal());

  EXPECT_FALSE(sa4.is_sitelocal());
  EXPECT_FALSE(sa5.is_sitelocal());
  EXPECT_FALSE(sa6.is_sitelocal());

  EXPECT_TRUE(sa7.is_sitelocal());
  EXPECT_TRUE(sa8.is_sitelocal());
  EXPECT_TRUE(sa9.is_sitelocal());

  EXPECT_FALSE(sa10.is_sitelocal());
  EXPECT_FALSE(sa11.is_sitelocal());
}

#endif
