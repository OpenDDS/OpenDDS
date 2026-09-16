#include <dds/DCPS/LinuxNetworkConfigMonitor.h>

#ifdef OPENDDS_LINUX_NETWORK_CONFIG_MONITOR

#include <dds/DCPS/InternalDataReader.h>
#include <dds/DCPS/InternalTopic.h>
#include <dds/DCPS/Qos_Helper.h>

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <net/if.h>

#include <cstring>
#include <vector>

using namespace OpenDDS::DCPS;

namespace {

// Builds a single netlink message (header + payload struct + attributes) in
// a scratch buffer, following the same NLMSG_*/RTA_* macros that the kernel
// and LinuxNetworkConfigMonitor::process_message() use to parse one.
class NetlinkMessage {
public:
  NetlinkMessage(unsigned short type, size_t payload_size)
    : buffer_(1024, '\0')
  {
    header()->nlmsg_type = type;
    header()->nlmsg_len = static_cast<__u32>(NLMSG_LENGTH(payload_size));
  }

  nlmsghdr* header() { return reinterpret_cast<nlmsghdr*>(&buffer_[0]); }
  void* payload() { return NLMSG_DATA(header()); }

  void add_attr(unsigned short rta_type, const void* data, size_t len)
  {
    nlmsghdr* nlh = header();
    rtattr* rta = reinterpret_cast<rtattr*>(reinterpret_cast<char*>(nlh) + NLMSG_ALIGN(nlh->nlmsg_len));
    rta->rta_type = rta_type;
    rta->rta_len = static_cast<unsigned short>(RTA_LENGTH(len));
    if (len) {
      std::memcpy(RTA_DATA(rta), data, len);
    }
    nlh->nlmsg_len = static_cast<__u32>(NLMSG_ALIGN(nlh->nlmsg_len) + RTA_ALIGN(rta->rta_len));
  }

private:
  std::vector<char> buffer_;
};

const unsigned int UP_FLAGS = IFF_UP | IFF_RUNNING | IFF_MULTICAST;
const unsigned int DOWN_FLAGS = IFF_MULTICAST;
const unsigned int ADMIN_UP_NO_CARRIER_FLAGS = IFF_UP | IFF_MULTICAST;

NetlinkMessage make_link_message(unsigned short type, int ifi_index, const char* name, unsigned int ifi_flags)
{
  NetlinkMessage msg(type, sizeof(ifinfomsg));
  ifinfomsg* ifi = reinterpret_cast<ifinfomsg*>(msg.payload());
  ifi->ifi_index = ifi_index;
  ifi->ifi_flags = ifi_flags;
  if (name) {
    msg.add_attr(IFLA_IFNAME, name, std::strlen(name) + 1);
  }
  return msg;
}

in_addr make_ipv4(const char* ip)
{
  in_addr addr = in_addr();
  inet_pton(AF_INET, ip, &addr);
  return addr;
}

NetlinkMessage make_addr_message(unsigned short type, int ifa_index, const char* ip)
{
  const in_addr raw = make_ipv4(ip);
  NetlinkMessage msg(type, sizeof(ifaddrmsg));
  ifaddrmsg* ifa = reinterpret_cast<ifaddrmsg*>(msg.payload());
  ifa->ifa_family = AF_INET;
  ifa->ifa_prefixlen = 32;
  ifa->ifa_index = static_cast<unsigned int>(ifa_index);
  msg.add_attr(IFA_ADDRESS, &raw, sizeof(raw));
  return msg;
}

// Mirrors how process_message() itself turns the raw attribute bytes into a
// NetworkAddress, so the expected value here can't drift from the address
// bytes put on the wire by make_addr_message().
NetworkAddress expected_address(const char* ip)
{
  const in_addr raw = make_ipv4(ip);
  ACE_INET_Addr addr;
  addr.set_address(reinterpret_cast<const char*>(&raw), static_cast<int>(sizeof(raw)), 0);
  return NetworkAddress(addr);
}

class TestLinuxNetworkConfigMonitor : public LinuxNetworkConfigMonitor {
public:
  TestLinuxNetworkConfigMonitor()
    : LinuxNetworkConfigMonitor(ReactorTask_rch())
  {}

  using LinuxNetworkConfigMonitor::process_message;
};

typedef InternalDataReader<NetworkInterfaceAddress> ReaderType;

struct Fixture {
  RcHandle<InternalTopic<NetworkInterfaceAddress> > topic;
  RcHandle<ReaderType> reader;
  RcHandle<TestLinuxNetworkConfigMonitor> lncm;

  Fixture()
    : topic(make_rch<InternalTopic<NetworkInterfaceAddress> >())
    , reader(make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable()))
    , lncm(make_rch<TestLinuxNetworkConfigMonitor>())
  {
    topic->connect(reader);
    lncm->connect(topic);
  }

  void process(NetlinkMessage& msg)
  {
    lncm->process_message(msg.header());
  }

  void take(ReaderType::SampleSequence& samples, InternalSampleInfoSequence& infos)
  {
    reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  }

  void drain()
  {
    ReaderType::SampleSequence samples;
    InternalSampleInfoSequence infos;
    take(samples, infos);
  }
};

} // namespace

TEST(dds_DCPS_LinuxNetworkConfigMonitor, new_addr_after_link_up_sets_address)
{
  Fixture f;
  NetlinkMessage link_up = make_link_message(RTM_NEWLINK, 1, "eth0", UP_FLAGS);
  f.process(link_up);
  NetlinkMessage addr = make_addr_message(RTM_NEWADDR, 1, "192.168.1.5");
  f.process(addr);

  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);
  ASSERT_EQ(samples.size(), 1u);
  EXPECT_EQ(samples[0].name, "eth0");
  EXPECT_TRUE(samples[0].can_multicast);
  EXPECT_EQ(samples[0].address, expected_address("192.168.1.5"));
}

