#include <dds/DCPS/transport/framework/TransportStatistics.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_transport_framework_InternalTransportStatistics, ctor)
{
  InternalTransportStatistics uut("a transport");
  EXPECT_FALSE(uut.count_messages());
}

TEST(dds_DCPS_transport_framework_InternalTransportStatistics, reload)
{
  InternalTransportStatistics uut("a transport");

  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  RcHandle<ConfigStoreImpl> config_store = make_rch<ConfigStoreImpl>(topic);
  config_store->set_boolean("KEY_PREFIX_COUNT_MESSAGES", true);
  uut.reload(config_store, "KEY_PREFIX");

  EXPECT_TRUE(uut.count_messages());
}
