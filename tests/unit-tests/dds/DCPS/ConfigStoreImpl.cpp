#include <dds/DCPS/ConfigStoreImpl.h>

#include <dds/DCPS/Qos_Helper.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_ConfigPair, ctor)
{
  {
    ConfigPair cp("", "value");
    EXPECT_EQ(cp.key(), "");
    EXPECT_EQ(cp.value(), "value");
  }

  {
    ConfigPair cp("^", "value");
    EXPECT_EQ(cp.key(), "");
    EXPECT_EQ(cp.value(), "value");
  }

  {
    ConfigPair cp("^&*", "value");
    EXPECT_EQ(cp.key(), "");
    EXPECT_EQ(cp.value(), "value");
  }

  {
    ConfigPair cp("~!abc.123__CamelCase/CAMELCase#$%", "value");
    EXPECT_EQ(cp.key(), "ABC_123_CAMEL_CASE_CAMEL_CASE");
    EXPECT_EQ(cp.value(), "value");
  }

  {
    ConfigPair cp("CamelCase", "value");
    EXPECT_EQ(cp.key(), "CAMEL_CASE");
    EXPECT_EQ(cp.value(), "value");
  }

  {
    ConfigPair cp("##CamelCase##", "value");
    EXPECT_EQ(cp.key(), "CAMEL_CASE");
    EXPECT_EQ(cp.value(), "value");
  }
}

TEST(dds_DCPS_ConfigPair, key_has_prefix)
{
  ConfigPair cp("key", "value");
  EXPECT_TRUE(cp.key_has_prefix(""));
  EXPECT_TRUE(cp.key_has_prefix("k"));
  EXPECT_TRUE(cp.key_has_prefix("ke"));
  EXPECT_TRUE(cp.key_has_prefix("key"));
  EXPECT_FALSE(cp.key_has_prefix("keya"));
  EXPECT_FALSE(cp.key_has_prefix("a"));
}

TEST(dds_DCPS_ConfigStoreImpl, has)
{
  ConfigStoreImpl store;
  EXPECT_FALSE(store.has("key"));
  store.set_boolean("key", true);
  EXPECT_TRUE(store.has("key"));
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_boolean)
{
  ConfigStoreImpl store;
  EXPECT_TRUE(store.get_boolean("key", true));
  EXPECT_FALSE(store.get_boolean("key", false));
  store.set_boolean("key", "true");
  EXPECT_TRUE(store.get_boolean("key", false));
  store.set_string("key", "not a boolean");
  EXPECT_TRUE(store.get_boolean("key", true));
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_int32)
{
  ConfigStoreImpl store;
  EXPECT_EQ(store.get_int32("key", -37), -37);
  store.set_int32("key", -38);
  EXPECT_EQ(store.get_int32("key", -37), -38);
  store.set_string("key", "not an int32");
  EXPECT_EQ(store.get_int32("key", -37), -37);
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_uint32)
{
  ConfigStoreImpl store;
  EXPECT_EQ(store.get_uint32("key", 37), 37);
  store.set_uint32("key", 38);
  EXPECT_EQ(store.get_uint32("key", 37), 38);
  store.set_string("key", "not a uint32");
  EXPECT_EQ(store.get_uint32("key", 37), 37);
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_float64)
{
  ConfigStoreImpl store;
  EXPECT_EQ(store.get_float64("key", 1.5), 1.5);
  store.set_float64("key", 2);
  EXPECT_EQ(store.get_float64("key", 1.5), 2);
  store.set_string("key", "not a float64");
  EXPECT_EQ(store.get_float64("key", 1.5), 1.5);
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_string)
{
  ConfigStoreImpl store;
  CORBA::String_var str = store.get_string("key", "default");
  EXPECT_STREQ(str, "default");
  store.set_string("key", "not default");
  str = store.get_string("key", "default");
  EXPECT_STREQ(str, "not default");
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_duration)
{
  using OpenDDS::DCPS::operator==;

  ConfigStoreImpl store;
  const DDS::Duration_t default_duration = make_duration_t(1,2);
  const DDS::Duration_t duration = make_duration_t(3,4);
  EXPECT_TRUE(store.get_duration("key", default_duration) == default_duration);
  store.set_duration("key", duration);
  EXPECT_TRUE(store.get_duration("key", default_duration) == duration);
  store.set_string("key", "not a duration");
  EXPECT_TRUE(store.get_duration("key", default_duration) == default_duration);
}