// This is the scenario the link-flap fix addresses: NetworkManager can bring
// a link down and back up so quickly that it never re-announces the address
// via RTM_NEWADDR, since the address never actually left the interface.
TEST(dds_DCPS_LinuxNetworkConfigMonitor, link_flap_rejoins_cached_address_without_new_addr)
{
  Fixture f;
  NetlinkMessage link_up = make_link_message(RTM_NEWLINK, 1, "eth0", UP_FLAGS);
  f.process(link_up);
  NetlinkMessage addr = make_addr_message(RTM_NEWADDR, 1, "192.168.1.5");
  f.process(addr);
  f.drain();

  NetlinkMessage link_down = make_link_message(RTM_NEWLINK, 1, "eth0", DOWN_FLAGS);
  f.process(link_down);

  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);
  ASSERT_EQ(samples.size(), 1u);
  EXPECT_EQ(infos[0].instance_state, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  // No RTM_NEWADDR here: the address should still be rejoined from cache.
  NetlinkMessage link_up_again = make_link_message(RTM_NEWLINK, 1, "eth0", UP_FLAGS);
  f.process(link_up_again);

  f.take(samples, infos);
  ASSERT_EQ(samples.size(), 1u);
  EXPECT_EQ(infos[0].instance_state, DDS::ALIVE_INSTANCE_STATE);
  EXPECT_EQ(samples[0].name, "eth0");
  EXPECT_EQ(samples[0].address, expected_address("192.168.1.5"));
}

TEST(dds_DCPS_LinuxNetworkConfigMonitor, del_addr_removes_cached_address_so_flap_does_not_rejoin)
{
  Fixture f;
  NetlinkMessage link_up = make_link_message(RTM_NEWLINK, 1, "eth0", UP_FLAGS);
  f.process(link_up);
  NetlinkMessage addr = make_addr_message(RTM_NEWADDR, 1, "192.168.1.5");
  f.process(addr);
  NetlinkMessage del = make_addr_message(RTM_DELADDR, 1, "192.168.1.5");
  f.process(del);
  f.drain();

  NetlinkMessage link_down = make_link_message(RTM_NEWLINK, 1, "eth0", DOWN_FLAGS);
  f.process(link_down);
  f.drain();
  NetlinkMessage link_up_again = make_link_message(RTM_NEWLINK, 1, "eth0", UP_FLAGS);
  f.process(link_up_again);

  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);
  EXPECT_EQ(samples.size(), 0u);
}

// IFF_UP alone (administratively up, no carrier) must not be mistaken for
// the link being usable.
TEST(dds_DCPS_LinuxNetworkConfigMonitor, admin_up_without_carrier_is_treated_as_down)
{
  Fixture f;
  NetlinkMessage link_up = make_link_message(RTM_NEWLINK, 1, "eth0", UP_FLAGS);
  f.process(link_up);
  NetlinkMessage addr = make_addr_message(RTM_NEWADDR, 1, "192.168.1.5");
  f.process(addr);
  f.drain();

  NetlinkMessage carrier_lost = make_link_message(RTM_NEWLINK, 1, "eth0", ADMIN_UP_NO_CARRIER_FLAGS);
  f.process(carrier_lost);

  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);
  ASSERT_EQ(samples.size(), 1u);
  EXPECT_EQ(infos[0].instance_state, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
}

TEST(dds_DCPS_LinuxNetworkConfigMonitor, rename_unregisters_old_name_and_starts_fresh)
{
  Fixture f;
  NetlinkMessage link_up = make_link_message(RTM_NEWLINK, 1, "eth0", UP_FLAGS);
  f.process(link_up);
  NetlinkMessage addr = make_addr_message(RTM_NEWADDR, 1, "192.168.1.5");
  f.process(addr);
  f.drain();

  NetlinkMessage rename = make_link_message(RTM_NEWLINK, 1, "eth1", UP_FLAGS);
  f.process(rename);

  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);
  ASSERT_EQ(samples.size(), 1u);
  EXPECT_EQ(samples[0].name, "eth0");
  EXPECT_EQ(infos[0].instance_state, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  // The rename started a fresh, empty address cache under the new name, so a
  // flap on it must not rejoin the old interface's address.
  NetlinkMessage link_down = make_link_message(RTM_NEWLINK, 1, "eth1", DOWN_FLAGS);
  f.process(link_down);
  f.drain();
  NetlinkMessage link_up_again = make_link_message(RTM_NEWLINK, 1, "eth1", UP_FLAGS);
  f.process(link_up_again);
  f.take(samples, infos);
  EXPECT_EQ(samples.size(), 0u);
}

TEST(dds_DCPS_LinuxNetworkConfigMonitor, dellink_unregisters_interface)
{
  Fixture f;
  NetlinkMessage link_up = make_link_message(RTM_NEWLINK, 1, "eth0", UP_FLAGS);
  f.process(link_up);
  NetlinkMessage addr = make_addr_message(RTM_NEWADDR, 1, "192.168.1.5");
  f.process(addr);
  f.drain();

  NetlinkMessage del_link = make_link_message(RTM_DELLINK, 1, static_cast<const char*>(0), 0);
  f.process(del_link);

  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);
  ASSERT_EQ(samples.size(), 1u);
  EXPECT_EQ(infos[0].instance_state, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
}

#endif // OPENDDS_LINUX_NETWORK_CONFIG_MONITOR
