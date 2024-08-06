#include <dds/DCPS/ConfigStoreImpl.h>

#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/LogAddr.h>

#include <gtestWrapper.h>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_ConfigPair, split)
{
  typedef OPENDDS_VECTOR(String) StringVec;
  {
    const StringVec actual = split("a,b,c", ",", false, false);
    ASSERT_EQ(actual.size(), 3u);
    EXPECT_EQ(actual[0], "a");
    EXPECT_EQ(actual[1], "b");
    EXPECT_EQ(actual[2], "c");
  }
  {
    const StringVec actual = split("a,,b,c,", ",", false, false);
    ASSERT_EQ(actual.size(), 5u);
    EXPECT_EQ(actual[0], "a");
    EXPECT_EQ(actual[1], "");
    EXPECT_EQ(actual[2], "b");
    EXPECT_EQ(actual[3], "c");
    EXPECT_EQ(actual[4], "");
  }
  {
    const StringVec actual = split("a,b,c", " ", false, false);
    ASSERT_EQ(actual.size(), 1u);
    EXPECT_EQ(actual[0], "a,b,c");
  }
  {
    const StringVec actual = split("a b,c", ", ", false, false);
    ASSERT_EQ(actual.size(), 3u);
    EXPECT_EQ(actual[0], "a");
    EXPECT_EQ(actual[1], "b");
    EXPECT_EQ(actual[2], "c");
  }
  {
    const StringVec actual = split("  ,a b,c", ", ", true, false);
    ASSERT_EQ(actual.size(), 3u);
    EXPECT_EQ(actual[0], "a");
    EXPECT_EQ(actual[1], "b");
    EXPECT_EQ(actual[2], "c");
  }
  {
    const StringVec actual = split("  ,a b, ,c", ", ", true, true);
    ASSERT_EQ(actual.size(), 3u);
    EXPECT_EQ(actual[0], "a");
    EXPECT_EQ(actual[1], "b");
    EXPECT_EQ(actual[2], "c");
  }
}

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

  {
    ConfigPair cp("UseXTypes", "complete");
    EXPECT_EQ(cp.key(), "USE_X_TYPES");
    EXPECT_EQ(cp.value(), "complete");
  }

  {
    ConfigPair cp("UseXYZTypes", "complete");
    EXPECT_EQ(cp.key(), "USE_XYZ_TYPES");
    EXPECT_EQ(cp.value(), "complete");
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
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
  EXPECT_FALSE(store.has("key"));
  store.set_boolean("key", true);
  EXPECT_TRUE(store.has("key"));
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_boolean)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
  EXPECT_TRUE(store.get_boolean("key", true));
  EXPECT_FALSE(store.get_boolean("key", false));
  store.set_boolean("key", "true");
  EXPECT_TRUE(store.get_boolean("key", false));
  store.set_string("key", "not a boolean");
  EXPECT_TRUE(store.get_boolean("key", true));
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_int32)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
  EXPECT_EQ(store.get_int32("key", -37), -37);
  store.set_int32("key", -38);
  EXPECT_EQ(store.get_int32("key", -37), -38);
  store.set_string("key", "not an int32");
  EXPECT_EQ(store.get_int32("key", -37), -37);
  store.set_string("key", "DURATION_INFINITE_SEC");
  EXPECT_EQ(store.get_int32("key", 0), DDS::DURATION_INFINITE_SEC);
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_uint32)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
  EXPECT_EQ(store.get_uint32("key", 37), 37u);
  store.set_uint32("key", 38);
  EXPECT_EQ(store.get_uint32("key", 37), 38u);
  store.set_string("key", "not a uint32");
  EXPECT_EQ(store.get_uint32("key", 37), 37u);
  store.set_string("key", "DURATION_INFINITE_NANOSEC");
  EXPECT_EQ(store.get_uint32("key", 0), DDS::DURATION_INFINITE_NSEC);
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_float64)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
  EXPECT_EQ(store.get_float64("key", 1.5), 1.5);
  store.set_float64("key", 2);
  EXPECT_EQ(store.get_float64("key", 1.5), 2);
  store.set_string("key", "not a float64");
  EXPECT_EQ(store.get_float64("key", 1.5), 1.5);
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_string)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
  CORBA::String_var str = store.get_string("key", "default");
  EXPECT_STREQ(str, "default");
  store.set_string("key", "not default");
  str = store.get_string("key", "default");
  EXPECT_STREQ(str, "not default");
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_duration)
{
  using OpenDDS::DCPS::operator==;

  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
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
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
  store.set_boolean("key", true);
  store.unset("key");
  EXPECT_FALSE(store.has("key"));
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_String)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
  const String default_string = "default";
  const String other_string = "other";
  EXPECT_EQ(store.get("key", default_string), default_string);
  store.set("key", other_string);
  EXPECT_EQ(store.get("key", default_string), other_string);

  store.set("key", "");
  EXPECT_EQ(store.get("key", default_string, false), default_string);
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_StringList)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
  ConfigStoreImpl::StringList default_list;
  default_list.push_back("Lorem");
  default_list.push_back("ipsum");
  default_list.push_back("dolor");

  ConfigStoreImpl::StringList other_list;
  other_list.push_back("other");
  EXPECT_EQ(store.get("key", default_list), default_list);
  store.set("key", other_list);
  EXPECT_EQ(store.get("key", default_list), other_list);
}

