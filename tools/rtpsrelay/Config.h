/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef RTPSRELAY_CONFIG_H_
#define RTPSRELAY_CONFIG_H_

#include <dds/DCPS/ConfigStoreImpl.h>
#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/TimeDuration.h>

#include <dds/DdsDcpsInfrastructureC.h>

#include <dds/rtpsrelaylib/RelayC.h>

#include <ace/Arg_Shifter.h>

namespace RtpsRelay {

const char RTPS_RELAY_ADMIT_STATE[] = "RTPS_RELAY_ADMIT_STATE";
const OpenDDS::DCPS::EnumList<AdmitState> admit_state_encoding[] =
  {
    { AdmitState::AS_NORMAL, "Normal" },
    { AdmitState::AS_NOT_ADMITTING, "NotAdmitting" }
  };

const char RTPS_RELAY_DRAIN_STATE[] = "RTPS_RELAY_DRAIN_STATE";
const OpenDDS::DCPS::EnumList<DrainState> drain_state_encoding[] =
  {
    { DrainState::DS_NORMAL, "Normal" },
    { DrainState::DS_DRAINING, "Draining" }
  };

const char RTPS_RELAY_DRAIN_INTERVAL[] = "RTPS_RELAY_DRAIN_INTERVAL";

const char RTPS_RELAY_LIFESPAN[] = "RTPS_RELAY_LIFESPAN";
const DDS::Duration_t RTPS_RELAY_LIFESPAN_default = {60, 0}; // 1 minute

const char RTPS_RELAY_INACTIVE_PERIOD[] = "RTPS_RELAY_INACTIVE_PERIOD";
const DDS::Duration_t RTPS_RELAY_INACTIVE_PERIOD_default = {60, 0}; // 1 minute

const char RTPS_RELAY_LOG_WARNINGS[] = "RTPS_RELAY_LOG_WARNINGS";
const bool RTPS_RELAY_LOG_WARNINGS_default = false;

const char RTPS_RELAY_LOG_DISCOVERY[] = "RTPS_RELAY_LOG_DISCOVERY";
const bool RTPS_RELAY_LOG_DISCOVERY_default = false;

const char RTPS_RELAY_LOG_ACTIVITY[] = "RTPS_RELAY_LOG_ACTIVITY";
const bool RTPS_RELAY_LOG_ACTIVITY_default = false;

const char RTPS_RELAY_LOG_HTTP[] = "RTPS_RELAY_LOG_HTTP";
const bool RTPS_RELAY_LOG_HTTP_default = false;

const char RTPS_RELAY_LOG_THREAD_STATUS[] = "RTPS_RELAY_LOG_THREAD_STATUS";
const bool RTPS_RELAY_LOG_THREAD_STATUS_default = false;

const char RTPS_RELAY_THREAD_STATUS_SAFETY_FACTOR[] = "RTPS_RELAY_THREAD_STATUS_SAFETY_FACTOR";
const int RTPS_RELAY_THREAD_STATUS_SAFETY_FACTOR_default = 3;

const char RTPS_RELAY_UTILIZATION_LIMIT[] = "RTPS_RELAY_UTILIZATION_LIMIT";
const double RTPS_RELAY_UTILIZATION_LIMIT_default = .95;

const char RTPS_RELAY_LOG_UTILIZATION_CHANGES[] = "RTPS_RELAY_LOG_UTILIZATION_CHANGES";
const bool RTPS_RELAY_LOG_UTILIZATION_CHANGES_default = false;

const char RTPS_RELAY_LOG_RELAY_STATISTICS[] = "RTPS_RELAY_LOG_RELAY_STATISTICS";
const DDS::Duration_t RTPS_RELAY_LOG_RELAY_STATISTICS_default = {0, 0};

const char RTPS_RELAY_LOG_HANDLER_STATISTICS[] = "RTPS_RELAY_LOG_HANDLER_STATISTICS";
const DDS::Duration_t RTPS_RELAY_LOG_HANDLER_STATISTICS_default = {0, 0};

const char RTPS_RELAY_PUBLISH_RELAY_STATISTICS[] = "RTPS_RELAY_PUBLISH_RELAY_STATISTICS";
const DDS::Duration_t RTPS_RELAY_PUBLISH_RELAY_STATISTICS_default = {0, 0};

const char RTPS_RELAY_PUBLISH_HANDLER_STATISTICS[] = "RTPS_RELAY_PUBLISH_HANDLER_STATISTICS";
const DDS::Duration_t RTPS_RELAY_PUBLISH_HANDLER_STATISTICS_default = {0, 0};

const char RTPS_RELAY_PUBLISH_RELAY_STATUS[] = "RTPS_RELAY_PUBLISH_RELAY_STATUS";
const DDS::Duration_t RTPS_RELAY_PUBLISH_RELAY_STATUS_default = {0, 0};

const char RTPS_RELAY_PUBLISH_RELAY_STATUS_LIVELINESS[] = "RTPS_RELAY_PUBLISH_RELAY_STATUS_LIVELINESS";
const DDS::Duration_t RTPS_RELAY_PUBLISH_RELAY_STATUS_LIVELINESS_default = {0, 0};

const char RTPS_RELAY_RESTART_DETECTION[] = "RTPS_RELAY_RESTART_DETECTION";
const bool RTPS_RELAY_RESTART_DETECTION_default = false;

const char RTPS_RELAY_ADMISSION_CONTROL_QUEUE_SIZE[] = "RTPS_RELAY_ADMISSION_CONTROL_QUEUE_SIZE";
const DDS::UInt64 RTPS_RELAY_ADMISSION_CONTROL_QUEUE_SIZE_default = 0;

const char RTPS_RELAY_ADMISSION_CONTROL_QUEUE_DURATION[] = "RTPS_RELAY_ADMISSION_CONTROL_QUEUE_DURATION";
const DDS::Duration_t RTPS_RELAY_ADMISSION_CONTROL_QUEUE_DURATION_default = {0, 0};

const char RTPS_RELAY_MAX_IPS_PER_CLIENT[] = "RTPS_RELAY_MAX_IPS_PER_CLIENT";
const DDS::UInt64 RTPS_RELAY_MAX_IPS_PER_CLIENT_default = 0;

const char RTPS_RELAY_REJECTED_ADDRESS_DURATION[] = "RTPS_RELAY_REJECTED_ADDRESS_DURATION";
const DDS::Duration_t RTPS_RELAY_REJECTED_ADDRESS_DURATION_default = {0, 0};

const char RTPS_RELAY_MAX_PARTICIPANTS_HIGH_WATER[] = "RTPS_RELAY_MAX_PARTICIPANTS_HIGH_WATER";
const DDS::UInt64 RTPS_RELAY_MAX_PARTICIPANTS_HIGH_WATER_default = 0;

const char RTPS_RELAY_MAX_PARTICIPANTS_LOW_WATER[] = "RTPS_RELAY_MAX_PARTICIPANTS_LOW_WATER";
const DDS::UInt64 RTPS_RELAY_MAX_PARTICIPANTS_LOW_WATER_default = 0;

/// Configuration values for the RtpsRelay
///
/// Each value uses one of these implementation strategies:
/// a. A data member of this class holds the value and simple accessor/mutator functions are provided.
///    This is used for values that can't change after initialization.
/// b. The value is kept in the ConfigStore instance managed by the OpenDDS Service_Participant.
///    This is used for values that can change any time.  Internal components that depend on this
///    value can access it on-demand from this class and/or subscribe to updates from the ConfigStore.
///    These values are also cached locally for lightweight read access.
class Config : public OpenDDS::DCPS::ConfigListener {
public:
  Config()
    : InternalDataReaderListener{TheServiceParticipant->job_queue()}
  {
    TheServiceParticipant->config_store()->add_section("", "rtps_relay");
  }