TEST(dds_DCPS_ConfigStoreImpl, unset)
{
  ConfigStoreImpl store;
  store.set_boolean("key", true);
  store.unset("key");
  EXPECT_FALSE(store.has("key"));
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_String)
{
  ConfigStoreImpl store;
  const String default_string = "default";
  const String other_string = "other";
  EXPECT_EQ(store.get("key", default_string), default_string);
  store.set("key", other_string);
  EXPECT_EQ(store.get("key", default_string), other_string);
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_TimeDuration_seconds)
{
  ConfigStoreImpl store;
  const TimeDuration default_duration(1,2);
  const TimeDuration duration(3);
  EXPECT_EQ(store.get("key", default_duration, ConfigStoreImpl::Format_IntegerSeconds), default_duration);
  store.set("key", duration, ConfigStoreImpl::Format_IntegerSeconds);
  EXPECT_EQ(store.get("key", default_duration, ConfigStoreImpl::Format_IntegerSeconds), duration);
  store.set_string("key", "not a duration");
  EXPECT_EQ(store.get("key", default_duration, ConfigStoreImpl::Format_IntegerSeconds), default_duration);
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_TimeDuration_milliseconds)
{
  ConfigStoreImpl store;
  const TimeDuration default_duration(1,2);
  const TimeDuration duration(3,4000);
  EXPECT_EQ(store.get("key", default_duration, ConfigStoreImpl::Format_IntegerMilliseconds), default_duration);
  store.set("key", duration, ConfigStoreImpl::Format_IntegerMilliseconds);
  EXPECT_EQ(store.get("key", default_duration, ConfigStoreImpl::Format_IntegerMilliseconds), duration);
  store.set_string("key", "not a duration");
  EXPECT_EQ(store.get("key", default_duration, ConfigStoreImpl::Format_IntegerMilliseconds), default_duration);
}

TEST(dds_DCPS_ConfigStoreImpl, get_NetworkAddress)
{
  const NetworkAddress default_value;
  const NetworkAddress value("127.0.0.1:0");
  ConfigStoreImpl store;
  EXPECT_EQ(store.get("key", default_value), default_value);
  store.set("key", "127.0.0.1");
  EXPECT_EQ(store.get("key", default_value), value);
  store.set_string("key", "not a network address");
  EXPECT_EQ(store.get("key", default_value), default_value);
}

TEST(dds_DCPS_ConfigStoreImpl, connect_disconnect)
{
  ConfigStoreImpl store;
  ConfigReader_rch reader = make_rch<ConfigReader>(DataReaderQosBuilder());

  store.connect(reader);
  store.set_string("key", "value1");
  ConfigReader::SampleSequence datas;
  InternalSampleInfoSequence infos;
  reader->take(datas, infos, DDS::LENGTH_UNLIMITED,
               DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  bool found_it = false;
  for (size_t idx = 0; !found_it && idx != datas.size(); ++idx) {
    if (datas[idx] == ConfigPair("key", "value1")) {
      found_it = true;
    }
  }
  EXPECT_TRUE(found_it);
  store.disconnect(reader);
  store.set_string("key", "value2");
  reader->take(datas, infos, DDS::LENGTH_UNLIMITED,
               DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  found_it = false;
  for (size_t idx = 0; !found_it && idx != datas.size(); ++idx) {
    if (datas[idx] == ConfigPair("key", "value2")) {
      found_it = true;
    }
  }
  EXPECT_FALSE(found_it);
}

TEST(dds_DCPS_ConfigStoreImpl, contains_prefix)
{
  ConfigStoreImpl store;
  ConfigReader_rch reader = make_rch<ConfigReader>(store.datareader_qos());
  store.connect(reader);

  store.set("key", "value");
  EXPECT_TRUE(ConfigStoreImpl::contains_prefix(reader, "ke"));
  store.set("key", "value");
  EXPECT_FALSE(ConfigStoreImpl::contains_prefix(reader, "notkey"));

  store.disconnect(reader);
}
