#include <dds/DCPS/NetworkResource.h>
#include <dds/DCPS/transport/framework/TransportDebug.h>

#include <ace/Init_ACE.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

// Test Utils

namespace {

class ScopedDebugLevels {
public:
  explicit ScopedDebugLevels(unsigned int level)
    : previous_transport_debug_level_(OpenDDS::DCPS::Transport_debug_level)
  {
    OpenDDS::DCPS::Transport_debug_level = level;
  }

  ~ScopedDebugLevels()
  {
    OpenDDS::DCPS::Transport_debug_level = previous_transport_debug_level_;
  }

private:
  ScopedDebugLevels();

  unsigned int previous_transport_debug_level_;
};
}

// Tests

TEST(dds_DCPS_NetworkResource, hostname_info)
{
  HostnameInfo info_1;
  info_1.index_ = 3;
  info_1.hostname_ = "test.com";
  HostnameInfo info_3((HostnameInfo(info_1))); // info_2 is unnamed

  EXPECT_EQ(info_1.index_, info_3.index_);
  EXPECT_EQ(info_1.hostname_, info_3.hostname_);
}

TEST(dds_DCPS_NetworkResource, fully_qualified_domain_hostname_basic)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  String empty;
  String hostname = get_fully_qualified_hostname();
  EXPECT_NE(hostname, empty);
}

TEST(dds_DCPS_NetworkResource, choose_single_coherent_address_ipv4)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  const ACE_INET_Addr addr1("127.0.0.1:80", AF_INET);
  const ACE_INET_Addr addr2("192.168.2.7:80", AF_INET);
  const ACE_INET_Addr addr3("8.4.8.4:80", AF_INET);
  const ACE_INET_Addr addr4("148.8.12.37:80", AF_INET);

  OPENDDS_VECTOR(ACE_INET_Addr) vec;
  vec.push_back(addr1);
  vec.push_back(addr2);
  vec.push_back(addr3);
  vec.push_back(addr4);

  const OPENDDS_VECTOR(ACE_INET_Addr) vec1 = vec;
  const OPENDDS_VECTOR(ACE_INET_Addr) vec2 = vec;

  const ACE_INET_Addr result1 = choose_single_coherent_address(vec1, false);
  EXPECT_EQ(result1, addr3);

  const ACE_INET_Addr result2 = choose_single_coherent_address(vec2, true);
  EXPECT_EQ(result2, addr1);
}

#if defined ACE_HAS_IPV6
TEST(dds_DCPS_NetworkResource, choose_single_coherent_address_ipv6)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  const ACE_INET_Addr addr1("::1:80", AF_INET6);
  const ACE_INET_Addr addr2("fe80::f526:7d3e:c625:8f0c:80", AF_INET6);
  const ACE_INET_Addr addr3("2001:4860:4860::8844:80", AF_INET6);
  const ACE_INET_Addr addr4("2606:4700:4700::6400:80", AF_INET6);

  OPENDDS_VECTOR(ACE_INET_Addr) vec;
  vec.push_back(addr1);
  vec.push_back(addr2);
  vec.push_back(addr3);
  vec.push_back(addr4);

  const OPENDDS_VECTOR(ACE_INET_Addr) vec1 = vec;
  const OPENDDS_VECTOR(ACE_INET_Addr) vec2 = vec;

  const ACE_INET_Addr result1 = choose_single_coherent_address(vec1, false);
  EXPECT_EQ(result1, addr3);

  const ACE_INET_Addr result2 = choose_single_coherent_address(vec2, true);
  EXPECT_EQ(result2, addr1);

  // No loop-back, only link-local
  vec.erase(vec.begin());

  const OPENDDS_VECTOR(ACE_INET_Addr) vec3 = vec;
  const OPENDDS_VECTOR(ACE_INET_Addr) vec4 = vec;

  const ACE_INET_Addr result3 = choose_single_coherent_address(vec3, false);
  EXPECT_EQ(result3, addr3);

  const ACE_INET_Addr result4 = choose_single_coherent_address(vec4, true);
  EXPECT_EQ(result4, addr2);
}
#endif // ACE_HAS_IPV6

