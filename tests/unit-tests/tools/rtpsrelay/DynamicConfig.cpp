#ifdef OPENDDS_HAS_CXX11

#include <tools/rtpsrelay/Config.h>

#include <gtest/gtest.h>

using namespace RtpsRelay;

using OpenDDS::DCPS::RcHandle;
using OpenDDS::DCPS::inc_count;
using OpenDDS::DCPS::make_rch;

namespace DDS {
bool operator==(const DDS::Duration_t& lhs, const DDS::Duration_t& rhs)
{
  return lhs.sec == rhs.sec && lhs.nanosec == rhs.nanosec;
}
}

TEST(tools_rtpsrelay_Config, verify_config)
{
  Config config;
  const OpenDDS::DCPS::RcHandle<Config> config_rch{&config, inc_count{}};
  const auto config_reader = make_rch<OpenDDS::DCPS::ConfigReader>(TheServiceParticipant->config_store()->datareader_qos(), config_rch);
  TheServiceParticipant->config_topic()->connect(config_reader);

  const DDS::Duration_t lifespan = {90, 0};
  const DDS::Duration_t inactive_period = {120, 0};
  bool log_warnings = true;
  bool log_discovery = true;
  bool log_activity = true;
  bool log_http = true;
  bool log_thread_status = true;
  int thread_status_safety_factor = 5;
  double utilization_limit = 0.85;
  bool log_utilization_changes = true;
  const DDS::Duration_t log_relay_statistics = {30, 0};
  const DDS::Duration_t log_handler_statistics = {40, 0};
  const DDS::Duration_t publish_relay_statistics = {50, 0};
  const DDS::Duration_t publish_handler_statistics = {60, 0};
  const DDS::Duration_t publish_relay_status = {70, 0};
  const DDS::Duration_t publish_relay_status_liveliness = {140, 0};
  bool restart_detection = true;
  DDS::UInt64 admission_control_queue_size = 999;
  const DDS::Duration_t admission_control_queue_duration = {90, 0};
  DDS::UInt64 max_ips_per_client = 5;
  const DDS::Duration_t rejected_address_duration = {100, 0};
  DDS::UInt64 max_participants_high_water = 100;
  DDS::UInt64 max_participants_low_water = 40;
  //const OpenDDS::DCPS::TimeDuration drain_interval{10, 0};

  TheServiceParticipant->config_store()->add_section("", "rtps_relay");
  TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_LIFESPAN, lifespan);
  TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_INACTIVE_PERIOD, inactive_period);
  TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_WARNINGS, log_warnings);
  TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_DISCOVERY, log_discovery);
  TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_ACTIVITY, log_activity);
  TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_HTTP, log_http);
  TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_THREAD_STATUS, log_thread_status);
  TheServiceParticipant->config_store()->set_int32(RTPS_RELAY_THREAD_STATUS_SAFETY_FACTOR, thread_status_safety_factor);
  TheServiceParticipant->config_store()->set_float64(RTPS_RELAY_UTILIZATION_LIMIT, utilization_limit);
  TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_UTILIZATION_CHANGES, log_utilization_changes);
  TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_LOG_RELAY_STATISTICS, log_relay_statistics);
  TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_LOG_HANDLER_STATISTICS, log_handler_statistics);
  TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_PUBLISH_RELAY_STATISTICS, publish_relay_statistics);
  TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_PUBLISH_HANDLER_STATISTICS, publish_handler_statistics);
  TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_PUBLISH_RELAY_STATUS, publish_relay_status);
  TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_PUBLISH_RELAY_STATUS_LIVELINESS, publish_relay_status_liveliness);
  TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_RESTART_DETECTION, restart_detection);
  TheServiceParticipant->config_store()->set_uint64(RTPS_RELAY_ADMISSION_CONTROL_QUEUE_SIZE, admission_control_queue_size);
  TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_ADMISSION_CONTROL_QUEUE_DURATION, admission_control_queue_duration);
  TheServiceParticipant->config_store()->set_uint64(RTPS_RELAY_MAX_IPS_PER_CLIENT, max_ips_per_client);
  TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_REJECTED_ADDRESS_DURATION, rejected_address_duration);
  TheServiceParticipant->config_store()->set_uint64(RTPS_RELAY_MAX_PARTICIPANTS_HIGH_WATER, max_participants_high_water);
  TheServiceParticipant->config_store()->set_uint64(RTPS_RELAY_MAX_PARTICIPANTS_LOW_WATER, max_participants_low_water);
  //TheServiceParticipant->config_store()->set(RTPS_RELAY_DRAIN_INTERVAL, drain_interval, OpenDDS::DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);

  // Check that the configuration changes are populated in the Config instance.
  EXPECT_EQ(config.lifespan().to_dds_duration(), lifespan);
  EXPECT_EQ(config.inactive_period().to_dds_duration(), inactive_period);
  EXPECT_EQ(config.log_warnings(), log_warnings);
  EXPECT_EQ(config.log_discovery(), log_discovery);
  EXPECT_EQ(config.log_activity(), log_activity);
  EXPECT_EQ(config.log_http(), log_http);
  EXPECT_EQ(config.log_thread_status(), log_thread_status);
  EXPECT_EQ(config.thread_status_safety_factor(), thread_status_safety_factor);
  EXPECT_DOUBLE_EQ(config.utilization_limit(), utilization_limit);
  EXPECT_EQ(config.log_utilization_changes(), log_utilization_changes);
  EXPECT_EQ(config.log_relay_statistics().to_dds_duration(), log_relay_statistics);
  EXPECT_EQ(config.log_handler_statistics().to_dds_duration(), log_handler_statistics);
  EXPECT_EQ(config.publish_relay_statistics().to_dds_duration(), publish_relay_statistics);
  EXPECT_EQ(config.publish_handler_statistics().to_dds_duration(), publish_handler_statistics);
  EXPECT_EQ(config.publish_relay_status().to_dds_duration(), publish_relay_status);
  EXPECT_EQ(config.publish_relay_status_liveliness().to_dds_duration(), publish_relay_status_liveliness);
  EXPECT_EQ(config.restart_detection(), restart_detection);
  EXPECT_EQ(config.admission_control_queue_size(), admission_control_queue_size);
  EXPECT_EQ(config.admission_control_queue_duration().to_dds_duration(), admission_control_queue_duration);
  EXPECT_EQ(config.max_ips_per_client(), max_ips_per_client);
  EXPECT_EQ(config.rejected_address_duration().to_dds_duration(), rejected_address_duration);
  EXPECT_EQ(config.admission_max_participants_high_water(), max_participants_high_water);
  EXPECT_EQ(config.admission_max_participants_low_water(), max_participants_low_water);
  //EXPECT_EQ(config.drain_interval(), drain_interval);
}

#endif