enum MyConfigStoreEnum {
  ALPHA,
  BETA,
  GAMMA,
  DELTA
};

TEST(dds_DCPS_ConfigStoreImpl, set_get_Enum)
{
  const EnumList<MyConfigStoreEnum> kinds[] =
    {
      { ALPHA, "alpha" },
      { BETA, "beta" },
      { GAMMA, "gamma" }
    };

  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
  // Get the default if there is no entry.
  EXPECT_EQ(store.get("key", GAMMA, kinds), GAMMA);

  // Default not in helper works.
  EXPECT_EQ(store.get("key", DELTA, kinds), DELTA);

  // Setting with enum works.
  store.set("key", ALPHA, kinds);
  EXPECT_EQ(store.get("key", GAMMA, kinds), ALPHA);

  // Setting with string works.
  store.set("key", "beta", kinds);
  EXPECT_EQ(store.get("key", GAMMA, kinds), BETA);

  // Setting with enum that is not in helper does nothing.
  store.set("key", DELTA, kinds);
  EXPECT_EQ(store.get("key", GAMMA, kinds), BETA);

  // Setting with enum that is not in helper does nothing.
  store.set("key", "delta", kinds);
  EXPECT_EQ(store.get("key", GAMMA, kinds), BETA);
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_TimeDuration_seconds)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
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
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
  const TimeDuration default_duration(1,2);
  const TimeDuration duration(3,4000);
  EXPECT_EQ(store.get("key", default_duration, ConfigStoreImpl::Format_IntegerMilliseconds), default_duration);
  store.set("key", duration, ConfigStoreImpl::Format_IntegerMilliseconds);
  EXPECT_EQ(store.get("key", default_duration, ConfigStoreImpl::Format_IntegerMilliseconds), duration);
  store.set_string("key", "not a duration");
  EXPECT_EQ(store.get("key", default_duration, ConfigStoreImpl::Format_IntegerMilliseconds), default_duration);
}

TEST(dds_DCPS_ConfigStoreImpl, set_get_TimeDuration_fractional_seconds)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);
  const TimeDuration default_duration(1,500000);
  const TimeDuration duration(2,500000);
  EXPECT_EQ(store.get("key", default_duration, ConfigStoreImpl::Format_FractionalSeconds), default_duration);
  store.set("key", duration, ConfigStoreImpl::Format_FractionalSeconds);
  EXPECT_EQ(store.get("key", default_duration, ConfigStoreImpl::Format_FractionalSeconds), duration);
  store.set_string("key", "not a duration");
  EXPECT_EQ(store.get("key", default_duration, ConfigStoreImpl::Format_FractionalSeconds), default_duration);
}

TEST(dds_DCPS_ConfigStoreImpl, get_NetworkAddress)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);

  {
    const NetworkAddress default_value("0.0.0.0:0");
    EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV4), default_value);

    {
      const NetworkAddress value_no_port("127.0.0.1:0");
      store.set("key", value_no_port, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV4);
      EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV4), value_no_port);
    }

    {
      const NetworkAddress value_required_port("127.0.0.1:80");
      store.set("key", value_required_port, ConfigStoreImpl::Format_Required_Port, ConfigStoreImpl::Kind_IPV4);
      EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_Required_Port, ConfigStoreImpl::Kind_IPV4), value_required_port);
    }

    {
      const NetworkAddress value_optional_port("127.0.0.1:0");
      store.set("key", value_optional_port, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV4);
      EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV4), value_optional_port);
    }

    {
      const NetworkAddress value_optional_port("127.0.0.1:80");
      store.set("key", value_optional_port, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV4);
      EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV4), value_optional_port);
    }

    {
      store.set_string("key", "not a network address");
      EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV4), default_value);
    }

    {
      const NetworkAddress value_required_port("127.0.0.1:80");
      store.set("key", value_required_port, ConfigStoreImpl::Format_Required_Port, ConfigStoreImpl::Kind_ANY);
      EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_Required_Port, ConfigStoreImpl::Kind_ANY), value_required_port);
    }
  }