TEST(dds_DCPS_NetworkResource, choose_single_coherent_address_localhost)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  ACE_INET_Addr addr1 = choose_single_coherent_address("localhost:5200", false);
  ACE_INET_Addr addr2 = choose_single_coherent_address("localhost:5200", true);
  EXPECT_NE(addr1, ACE_INET_Addr());
  EXPECT_NE(addr2, ACE_INET_Addr());
  EXPECT_EQ(addr1, addr2);
  EXPECT_EQ(addr2, addr1);
#if defined ACE_HAS_IPV6 && defined IPV6_V6ONLY
  EXPECT_FALSE(addr1.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr2.is_ipv4_mapped_ipv6());
#endif
}

TEST(dds_DCPS_NetworkResource, choose_single_coherent_address_double_self)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  String hostname = get_fully_qualified_hostname();
  ACE_INET_Addr addr1 = choose_single_coherent_address(hostname + ":5432", false);
  ACE_INET_Addr addr2 = choose_single_coherent_address(hostname + ":5432", false);
  EXPECT_NE(addr1, ACE_INET_Addr());
  EXPECT_NE(addr2, ACE_INET_Addr());
  EXPECT_EQ(addr1, addr2);
  EXPECT_EQ(addr2, addr1);
#if defined ACE_HAS_IPV6 && defined IPV6_V6ONLY
  EXPECT_FALSE(addr1.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr2.is_ipv4_mapped_ipv6());
#endif
}

#if defined ACE_HAS_IPV6
TEST(dds_DCPS_NetworkResource, choose_single_coherent_address_ipv6_literals)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  ACE_INET_Addr addr1 = choose_single_coherent_address("2001:4860:4860::8844:22", false);
  ACE_INET_Addr addr2 = choose_single_coherent_address("2001:4860:4860:0000:0000:0000:0000:8844:22", false);
  ACE_INET_Addr addr3 = choose_single_coherent_address("[2001:4860:4860::8844]:22", false);
  ACE_INET_Addr addr4 = choose_single_coherent_address("[2001:4860:4860:0000:0000:0000:0000:8844]:22", false);
  ACE_INET_Addr addr5 = choose_single_coherent_address("   [2001:4860:4860::8844]:22", false);
  ACE_INET_Addr addr6 = choose_single_coherent_address("  [2001:4860:4860:0000:0000:0000:0000:8844]:22", false);
  EXPECT_NE(addr1, ACE_INET_Addr());
  EXPECT_NE(addr2, ACE_INET_Addr());
  EXPECT_NE(addr3, ACE_INET_Addr());
  EXPECT_NE(addr4, ACE_INET_Addr());
  EXPECT_NE(addr5, ACE_INET_Addr());
  EXPECT_NE(addr6, ACE_INET_Addr());
  EXPECT_EQ(addr1, addr2);
  EXPECT_EQ(addr2, addr3);
  EXPECT_EQ(addr3, addr4);
  EXPECT_EQ(addr4, addr5);
  EXPECT_EQ(addr5, addr6);
  EXPECT_EQ(addr6, addr1);
  EXPECT_EQ(addr1.get_port_number(), 22);
  EXPECT_EQ(addr2.get_port_number(), 22);
  EXPECT_EQ(addr3.get_port_number(), 22);
  EXPECT_EQ(addr4.get_port_number(), 22);
  EXPECT_EQ(addr5.get_port_number(), 22);
  EXPECT_EQ(addr6.get_port_number(), 22);
#if defined ACE_HAS_IPV6 && defined IPV6_V6ONLY
  EXPECT_FALSE(addr1.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr2.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr3.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr4.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr5.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr6.is_ipv4_mapped_ipv6());
#endif
}

