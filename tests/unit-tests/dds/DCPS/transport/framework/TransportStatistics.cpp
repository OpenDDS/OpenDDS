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

  RcHandle<ConfigStoreImpl> config_store = make_rch<ConfigStoreImpl>();
  config_store->set_boolean("key_COUNT_MESSAGES", true);
  uut.reload(config_store, "key");

  EXPECT_TRUE(uut.count_messages());
}