  bool from_arg(ACE_Arg_Shifter_T<char>& args);
  void set_defaults();

  void relay_id(const std::string& value)
  {
    relay_id_ = value;
  }

  const std::string& relay_id() const
  {
    return relay_id_;
  }

  void application_participant_guid(const OpenDDS::DCPS::GUID_t& value)
  {
    application_participant_guid_ = value;
  }

  const OpenDDS::DCPS::GUID_t& application_participant_guid() const
  {
    return application_participant_guid_;
  }

  void lifespan(const OpenDDS::DCPS::TimeDuration& value)
  {
    lifespan(value.to_dds_duration());
  }

  OpenDDS::DCPS::TimeDuration lifespan() const
  {
    return cached_lifespan_.get();
  }

  void inactive_period(const OpenDDS::DCPS::TimeDuration& value)
  {
    inactive_period(value.to_dds_duration());
  }

  OpenDDS::DCPS::TimeDuration inactive_period() const
  {
    return cached_inactive_period_.get();
  }

  void buffer_size(int value)
  {
    buffer_size_ = value;
  }

  int buffer_size() const
  {
    return buffer_size_;
  }

  void application_domain(DDS::DomainId_t value)
  {
    application_domain_ = value;
  }

  DDS::DomainId_t application_domain() const
  {
    return application_domain_;
  }

