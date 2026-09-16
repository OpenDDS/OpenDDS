#include <dds/DCPS/NetworkConfigMonitor.h>

#include <dds/DCPS/InternalDataReader.h>
#include <dds/DCPS/InternalTopic.h>
#include <dds/DCPS/Qos_Helper.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

namespace {

class TestNetworkConfigMonitor : public NetworkConfigMonitor {
public:
  bool open() { return true; }
  bool close() { return true; }

  using NetworkConfigMonitor::set;
  using NetworkConfigMonitor::clear;
  using NetworkConfigMonitor::remove_interface;
  using NetworkConfigMonitor::remove_address;
};

typedef InternalDataReader<NetworkInterfaceAddress> ReaderType;

NetworkAddress make_addr(const char* ip)
{
  return NetworkAddress(ACE_INET_Addr(static_cast<unsigned short>(0), ip));
}

struct Fixture {
  RcHandle<InternalTopic<NetworkInterfaceAddress> > topic;
  RcHandle<ReaderType> reader;
  RcHandle<TestNetworkConfigMonitor> ncm;

  Fixture()
    : topic(make_rch<InternalTopic<NetworkInterfaceAddress> >())
    , reader(make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable()))
    , ncm(make_rch<TestNetworkConfigMonitor>())
  {
    topic->connect(reader);
    ncm->connect(topic);
  }

  // Drains and returns everything currently in the reader.
  void take(ReaderType::SampleSequence& samples, InternalSampleInfoSequence& infos)
  {
    reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  }

  size_t count_with_state(DDS::InstanceStateKind state)
  {
    ReaderType::SampleSequence samples;
    InternalSampleInfoSequence infos;
    take(samples, infos);
    size_t count = 0;
    for (size_t i = 0; i < infos.size(); ++i) {
      if (infos[i].instance_state == state) {
        ++count;
      }
    }
    return count;
  }
};

} // namespace

TEST(dds_DCPS_NetworkConfigMonitor, set_new_writes_sample)
{
  Fixture f;
  const NetworkInterfaceAddress nia("eth0", true, make_addr("127.0.0.1"));

  f.ncm->set(nia);

  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);
  ASSERT_EQ(samples.size(), 1u);
  EXPECT_EQ(samples[0], nia);
}

TEST(dds_DCPS_NetworkConfigMonitor, set_same_value_does_not_rewrite)
{
  Fixture f;
  const NetworkInterfaceAddress nia("eth0", true, make_addr("127.0.0.1"));

  f.ncm->set(nia);
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);
  ASSERT_EQ(samples.size(), 1u);

  // Same key and value: should not produce another write.
  f.ncm->set(nia);
  f.take(samples, infos);
  EXPECT_EQ(samples.size(), 0u);
}

TEST(dds_DCPS_NetworkConfigMonitor, set_changed_value_rewrites)
{
  Fixture f;
  const NetworkInterfaceAddress nia1("eth0", true, make_addr("127.0.0.1"));
  const NetworkInterfaceAddress nia2("eth0", false, make_addr("127.0.0.1"));

  f.ncm->set(nia1);
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);
  ASSERT_EQ(samples.size(), 1u);

  // Same key (name + address), different can_multicast: should rewrite.
  f.ncm->set(nia2);
  f.take(samples, infos);
  ASSERT_EQ(samples.size(), 1u);
  EXPECT_EQ(samples[0], nia2);
}

TEST(dds_DCPS_NetworkConfigMonitor, remove_interface_unregisters_matching_addresses_only)
{
  Fixture f;
  const NetworkInterfaceAddress eth0_a("eth0", true, make_addr("127.0.0.1"));
  const NetworkInterfaceAddress eth0_b("eth0", true, make_addr("127.0.0.2"));
  const NetworkInterfaceAddress eth1_a("eth1", true, make_addr("127.0.0.3"));

  f.ncm->set(eth0_a);
  f.ncm->set(eth0_b);
  f.ncm->set(eth1_a);
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);

  f.ncm->remove_interface("eth0");

  EXPECT_EQ(f.count_with_state(DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE), 2u);
}

TEST(dds_DCPS_NetworkConfigMonitor, remove_address_unregisters_matching_address_only)
{
  Fixture f;
  const NetworkInterfaceAddress eth0_a("eth0", true, make_addr("127.0.0.1"));
  const NetworkInterfaceAddress eth0_b("eth0", true, make_addr("127.0.0.2"));

  f.ncm->set(eth0_a);
  f.ncm->set(eth0_b);
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);

  f.ncm->remove_address("eth0", eth0_a.address);

  EXPECT_EQ(f.count_with_state(DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE), 1u);
}

TEST(dds_DCPS_NetworkConfigMonitor, clear_unregisters_all)
{
  Fixture f;
  const NetworkInterfaceAddress eth0_a("eth0", true, make_addr("127.0.0.1"));
  const NetworkInterfaceAddress eth1_a("eth1", true, make_addr("127.0.0.3"));

  f.ncm->set(eth0_a);
  f.ncm->set(eth1_a);
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);

  f.ncm->clear();

  EXPECT_EQ(f.count_with_state(DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE), 2u);
}

TEST(dds_DCPS_NetworkConfigMonitor, set_list_adds_changes_and_removes)
{
  Fixture f;
  const NetworkInterfaceAddress eth0_a("eth0", true, make_addr("127.0.0.1"));
  const NetworkInterfaceAddress eth1_a("eth1", true, make_addr("127.0.0.3"));

  NetworkConfigMonitor::List list1;
  list1.push_back(eth0_a);
  list1.push_back(eth1_a);
  f.ncm->set(list1);

  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  f.take(samples, infos);
  ASSERT_EQ(samples.size(), 2u);

  // eth1 dropped, eth0 changed.
  const NetworkInterfaceAddress eth0_b("eth0", false, make_addr("127.0.0.1"));
  NetworkConfigMonitor::List list2;
  list2.push_back(eth0_b);
  f.ncm->set(list2);

  f.take(samples, infos);
  ASSERT_EQ(samples.size(), 2u);
  ASSERT_EQ(infos.size(), 2u);

  bool found_eth0_change = false;
  bool found_eth1_removal = false;
  for (size_t i = 0; i < samples.size(); ++i) {
    if (samples[i].name == "eth0") {
      EXPECT_EQ(infos[i].instance_state, DDS::ALIVE_INSTANCE_STATE);
      EXPECT_EQ(samples[i], eth0_b);
      found_eth0_change = true;
    } else if (samples[i].name == "eth1") {
      EXPECT_EQ(infos[i].instance_state, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
      found_eth1_removal = true;
    }
  }
  EXPECT_TRUE(found_eth0_change);
  EXPECT_TRUE(found_eth1_removal);
}
