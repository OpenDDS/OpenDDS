#ifndef RTPSRELAY_CONFIG_H_
#define RTPSRELAY_CONFIG_H_

#include <dds/DCPS/TimeDuration.h>
#include <dds/DCPS/GuidUtils.h>
#include <dds/DdsDcpsInfrastructureC.h>

namespace RtpsRelay {

class Config {
public:
  Config()
    : application_participant_guid_(OpenDDS::DCPS::GUID_UNKNOWN)
    , lifespan_(60) // 1 minute
    , max_pending_(0)
    , pending_timeout_(60) // 1 minute
#ifdef ACE_DEFAULT_MAX_SOCKET_BUFSIZ
    , buffer_size_(ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
#else
    , buffer_size_(16384)
#endif
    , application_domain_(1)
    , allow_empty_partition_(true)
    , log_warnings_(false)
    , log_entries_(false)
    , log_discovery_(false)
    , log_activity_(false)
    , restart_detection_(false)
  {}

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
    lifespan_ = value;
  }

  const OpenDDS::DCPS::TimeDuration& lifespan() const
  {
    return lifespan_;
  }

  void max_pending(size_t value)
  {
    max_pending_ = value;
  }

  size_t max_pending() const
  {
    return max_pending_;
  }

  void pending_timeout(const OpenDDS::DCPS::TimeDuration& value)
  {
    pending_timeout_ = value;
  }

  const OpenDDS::DCPS::TimeDuration& pending_timeout() const
  {
    return pending_timeout_;
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
    log_warnings_ = flag;
  }

  bool log_warnings() const
  {
    return log_warnings_;
  }

  void log_entries(bool flag)
  {
    log_entries_ = flag;
  }

  bool log_entries() const
  {
    return log_entries_;
  }

  void log_discovery(bool flag)
  {
    log_discovery_ = flag;
  }

  bool log_discovery() const
  {
    return log_discovery_;
  }

  void log_activity(bool flag)
  {
    log_activity_ = flag;
  }

  bool log_activity() const
  {
    return log_activity_;
  }

  void log_relay_statistics(OpenDDS::DCPS::TimeDuration value)
  {
    log_relay_statistics_ = value;
  }

  OpenDDS::DCPS::TimeDuration log_relay_statistics() const
  {
    return log_relay_statistics_;
  }

  void log_handler_statistics(OpenDDS::DCPS::TimeDuration value)
  {
    log_handler_statistics_ = value;
  }

  OpenDDS::DCPS::TimeDuration log_handler_statistics() const
  {
    return log_handler_statistics_;
  }

  void log_participant_statistics(bool value)
  {
    log_participant_statistics_ = value;
  }

  bool log_participant_statistics() const
  {
    return log_participant_statistics_;
  }

  void publish_relay_statistics(OpenDDS::DCPS::TimeDuration value)
  {
    publish_relay_statistics_ = value;
  }

  OpenDDS::DCPS::TimeDuration publish_relay_statistics() const
  {
    return publish_relay_statistics_;
  }

  void publish_handler_statistics(OpenDDS::DCPS::TimeDuration value)
  {
    publish_handler_statistics_ = value;
  }

  OpenDDS::DCPS::TimeDuration publish_handler_statistics() const
  {
    return publish_handler_statistics_;
  }

  void publish_participant_statistics(bool value)
  {
    publish_participant_statistics_ = value;
  }

  bool publish_participant_statistics() const
  {
    return publish_participant_statistics_;
  }

  void publish_relay_status(OpenDDS::DCPS::TimeDuration value)
  {
    publish_relay_status_ = value;
  }

  OpenDDS::DCPS::TimeDuration publish_relay_status() const
  {
    return publish_relay_status_;
  }

  void publish_relay_status_liveliness(OpenDDS::DCPS::TimeDuration value)
  {
    publish_relay_status_liveliness_ = value;
  }

  OpenDDS::DCPS::TimeDuration publish_relay_status_liveliness() const
  {
    return publish_relay_status_liveliness_;
  }

  void restart_detection(bool flag)
  {
    restart_detection_ = flag;
  }

  bool restart_detection() const
  {
    return restart_detection_;
  }

private:
  std::string relay_id_;
  OpenDDS::DCPS::GUID_t application_participant_guid_;
  OpenDDS::DCPS::TimeDuration lifespan_;
  size_t max_pending_;
  OpenDDS::DCPS::TimeDuration pending_timeout_;
  int buffer_size_;
  DDS::DomainId_t application_domain_;
  bool allow_empty_partition_;
  bool log_warnings_;
  bool log_entries_;
  bool log_discovery_;
  bool log_activity_;
  OpenDDS::DCPS::TimeDuration log_relay_statistics_;
  OpenDDS::DCPS::TimeDuration log_handler_statistics_;
  bool log_participant_statistics_;
  OpenDDS::DCPS::TimeDuration publish_relay_statistics_;
  OpenDDS::DCPS::TimeDuration publish_handler_statistics_;
  bool publish_participant_statistics_;
  OpenDDS::DCPS::TimeDuration publish_relay_status_;
  OpenDDS::DCPS::TimeDuration publish_relay_status_liveliness_;
  bool restart_detection_;
};

}

#endif // RTPSRELAY_CONFIG_H_