  void allow_empty_partition(bool flag)
  {
    allow_empty_partition_ = flag;
  }

  bool allow_empty_partition() const
  {
    return allow_empty_partition_;
  }

  void log_warnings(bool flag)
  {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_WARNINGS, flag);
    cached_log_warnings_.set(flag);
  }

  bool log_warnings() const
  {
    return cached_log_warnings_.get();
  }

  void log_discovery(bool flag)
  {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_DISCOVERY, flag);
    cached_log_discovery_.set(flag);
  }

  bool log_discovery() const
  {
    return cached_log_discovery_.get();
  }

  void log_activity(bool flag)
  {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_ACTIVITY, flag);
    cached_log_activity_.set(flag);
  }

  bool log_activity() const
  {
    return cached_log_activity_.get();
  }

  void log_http(bool flag)
  {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_HTTP, flag);
    cached_log_http_.set(flag);
  }

  bool log_http() const
  {
    return cached_log_http_.get();
  }

  void log_thread_status(bool flag)
  {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_THREAD_STATUS, flag);
    cached_log_thread_status_.set(flag);
  }

  bool log_thread_status() const
  {
    return cached_log_thread_status_.get();
  }

  void thread_status_safety_factor(int value)
  {
    TheServiceParticipant->config_store()->set_int32(RTPS_RELAY_THREAD_STATUS_SAFETY_FACTOR, value);
    cached_thread_status_safety_factor_.set(value);
  }

  int thread_status_safety_factor() const
  {
    return cached_thread_status_safety_factor_.get();
  }

  void utilization_limit(double value)
  {
    TheServiceParticipant->config_store()->set_float64(RTPS_RELAY_UTILIZATION_LIMIT, value);
    cached_utilization_limit_.set(value);
  }

  double utilization_limit() const
  {
    return cached_utilization_limit_.get();
  }

  void log_utilization_changes(bool value)
  {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_UTILIZATION_CHANGES, value);
    cached_log_utilization_changes_.set(value);
  }

  bool log_utilization_changes() const
  {
    return cached_log_utilization_changes_.get();
  }

  void log_relay_statistics(const OpenDDS::DCPS::TimeDuration& value)
  {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_LOG_RELAY_STATISTICS, value.to_dds_duration());
    cached_log_relay_statistics_.set(value);
  }

  OpenDDS::DCPS::TimeDuration log_relay_statistics() const
  {
    return cached_log_relay_statistics_.get();
  }

  void log_handler_statistics(const OpenDDS::DCPS::TimeDuration& value)
  {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_LOG_HANDLER_STATISTICS, value.to_dds_duration());
    cached_log_handler_statistics_.set(value);
  }

  OpenDDS::DCPS::TimeDuration log_handler_statistics() const
  {
    return cached_log_handler_statistics_.get();
  }

  void publish_relay_statistics(const OpenDDS::DCPS::TimeDuration& value)
  {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_PUBLISH_RELAY_STATISTICS, value.to_dds_duration());
    cached_publish_relay_statistics_.set(value);
  }

  OpenDDS::DCPS::TimeDuration publish_relay_statistics() const
  {
    return cached_publish_relay_statistics_.get();
  }

  void publish_handler_statistics(const OpenDDS::DCPS::TimeDuration& value)
  {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_PUBLISH_HANDLER_STATISTICS, value.to_dds_duration());
    cached_publish_handler_statistics_.set(value);
  }

  OpenDDS::DCPS::TimeDuration publish_handler_statistics() const
  {
    return cached_publish_handler_statistics_.get();
  }

  void publish_relay_status(const OpenDDS::DCPS::TimeDuration& value)
  {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_PUBLISH_RELAY_STATUS, value.to_dds_duration());
    cached_publish_relay_status_.set(value);
  }

  OpenDDS::DCPS::TimeDuration publish_relay_status() const
  {
    return cached_publish_relay_status_.get();
  }

  void publish_relay_status_liveliness(const OpenDDS::DCPS::TimeDuration& value)
  {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_PUBLISH_RELAY_STATUS_LIVELINESS, value.to_dds_duration());
    cached_publish_relay_status_liveliness_.set(value);
  }

  OpenDDS::DCPS::TimeDuration publish_relay_status_liveliness() const
  {
    return cached_publish_relay_status_liveliness_.get();
  }

  void restart_detection(bool flag)
  {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_RESTART_DETECTION, flag);
    cached_restart_detection_.set(flag);
  }

  bool restart_detection() const
  {
    return cached_restart_detection_.get();
  }

  void admission_control_queue_size(size_t value)
  {
    TheServiceParticipant->config_store()->set_uint64(RTPS_RELAY_ADMISSION_CONTROL_QUEUE_SIZE, value);
    cached_admission_control_queue_size_.set(value);
  }

  size_t admission_control_queue_size() const
  {
    return cached_admission_control_queue_size_.get();
  }

  void admission_control_queue_duration(const OpenDDS::DCPS::TimeDuration& value)
  {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_ADMISSION_CONTROL_QUEUE_DURATION, value.to_dds_duration());
    cached_admission_control_queue_duration_.set(value);
  }

  OpenDDS::DCPS::TimeDuration admission_control_queue_duration() const
  {
    return cached_admission_control_queue_duration_.get();
  }

  void run_time(OpenDDS::DCPS::TimeDuration value)
  {
    run_time_ = value;
  }

  OpenDDS::DCPS::TimeDuration run_time() const
  {
    return run_time_;
  }

  void max_ips_per_client(size_t value)
  {
    TheServiceParticipant->config_store()->set_uint64(RTPS_RELAY_MAX_IPS_PER_CLIENT, value);
    cached_max_ips_per_client_.set(value);
  }

  size_t max_ips_per_client() const
  {
    return cached_max_ips_per_client_.get();
  }

  void rejected_address_duration(const OpenDDS::DCPS::TimeDuration& value)
  {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_REJECTED_ADDRESS_DURATION, value.to_dds_duration());
    cached_rejected_address_duration_.set(value);
  }

  OpenDDS::DCPS::TimeDuration rejected_address_duration() const
  {
    return cached_rejected_address_duration_.get();
  }

  void admission_max_participants_high_water(size_t value)
  {
    TheServiceParticipant->config_store()->set_uint64(RTPS_RELAY_MAX_PARTICIPANTS_HIGH_WATER, value);
    cached_admission_max_participants_high_water_.set(value);
  }

  size_t admission_max_participants_high_water() const
  {
    return cached_admission_max_participants_high_water_.get();
  }

  void admission_max_participants_low_water(size_t value)
  {
    TheServiceParticipant->config_store()->set_uint64(RTPS_RELAY_MAX_PARTICIPANTS_LOW_WATER, value);
    cached_admission_max_participants_low_water_.set(value);
  }

  size_t admission_max_participants_low_water() const
  {
    return cached_admission_max_participants_low_water_.get();
  }

  void handler_threads(size_t count)
  {
    handler_threads_ = count;
  }

  size_t handler_threads() const
  {
    return handler_threads_;
  }

  void synchronous_output(bool flag)
  {
    synchronous_output_ = flag;
  }

  bool synchronous_output() const
  {
    return synchronous_output_;
  }

  void drain_interval(const OpenDDS::DCPS::TimeDuration& value)
  {
    TheServiceParticipant->config_store()->set(RTPS_RELAY_DRAIN_INTERVAL,
                                               value,
                                               OpenDDS::DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
  }

  static bool to_time_duration(const std::string& value, OpenDDS::DCPS::TimeDuration& out);

private:
  void on_data_available(InternalDataReader_rch reader) override;

  bool parse_admission_participants_range(const char* arg);

  void lifespan(const DDS::Duration_t& value)
  {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_LIFESPAN, value);
    cached_lifespan_.set(OpenDDS::DCPS::TimeDuration{value});
  }

  void inactive_period(const DDS::Duration_t& value)
  {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_INACTIVE_PERIOD, value);
    cached_inactive_period_.set(OpenDDS::DCPS::TimeDuration{value});
  }

  template <typename T, bool ConvertFromString(const std::string&, T&)>
  class CachedValue {
  public:
    explicit CachedValue(const T& default_value)
      : default_value_{default_value}
    {}

    void set(const std::string& value_as_string)
    {
      ACE_Guard<ACE_Thread_Mutex> g{lock_};
      T val;
      if (ConvertFromString(value_as_string, val)) {
        value_ = val;
      }
    }

    void set(const T& value)
    {
      ACE_Guard<ACE_Thread_Mutex> g{lock_};
      value_ = value;
    }

    template <typename Fn>
    void default_if_empty(Fn&& func)
    {
      ACE_Guard<ACE_Thread_Mutex> g{lock_};
      if (!value_) {
        func(default_value_);
        value_ = default_value_;
      }
    }

    T get() const
    {
      ACE_Guard<ACE_Thread_Mutex> g{lock_};
      return value_.value_or(default_value_);
    }

  private:
    mutable ACE_Thread_Mutex lock_;
    OPENDDS_OPTIONAL_NS::optional<T> value_;
    const T default_value_;
  };

  CachedValue<OpenDDS::DCPS::TimeDuration, to_time_duration> cached_lifespan_{OpenDDS::DCPS::TimeDuration{RTPS_RELAY_LIFESPAN_default}};
  CachedValue<OpenDDS::DCPS::TimeDuration, to_time_duration> cached_inactive_period_{OpenDDS::DCPS::TimeDuration{RTPS_RELAY_INACTIVE_PERIOD_default}};
  CachedValue<bool, OpenDDS::DCPS::ConfigStoreImpl::convert_value> cached_log_warnings_{RTPS_RELAY_LOG_WARNINGS_default};
  CachedValue<bool, OpenDDS::DCPS::ConfigStoreImpl::convert_value> cached_log_discovery_{RTPS_RELAY_LOG_DISCOVERY_default};
  CachedValue<bool, OpenDDS::DCPS::ConfigStoreImpl::convert_value> cached_log_activity_{RTPS_RELAY_LOG_ACTIVITY_default};
  CachedValue<bool, OpenDDS::DCPS::ConfigStoreImpl::convert_value> cached_log_http_{RTPS_RELAY_LOG_HTTP_default};
  CachedValue<bool, OpenDDS::DCPS::ConfigStoreImpl::convert_value> cached_log_thread_status_{RTPS_RELAY_LOG_THREAD_STATUS_default};
  CachedValue<int, OpenDDS::DCPS::convertToInteger> cached_thread_status_safety_factor_{RTPS_RELAY_THREAD_STATUS_SAFETY_FACTOR_default};
  CachedValue<double, OpenDDS::DCPS::convertToFloating> cached_utilization_limit_{RTPS_RELAY_UTILIZATION_LIMIT_default};
  CachedValue<bool, OpenDDS::DCPS::ConfigStoreImpl::convert_value> cached_log_utilization_changes_{RTPS_RELAY_LOG_UTILIZATION_CHANGES_default};
  CachedValue<OpenDDS::DCPS::TimeDuration, to_time_duration> cached_log_relay_statistics_{OpenDDS::DCPS::TimeDuration{RTPS_RELAY_LOG_RELAY_STATISTICS_default}};
  CachedValue<OpenDDS::DCPS::TimeDuration, to_time_duration> cached_log_handler_statistics_{OpenDDS::DCPS::TimeDuration{RTPS_RELAY_LOG_HANDLER_STATISTICS_default}};
  CachedValue<OpenDDS::DCPS::TimeDuration, to_time_duration> cached_publish_relay_statistics_{OpenDDS::DCPS::TimeDuration{RTPS_RELAY_PUBLISH_RELAY_STATISTICS_default}};
  CachedValue<OpenDDS::DCPS::TimeDuration, to_time_duration> cached_publish_handler_statistics_{OpenDDS::DCPS::TimeDuration{RTPS_RELAY_PUBLISH_HANDLER_STATISTICS_default}};
  CachedValue<OpenDDS::DCPS::TimeDuration, to_time_duration> cached_publish_relay_status_{OpenDDS::DCPS::TimeDuration{RTPS_RELAY_PUBLISH_RELAY_STATUS_default}};
  CachedValue<OpenDDS::DCPS::TimeDuration, to_time_duration> cached_publish_relay_status_liveliness_{OpenDDS::DCPS::TimeDuration{RTPS_RELAY_PUBLISH_RELAY_STATUS_LIVELINESS_default}};
  CachedValue<bool, OpenDDS::DCPS::ConfigStoreImpl::convert_value> cached_restart_detection_{RTPS_RELAY_RESTART_DETECTION_default};
  CachedValue<size_t, OpenDDS::DCPS::convertToInteger> cached_admission_control_queue_size_{RTPS_RELAY_ADMISSION_CONTROL_QUEUE_SIZE_default};
  CachedValue<OpenDDS::DCPS::TimeDuration, to_time_duration> cached_admission_control_queue_duration_{OpenDDS::DCPS::TimeDuration{RTPS_RELAY_ADMISSION_CONTROL_QUEUE_DURATION_default}};
  CachedValue<size_t, OpenDDS::DCPS::convertToInteger> cached_max_ips_per_client_{RTPS_RELAY_MAX_IPS_PER_CLIENT_default};
  CachedValue<OpenDDS::DCPS::TimeDuration, to_time_duration> cached_rejected_address_duration_{OpenDDS::DCPS::TimeDuration{RTPS_RELAY_REJECTED_ADDRESS_DURATION_default}};
  CachedValue<size_t, OpenDDS::DCPS::convertToInteger> cached_admission_max_participants_high_water_{RTPS_RELAY_MAX_PARTICIPANTS_HIGH_WATER_default};
  CachedValue<size_t, OpenDDS::DCPS::convertToInteger> cached_admission_max_participants_low_water_{RTPS_RELAY_MAX_PARTICIPANTS_LOW_WATER_default};

  // start of variables without ConfigStore support
  std::string relay_id_;
  OpenDDS::DCPS::GUID_t application_participant_guid_ = OpenDDS::DCPS::GUID_UNKNOWN;
  int buffer_size_ =
#ifdef ACE_DEFAULT_MAX_SOCKET_BUFSIZ
    ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
#else
    16384;
#endif
  DDS::DomainId_t application_domain_ = 1;
  bool allow_empty_partition_ = true;
  OpenDDS::DCPS::TimeDuration run_time_;
  bool synchronous_output_ = false;
  size_t handler_threads_ = 1;
  // end of variables without ConfigStore support
};

}

#endif // RTPSRELAY_CONFIG_H_
