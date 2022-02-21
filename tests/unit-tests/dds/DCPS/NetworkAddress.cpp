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

  // This is necessary because of a deficiency in ACE_INET_Addr where calling set_addr will not copy / set
  // the bytes of 'sin_zero' (reserved for system use), but the 2 argument constructor might, depending on the system
  ACE_INET_Addr ia2;
  ia2.set_addr(ia.get_addr(), ia.get_addr_size());
  EXPECT_EQ(sa.to_addr(), ia2);
}

TEST(dds_DCPS_NetworkAddress, AddrConstructorStrIpFour)
{
  const ACE_INET_Addr ia("127.0.10.13:4321");
  const NetworkAddress sa(ia);
  EXPECT_EQ(sa.get_type(), AF_INET);
  EXPECT_EQ(sa.get_port_number(), 4321);

  // This is necessary because of a deficiency in ACE_INET_Addr where calling set_addr will not copy / set
  // the bytes of 'sin_zero' (reserved for system use), but the 2 argument constructor might, depending on the system
  ACE_INET_Addr ia2;
  ia2.set_addr(ia.get_addr(), ia.get_addr_size());
  EXPECT_EQ(sa.to_addr(), ia2);
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

  // This is necessary because of a deficiency in ACE_INET_Addr where calling set_addr will not copy / set
  // the bytes of 'sin_zero' (reserved for system use), but the 2 argument constructor might, depending on the system
  ACE_INET_Addr ia(1234, "127.0.10.13");
  ACE_INET_Addr ia2;
  ia2.set_addr(ia.get_addr(), ia.get_addr_size());
  EXPECT_EQ(sa.to_addr(), ia2);

  EXPECT_EQ(sa, NetworkAddress(1234, "127.0.10.13"));
}

TEST(dds_DCPS_NetworkAddress, StrConstructorIpFour)
{
  const NetworkAddress sa("127.0.10.13:4321");
  EXPECT_EQ(sa.get_type(), AF_INET);
  EXPECT_EQ(sa.get_port_number(), 4321);

  // This is necessary because of a deficiency in ACE_INET_Addr where calling set_addr will not copy / set
  // the bytes of 'sin_zero' (reserved for system use), but the 2 argument constructor might, depending on the system
  ACE_INET_Addr ia(4321, "127.0.10.13");
  ACE_INET_Addr ia2;
  ia2.set_addr(ia.get_addr(), ia.get_addr_size());
  EXPECT_EQ(sa.to_addr(), ia2);

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

TEST(dds_DCPS_NetworkAddress, IsMoreLocalIpFour)
{
  NetworkAddress sa1(1234, "127.0.0.1");
  NetworkAddress sa2(1234, "127.0.1.1");
  NetworkAddress sa3(1234, "192.168.0.127");
  NetworkAddress sa4(1234, "10.11.12.13");
  NetworkAddress sa5(1234, "1.2.3.4");
  NetworkAddress sa6(1234, "2.3.4.5");

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa1, sa1));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa2, sa1));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa3, sa1));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa4, sa1));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa5, sa1));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa6, sa1));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa1, sa2));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa2, sa2));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa3, sa2));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa4, sa2));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa5, sa2));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa6, sa2));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa1, sa3));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa2, sa3));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa3, sa3));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa4, sa3));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa5, sa3));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa6, sa3));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa1, sa4));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa2, sa4));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa3, sa4));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa4, sa4));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa5, sa4));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa6, sa4));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa1, sa5));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa2, sa5));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa3, sa5));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa4, sa5));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa5, sa5));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa6, sa5));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa1, sa6));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa2, sa6));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa3, sa6));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa4, sa6));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa5, sa6));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa6, sa6));
}

#if defined (ACE_HAS_IPV6)

