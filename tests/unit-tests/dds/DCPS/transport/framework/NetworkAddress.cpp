#include <dds/DCPS/transport/framework/NetworkAddress.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(network_addres_test, choose_single_coherent_address_double)
{
  ACE_INET_Addr addr1 = choose_single_coherent_address("www.google.com:80", false);
  ACE_INET_Addr addr2 = choose_single_coherent_address("www.google.com:80", false);
  EXPECT_NE(addr1, ACE_INET_Addr());
  EXPECT_NE(addr2, ACE_INET_Addr());
  EXPECT_EQ(addr1, addr2);
#if defined ACE_HAS_IPV6 && defined IPV6_V6ONLY
  EXPECT_FALSE(addr1.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr2.is_ipv4_mapped_ipv6());
#endif
}

TEST(network_addres_test, choose_single_coherent_address_double_portless)
{
  ACE_INET_Addr addr1 = choose_single_coherent_address("www.yahoo.com", false);
  ACE_INET_Addr addr2 = choose_single_coherent_address("www.yahoo.com", false);
  EXPECT_NE(addr1, ACE_INET_Addr());
  EXPECT_NE(addr2, ACE_INET_Addr());
  EXPECT_EQ(addr1, addr2);
#if defined ACE_HAS_IPV6 && defined IPV6_V6ONLY
  EXPECT_FALSE(addr1.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr2.is_ipv4_mapped_ipv6());
#endif
}

TEST(network_addres_test, choose_single_coherent_address_triple)
{
  ACE_INET_Addr addr1 = choose_single_coherent_address("www.facebook.com:80", false);
  ACE_INET_Addr addr2 = choose_single_coherent_address("www.facebook.com:80", false);
  ACE_INET_Addr addr3 = choose_single_coherent_address("www.facebook.com:80", false);
  EXPECT_NE(addr1, ACE_INET_Addr());
  EXPECT_NE(addr2, ACE_INET_Addr());
  EXPECT_NE(addr3, ACE_INET_Addr());
  EXPECT_EQ(addr1, addr2);
  EXPECT_EQ(addr2, addr3);
  EXPECT_EQ(addr3, addr1);
#if defined ACE_HAS_IPV6 && defined IPV6_V6ONLY
  EXPECT_FALSE(addr1.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr2.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr3.is_ipv4_mapped_ipv6());
#endif
}

TEST(network_addres_test, choose_single_coherent_address_triple_self)
{
  OPENDDS_STRING hostname = get_fully_qualified_hostname();
  ACE_INET_Addr addr1 = choose_single_coherent_address(hostname + ":5432", false);
  ACE_INET_Addr addr2 = choose_single_coherent_address(hostname + ":5432", false);
  ACE_INET_Addr addr3 = choose_single_coherent_address(hostname + ":5432", false);
  EXPECT_NE(addr1, ACE_INET_Addr());
  EXPECT_NE(addr2, ACE_INET_Addr());
  EXPECT_NE(addr3, ACE_INET_Addr());
  EXPECT_EQ(addr1, addr2);
  EXPECT_EQ(addr2, addr3);
  EXPECT_EQ(addr3, addr1);
#if defined ACE_HAS_IPV6 && defined IPV6_V6ONLY
  EXPECT_FALSE(addr1.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr2.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr3.is_ipv4_mapped_ipv6());
#endif
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