#if ACE_HAS_IPV6
  {
    const NetworkAddress default_value("::");
    EXPECT_EQ(store.get("key6", default_value, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV6), default_value);

    {
      const NetworkAddress value_no_port("::1");
      store.set("key6", value_no_port, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV6);
      EXPECT_EQ(store.get("key6", default_value, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV6), value_no_port);
    }

    {
      const NetworkAddress value_required_port("[::1]:80");
      store.set("key6", value_required_port, ConfigStoreImpl::Format_Required_Port, ConfigStoreImpl::Kind_IPV6);
      EXPECT_EQ(store.get("key6", default_value, ConfigStoreImpl::Format_Required_Port, ConfigStoreImpl::Kind_IPV6), value_required_port);
    }

    {
      const NetworkAddress value_optional_port("::1");
      store.set("key6", value_optional_port, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV6);
      EXPECT_EQ(store.get("key6", default_value, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV6), value_optional_port);
    }

    {
      const NetworkAddress value_optional_port("[::1]:80");
      store.set("key6", value_optional_port, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV6);
      EXPECT_EQ(store.get("key6", default_value, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV6), value_optional_port);
    }

    {
      store.set_string("key6", "not a network address");
      EXPECT_EQ(store.get("key6", default_value, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV6), default_value);
    }

    {
      const NetworkAddress value_required_port("[::1]:80");
      store.set("key6", value_required_port, ConfigStoreImpl::Format_Required_Port, ConfigStoreImpl::Kind_ANY);
      EXPECT_EQ(store.get("key6", default_value, ConfigStoreImpl::Format_Required_Port, ConfigStoreImpl::Kind_ANY), value_required_port);
    }
  }
#endif
}

TEST(dds_DCPS_ConfigStoreImpl, get_NetworkAddressSet)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store(topic);

  {
    NetworkAddressSet default_value;
    default_value.insert(NetworkAddress("0.0.0.0:0"));
    EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV4), default_value);

    {
      NetworkAddressSet empty;
      store.set("key", empty, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV4);
      EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV4), empty);
    }

    {
      NetworkAddressSet value_no_port;
      value_no_port.insert(NetworkAddress("127.0.0.1:0"));
      store.set("key", value_no_port, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV4);
      EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV4), value_no_port);
    }

    {
      NetworkAddressSet value_required_port;
      value_required_port.insert(NetworkAddress("127.0.0.1:80"));
      value_required_port.insert(NetworkAddress("127.0.0.1:81"));
      store.set("key", value_required_port, ConfigStoreImpl::Format_Required_Port, ConfigStoreImpl::Kind_IPV4);
      EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_Required_Port, ConfigStoreImpl::Kind_IPV4), value_required_port);
    }

    {
      NetworkAddressSet value_optional_port;
      value_optional_port.insert(NetworkAddress("127.0.0.1:0"));
      value_optional_port.insert(NetworkAddress("127.0.0.2:80"));
      value_optional_port.insert(NetworkAddress("127.0.0.3:0"));
      store.set("key", value_optional_port, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV4);
      EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV4), value_optional_port);
    }

    {
      store.set_string("key", "not a network address");
      EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV4), default_value);
    }
  }