TEST(dds_DCPS_NetworkAddress, IsMoreLocalIpSix)
{
  NetworkAddress sa01(1234, "::1"); // loopback
  NetworkAddress sa02(1234, "fe80::1"); // link local
  NetworkAddress sa03(1234, "fe80:1::1");
  NetworkAddress sa04(1234, "fe80::1:1");
  NetworkAddress sa05(1234, "fc03::1"); // unique local
  NetworkAddress sa06(1234, "fd01::2");
  NetworkAddress sa07(1234, "fec0::1"); // site local
  NetworkAddress sa08(1234, "fec2:1::1");
  NetworkAddress sa09(1234, "fed0::1:1");
  NetworkAddress sa10(1234, "0101::"); // global
  NetworkAddress sa11(1234, "2600:6c40:6100:4f4:1def:d066:90c:3371");
  NetworkAddress sa12(1234, "1.2.3.4");
  NetworkAddress sa13(1234, "2.3.4.5");

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa01));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa02, sa01));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa03, sa01));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa04, sa01));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa05, sa01));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa06, sa01));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa07, sa01));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa08, sa01));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa09, sa01));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa10, sa01));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa11, sa01));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa12, sa01));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa13, sa01));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa02));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa02, sa02));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa03, sa02));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa04, sa02));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa05, sa02));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa06, sa02));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa07, sa02));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa08, sa02));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa09, sa02));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa10, sa02));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa11, sa02));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa12, sa02));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa13, sa02));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa03));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa02, sa03));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa03, sa03));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa04, sa03));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa05, sa03));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa06, sa03));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa07, sa03));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa08, sa03));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa09, sa03));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa10, sa03));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa11, sa03));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa12, sa03));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa13, sa03));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa04));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa02, sa04));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa03, sa04));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa04, sa04));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa05, sa04));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa06, sa04));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa07, sa04));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa08, sa04));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa09, sa04));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa10, sa04));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa11, sa04));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa12, sa04));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa13, sa04));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa05));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa02, sa05));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa03, sa05));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa04, sa05));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa05, sa05));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa06, sa05));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa07, sa05));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa08, sa05));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa09, sa05));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa10, sa05));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa11, sa05));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa12, sa05));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa13, sa05));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa06));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa02, sa06));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa03, sa06));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa04, sa06));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa05, sa06));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa06, sa06));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa07, sa06));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa08, sa06));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa09, sa06));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa10, sa06));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa11, sa06));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa12, sa06));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa13, sa06));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa07));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa02, sa07));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa03, sa07));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa04, sa07));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa05, sa07));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa06, sa07));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa07, sa07));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa08, sa07));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa09, sa07));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa10, sa07));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa11, sa07));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa12, sa07));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa13, sa07));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa08));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa02, sa08));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa03, sa08));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa04, sa08));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa05, sa08));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa06, sa08));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa07, sa08));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa08, sa08));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa09, sa08));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa10, sa08));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa11, sa08));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa12, sa08));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa13, sa08));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa09));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa02, sa09));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa03, sa09));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa04, sa09));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa05, sa09));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa06, sa09));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa07, sa09));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa08, sa09));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa09, sa09));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa10, sa09));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa11, sa09));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa12, sa09));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa13, sa09));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa10));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa02, sa10));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa03, sa10));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa04, sa10));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa05, sa10));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa06, sa10));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa07, sa10));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa08, sa10));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa09, sa10));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa10, sa10));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa11, sa10));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa12, sa10));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa13, sa10));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa11));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa02, sa11));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa03, sa11));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa04, sa11));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa05, sa11));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa06, sa11));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa07, sa11));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa08, sa11));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa09, sa11));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa10, sa11));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa11, sa11));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa12, sa11));
  EXPECT_TRUE (OpenDDS::DCPS::is_more_local(sa13, sa11));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa12));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa02, sa12));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa03, sa12));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa04, sa12));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa05, sa12));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa06, sa12));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa07, sa12));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa08, sa12));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa09, sa12));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa10, sa12));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa11, sa12));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa12, sa12));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa13, sa12));

  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa01, sa13));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa02, sa13));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa03, sa13));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa04, sa13));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa05, sa13));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa06, sa13));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa07, sa13));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa08, sa13));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa09, sa13));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa10, sa13));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa11, sa13));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa12, sa13));
  EXPECT_FALSE(OpenDDS::DCPS::is_more_local(sa13, sa13));
}

#endif
