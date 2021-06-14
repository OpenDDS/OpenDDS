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
    , static_limit_(0)
    , max_pending_(0)
    , application_domain_(1)
    , log_warnings_(false)
    , log_entries_(false)
    , log_discovery_(false)
    , log_activity_(false)
  {}

  void application_participant_guid(const OpenDDS::DCPS::RepoId& value)
  {
    application_participant_guid_ = value;
  }

  const OpenDDS::DCPS::RepoId& application_participant_guid() const
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

  void static_limit(size_t value)
  {
    static_limit_ = value;
  }

  size_t static_limit() const
  {
    return static_limit_;
  }

  void max_pending(size_t value)
  {
    max_pending_ = value;
  }

  size_t max_pending() const
  {
    return max_pending_;
  }

  void application_domain(DDS::DomainId_t value)
  {
    application_domain_ = value;
  }

  DDS::DomainId_t application_domain() const
  {
    return application_domain_;
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

  void log_participant_statistics(OpenDDS::DCPS::TimeDuration value)
  {
    log_participant_statistics_ = value;
  }

  OpenDDS::DCPS::TimeDuration log_participant_statistics() const
  {
    return log_participant_statistics_;
  }

  void log_domain_statistics(OpenDDS::DCPS::TimeDuration value)
  {
    log_domain_statistics_ = value;
  }

  OpenDDS::DCPS::TimeDuration log_domain_statistics() const
  {
    return log_domain_statistics_;
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

  void publish_participant_statistics(OpenDDS::DCPS::TimeDuration value)
  {
    publish_participant_statistics_ = value;
  }

  OpenDDS::DCPS::TimeDuration publish_participant_statistics() const
  {
    return publish_participant_statistics_;
  }

  void publish_domain_statistics(OpenDDS::DCPS::TimeDuration value)
  {
    publish_domain_statistics_ = value;
  }

  OpenDDS::DCPS::TimeDuration publish_domain_statistics() const
  {
    return publish_domain_statistics_;
  }

private:
  OpenDDS::DCPS::RepoId application_participant_guid_;
  OpenDDS::DCPS::TimeDuration lifespan_;
  size_t static_limit_;
  size_t max_pending_;
  DDS::DomainId_t application_domain_;
  bool log_warnings_;
  bool log_entries_;
  bool log_discovery_;
  bool log_activity_;
  OpenDDS::DCPS::TimeDuration log_relay_statistics_;
  OpenDDS::DCPS::TimeDuration log_handler_statistics_;
  OpenDDS::DCPS::TimeDuration log_participant_statistics_;
  OpenDDS::DCPS::TimeDuration log_domain_statistics_;
  OpenDDS::DCPS::TimeDuration publish_relay_statistics_;
  OpenDDS::DCPS::TimeDuration publish_handler_statistics_;
  OpenDDS::DCPS::TimeDuration publish_participant_statistics_;
  OpenDDS::DCPS::TimeDuration publish_domain_statistics_;
};

}

#endif // RTPSRELAY_CONFIG_H_