#if ACE_HAS_IPV6
  {
    NetworkAddressSet default_value;
    default_value.insert(NetworkAddress("::"));
    EXPECT_EQ(store.get("key6", default_value, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV6), default_value);

    {
      NetworkAddressSet empty;
      store.set("key", empty, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV6);
      EXPECT_EQ(store.get("key", default_value, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV6), empty);
    }

    {
      NetworkAddressSet value_no_port;
      value_no_port.insert(NetworkAddress("::1"));
      store.set("key6", value_no_port, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV6);
      EXPECT_EQ(store.get("key6", default_value, ConfigStoreImpl::Format_No_Port, ConfigStoreImpl::Kind_IPV6), value_no_port);
    }

    {
      NetworkAddressSet value_required_port;
      value_required_port.insert(NetworkAddress("[::1]:80"));
      value_required_port.insert(NetworkAddress("[::1]:81"));
      store.set("key6", value_required_port, ConfigStoreImpl::Format_Required_Port, ConfigStoreImpl::Kind_IPV6);
      EXPECT_EQ(store.get("key6", default_value, ConfigStoreImpl::Format_Required_Port, ConfigStoreImpl::Kind_IPV6), value_required_port);
    }

    {
      NetworkAddressSet value_optional_port;
      value_optional_port.insert(NetworkAddress("::1"));
      value_optional_port.insert(NetworkAddress("[::2]:80"));
      value_optional_port.insert(NetworkAddress("::3"));
      store.set("key6", value_optional_port, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV6);
      EXPECT_EQ(store.get("key6", default_value, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV6), value_optional_port);
    }

    {
      store.set_string("key6", "not a network address");
      EXPECT_EQ(store.get("key6", default_value, ConfigStoreImpl::Format_Optional_Port, ConfigStoreImpl::Kind_IPV6), default_value);
    }
  }
#endif
}

TEST(dds_DCPS_ConfigStoreImpl, take_has_prefix)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl store1(topic);
  ConfigReader_rch reader = make_rch<ConfigReader>(store1.datareader_qos());
  topic->connect(reader);

  store1.set("PREFIX1_key", "value");
  EXPECT_TRUE(take_has_prefix(reader, "PREFIX1"));
  EXPECT_FALSE(take_has_prefix(reader, "PREFIX1"));

  topic->disconnect(reader);
}

namespace {
  class Listener : public ConfigListener {
  public:
    Listener(JobQueue_rch job_queue)
      : ConfigListener(job_queue)
    {}

    MOCK_METHOD1(on_data_available, void(ConfigReader_rch));
  };
}

TEST(dds_DCPS_ConfigStoreImpl, process_section)
{
  JobQueue_rch job_queue = make_rch<JobQueue>(ACE_Reactor::instance());
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl config_store(topic);
  config_store.set("MYPREFIX_MY_SECTION_THIRDKEY", "firstvalue");
  ConfigReader_rch reader = make_rch<ConfigReader>(config_store.datareader_qos());
  RcHandle<Listener> listener = make_rch<Listener>(job_queue);
  ACE_Configuration_Heap config;
  ACE_Configuration_Section_Key section_key;
  config.open();
  config.open_section(config.root_section(), ACE_TEXT("my_section"), true, section_key);
  config.set_string_value(section_key, ACE_TEXT("mykey"), ACE_TEXT("myvalue"));
  config.set_string_value(section_key, ACE_TEXT("anotherkey"), ACE_TEXT("$file"));
  config.set_string_value(section_key, ACE_TEXT("thirdkey"), ACE_TEXT("secondvalue"));

  EXPECT_CALL(*listener.get(), on_data_available(reader)).Times(3);

  process_section(config_store, reader, listener, "MYPREFIX", config, config.root_section(), false);

  EXPECT_EQ(config_store.get("MYPREFIX_MY_SECTION", "default"), "@my_section");
  EXPECT_EQ(config_store.get("MYPREFIX_MY_SECTION_MYKEY", "default"), "myvalue");
  EXPECT_EQ(config_store.get("MYPREFIX_MY_SECTION_ANOTHERKEY", "default"), "$file");
  EXPECT_EQ(config_store.get("MYPREFIX_MY_SECTION_THIRDKEY", "default"), "firstvalue");
}

TEST(dds_DCPS_ConfigStoreImpl, process_section_allow_overwrite)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl config_store(topic);
  config_store.set("MYPREFIX_MY_SECTION_THIRDKEY", "firstvalue");
  ACE_Configuration_Heap config;
  ACE_Configuration_Section_Key section_key;
  config.open();
  config.open_section(config.root_section(), ACE_TEXT("my_section"), true, section_key);
  config.set_string_value(section_key, ACE_TEXT("mykey"), ACE_TEXT("myvalue"));
  config.set_string_value(section_key, ACE_TEXT("anotherkey"), ACE_TEXT("$file"));
  config.set_string_value(section_key, ACE_TEXT("thirdkey"), ACE_TEXT("secondvalue"));

  process_section(config_store, ConfigReader_rch(), ConfigReaderListener_rch(), "MYPREFIX", config, config.root_section(), true);

  EXPECT_EQ(config_store.get("MYPREFIX_MY_SECTION_MYKEY", "default"), "myvalue");
  EXPECT_EQ(config_store.get("MYPREFIX_MY_SECTION_ANOTHERKEY", "default"), "$file");
  EXPECT_EQ(config_store.get("MYPREFIX_MY_SECTION_THIRDKEY", "default"), "secondvalue");
}

