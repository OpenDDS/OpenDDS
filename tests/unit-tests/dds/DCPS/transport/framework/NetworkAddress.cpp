#include <dds/DCPS/transport/framework/NetworkAddress.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

// Test Utils

namespace {
class ScopedDebugLevels
{
public:
  explicit ScopedDebugLevels(int level) : previous_transport_debug_level_(OpenDDS::DCPS::Transport_debug_level) {
    ACE_DEBUG((LM_DEBUG, "BLURP\n"));
    OpenDDS::DCPS::Transport_debug_level = level;
  }
  ~ScopedDebugLevels() {
    ACE_DEBUG((LM_DEBUG, "BLAH\n"));
    OpenDDS::DCPS::Transport_debug_level = previous_transport_debug_level_;
  }
private:
  ScopedDebugLevels();

  int previous_transport_debug_level_;
};
}

// Tests

TEST(network_address_test, fully_qualified_domain_hostname_basic)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  OPENDDS_STRING empty;
  OPENDDS_STRING hostname = get_fully_qualified_hostname();
  EXPECT_NE(hostname, empty);
}

TEST(network_address_test, choose_single_coherent_address_ipv4)
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
TEST(network_address_test, choose_single_coherent_address_ipv6)
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

TEST(network_addres_test, choose_single_coherent_address_double)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

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
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

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
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

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
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

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
