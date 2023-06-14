#include <dds/DCPS/transport/framework/MessageDropper.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_transport_framework_MessageDropper, ctor)
{
  MessageDropper uut;
  EXPECT_FALSE(uut.drop_messages());
  EXPECT_EQ(uut.drop_messages_m(), 0.0);
  EXPECT_EQ(uut.drop_messages_b(), 0.0);
}

TEST(dds_DCPS_transport_framework_MessageDropper, reload)
{
  MessageDropper uut;

  RcHandle<ConfigStoreImpl> config_store = make_rch<ConfigStoreImpl>();
  config_store->set_boolean("key_DROP_MESSAGES", true);
  config_store->set_float64("key_DROP_MESSAGES_M", 1.0);
  config_store->set_float64("key_DROP_MESSAGES_B", 1.0);
  uut.reload(config_store, "key");

  EXPECT_TRUE(uut.drop_messages());
  EXPECT_EQ(uut.drop_messages_m(), 1.0);
  EXPECT_EQ(uut.drop_messages_b(), 1.0);
}