TEST(dds_DCPS_ConfigStoreImpl, get_section_names)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl config_store(topic);
  config_store.set("MYPREFIX_MY_SECTION", "@my_section");
  config_store.set("MYPREFIX_MY_SECTION_KEY", "not a section");
  config_store.set("MYPREFIX_MY_SECTION2", "@my_section2");
  config_store.set("MYPREFIX_MY_SECTION2_KEY", "not a section");
  config_store.set("NOTMYPREFIX_MY_SECTION2", "@my_section2");
  config_store.set("NOTMYPREFIX_MY_SECTION2_KEY", "not a section");

  const ConfigStoreImpl::StringList sections = config_store.get_section_names("MYPREFIX");
  ASSERT_EQ(sections.size(), 2U);
  EXPECT_NE(std::find(sections.begin(), sections.end(), "my_section"), sections.end());
  EXPECT_NE(std::find(sections.begin(), sections.end(), "my_section2"), sections.end());
}

TEST(dds_DCPS_ConfigStoreImpl, get_section_values)
{
  JobQueue_rch job_queue = make_rch<JobQueue>(ACE_Reactor::instance());
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl config_store(topic);
  ConfigReader_rch reader = make_rch<ConfigReader>(config_store.datareader_qos());
  RcHandle<Listener> listener = make_rch<Listener>(job_queue);
  ACE_Configuration_Heap config;
  ACE_Configuration_Section_Key section_key;
  config.open();
  config.open_section(config.root_section(), ACE_TEXT("my_section"), true, section_key);
  config.set_string_value(section_key, ACE_TEXT("mykey"), ACE_TEXT("myvalue"));
  config.set_string_value(section_key, ACE_TEXT("anotherkey"), ACE_TEXT("$file"));
  config.set_string_value(section_key, ACE_TEXT("thirdkey"), ACE_TEXT("secondvalue"));

  EXPECT_CALL(*listener.get(), on_data_available(reader)).Times(4);

  process_section(config_store, reader, listener, "MYPREFIX", config, config.root_section(), false);

  ConfigStoreImpl::StringMap expected_sm;
  expected_sm["MYKEY"] = "myvalue";
  expected_sm["ANOTHERKEY"] = "$file";
  expected_sm["THIRDKEY"] = "secondvalue";

  const ConfigStoreImpl::StringMap sm = config_store.get_section_values("MYPREFIX_MY_SECTION");
  EXPECT_EQ(sm, expected_sm);
}

TEST(dds_DCPS_ConfigStoreImpl, delete_section)
{
  ConfigTopic_rch topic = make_rch<ConfigTopic>();
  ConfigStoreImpl config_store(topic);
  config_store.set("MYPREFIX_MY_SECTION", "@my_section");
  config_store.set("MYPREFIX_MY_SECTION_KEY", "not a section");
  config_store.set("MYPREFIX_MY_SECTION2", "@my_section2");
  config_store.set("MYPREFIX_MY_SECTION2_KEY", "not a section");
  config_store.set("NOTMYPREFIX_MY_SECTION2", "@my_section2");
  config_store.set("NOTMYPREFIX_MY_SECTION2_KEY", "not a section");

  config_store.unset_section("MYPREFIX");
  EXPECT_FALSE(config_store.has("MYPREFIX_MY_SECTION"));
  EXPECT_FALSE(config_store.has("MYPREFIX_MY_SECTION_KEY"));
  EXPECT_FALSE(config_store.has("MYPREFIX_MY_SECTION2"));
  EXPECT_FALSE(config_store.has("MYPREFIX_MY_SECTION2_KEY"));
  EXPECT_TRUE(config_store.has("NOTMYPREFIX_MY_SECTION2"));
  EXPECT_TRUE(config_store.has("NOTMYPREFIX_MY_SECTION2_KEY"));
}