TEST(dds_DCPS_NetworkResource, choose_single_coherent_address_ipv6_literals_port0)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  ACE_INET_Addr addr1 = choose_single_coherent_address("2001:4860:4860::8844:0", false);
  ACE_INET_Addr addr2 = choose_single_coherent_address("2001:4860:4860:0000:0000:0000:0000:8844:0", false);
  ACE_INET_Addr addr3 = choose_single_coherent_address("[2001:4860:4860::8844]", false);
  ACE_INET_Addr addr4 = choose_single_coherent_address("[2001:4860:4860:0000:0000:0000:0000:8844]", false);
  ACE_INET_Addr addr5 = choose_single_coherent_address("[2001:4860:4860::8844]:0", false);
  ACE_INET_Addr addr6 = choose_single_coherent_address("[2001:4860:4860:0000:0000:0000:0000:8844]:0", false);
  EXPECT_NE(addr1, ACE_INET_Addr());
  EXPECT_NE(addr2, ACE_INET_Addr());
  EXPECT_NE(addr3, ACE_INET_Addr());
  EXPECT_NE(addr4, ACE_INET_Addr());
  EXPECT_NE(addr5, ACE_INET_Addr());
  EXPECT_NE(addr6, ACE_INET_Addr());
  EXPECT_EQ(addr1, addr2);
  EXPECT_EQ(addr2, addr3);
  EXPECT_EQ(addr3, addr4);
  EXPECT_EQ(addr4, addr5);
  EXPECT_EQ(addr5, addr6);
  EXPECT_EQ(addr6, addr1);
  EXPECT_EQ(addr1.get_port_number(), 0);
  EXPECT_EQ(addr2.get_port_number(), 0);
  EXPECT_EQ(addr3.get_port_number(), 0);
  EXPECT_EQ(addr4.get_port_number(), 0);
  EXPECT_EQ(addr5.get_port_number(), 0);
  EXPECT_EQ(addr6.get_port_number(), 0);
#if defined ACE_HAS_IPV6 && defined IPV6_V6ONLY
  EXPECT_FALSE(addr1.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr2.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr3.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr4.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr5.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr6.is_ipv4_mapped_ipv6());
#endif
}

TEST(dds_DCPS_NetworkResource, choose_single_coherent_address_ipv6_literals_localhost)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  ACE_INET_Addr addr1 = choose_single_coherent_address("::1:42", false); // This is ambiguous / malformed, but we should probably handle it
  ACE_INET_Addr addr2 = choose_single_coherent_address("[::1]:42", false);
  EXPECT_NE(addr1, ACE_INET_Addr());
  EXPECT_NE(addr2, ACE_INET_Addr());
  EXPECT_EQ(addr1, addr2);
  EXPECT_EQ(addr2, addr1);
  EXPECT_EQ(addr1.get_port_number(), 42);
  EXPECT_EQ(addr2.get_port_number(), 42);
#if defined ACE_HAS_IPV6 && defined IPV6_V6ONLY
  EXPECT_FALSE(addr1.is_ipv4_mapped_ipv6());
  EXPECT_FALSE(addr2.is_ipv4_mapped_ipv6());
#endif
}

TEST(dds_DCPS_NetworkResource, choose_single_coherent_address_ipv6_literals_localhost_port0)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  ACE_INET_Addr addr1 = choose_single_coherent_address("::1", false);
  EXPECT_NE(addr1, ACE_INET_Addr());
  EXPECT_EQ(addr1.get_port_number(), 0);
#if defined ACE_HAS_IPV6 && defined IPV6_V6ONLY
  EXPECT_FALSE(addr1.is_ipv4_mapped_ipv6());
#endif
}
#endif // ACE_HAS_IPV6

TEST(dds_DCPS_NetworkResource, choose_single_coherent_address_ipv4_literals)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  ACE_INET_Addr addr1 = choose_single_coherent_address("192.168.1.23:30", false);
  EXPECT_NE(addr1, ACE_INET_Addr());
  EXPECT_EQ(addr1.get_port_number(), 30);
}

TEST(dds_DCPS_NetworkResource, choose_single_coherent_address_ipv4_literals_port0)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  ACE_INET_Addr addr1 = choose_single_coherent_address("10.20.30.40", false);
  ACE_INET_Addr addr2 = choose_single_coherent_address("10.20.30.40:0", false);
  EXPECT_NE(addr1, ACE_INET_Addr());
  EXPECT_NE(addr2, ACE_INET_Addr());
  EXPECT_EQ(addr1, addr2);
  EXPECT_EQ(addr2, addr1);
  EXPECT_EQ(addr1.get_port_number(), 0);
  EXPECT_EQ(addr2.get_port_number(), 0);
}
