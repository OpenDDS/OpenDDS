#include <dds/DCPS/NetworkResource.h>
#include <dds/DCPS/transport/framework/TransportDebug.h>

#include <ace/Init_ACE.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

// Test Utils

namespace {
class ScopedDebugLevels
{
public:
  explicit ScopedDebugLevels(int level) : previous_transport_debug_level_(OpenDDS::DCPS::Transport_debug_level) {
    OpenDDS::DCPS::Transport_debug_level = level;
  }
  ~ScopedDebugLevels() {
    OpenDDS::DCPS::Transport_debug_level = previous_transport_debug_level_;
  }
private:
  ScopedDebugLevels();

  int previous_transport_debug_level_;
};
}

// Tests

TEST(dds_DCPS_NetworkResource, choose_single_coherent_address_double)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  ACE_INET_Addr addr1 = choose_single_coherent_address("www.bizinta.com:80", false);
  ACE_INET_Addr addr2 = choose_single_coherent_address("www.bizinta.com:80", false);

  if (addr1 == ACE_INET_Addr()) {
    EXPECT_NE(addr2, ACE_INET_Addr());
  } else {
    EXPECT_NE(addr2, ACE_INET_Addr());
    EXPECT_EQ(addr1, addr2);
    EXPECT_EQ(addr2, addr1);
#if defined ACE_HAS_IPV6 && defined IPV6_V6ONLY
    EXPECT_FALSE(addr1.is_ipv4_mapped_ipv6());
    EXPECT_FALSE(addr2.is_ipv4_mapped_ipv6());
#endif
  }
}

TEST(dds_DCPS_NetworkResource, choose_single_coherent_address_triple)
{
  //ScopedDebugLevels sdl(6); // Uncomment for greater debug levels

  ACE_INET_Addr addr1 = choose_single_coherent_address("www.hp.com:587", false);
  ACE_INET_Addr addr2 = choose_single_coherent_address("www.hp.com:587", false);
  ACE_INET_Addr addr3 = choose_single_coherent_address("www.hp.com:587", false);

  if (addr1 == ACE_INET_Addr()) {
    EXPECT_EQ(addr2, ACE_INET_Addr());
    EXPECT_EQ(addr3, ACE_INET_Addr());
  } else {
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
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
