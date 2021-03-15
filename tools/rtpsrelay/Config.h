#ifndef RTPSRELAY_CONFIG_H_
#define RTPSRELAY_CONFIG_H_

#include <dds/DCPS/TimeDuration.h>
#include <dds/DCPS/GuidUtils.h>
#include <dds/DdsDcpsInfrastructureC.h>

namespace RtpsRelay {

class Config {
public:
  Config()
    : statistics_interval_(60) // 1 minute
    , application_participant_guid_(OpenDDS::DCPS::GUID_UNKNOWN)
    , lifespan_(60) // 1 minute
    , application_domain_(1)
    , publish_relay_statistics_(true)
    , log_entries_(false)
    , log_discovery_(false)
    , log_activity_(false)
    , log_relay_statistics_(false)
    , log_handler_statistics_(false)
    , log_participant_statistics_(false)
    , log_domain_statistics_(false)
  {}

  void statistics_interval(const OpenDDS::DCPS::TimeDuration& value)
  {
    statistics_interval_ = value;
  }

  const OpenDDS::DCPS::TimeDuration& statistics_interval() const
  {
    return statistics_interval_;
  }

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

  void application_domain(DDS::DomainId_t value)
  {
    application_domain_ = value;
  }

  DDS::DomainId_t application_domain() const
  {
    return application_domain_;
  }

  void publish_relay_statistics(bool flag)
  {
    publish_relay_statistics_ = flag;
  }

  bool publish_relay_statistics() const
  {
    return publish_relay_statistics_;
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

  void log_relay_statistics(bool flag)
  {
    log_relay_statistics_ = flag;
  }

  bool log_relay_statistics() const
  {
    return log_relay_statistics_;
  }

  void log_handler_statistics(bool flag)
  {
    log_handler_statistics_ = flag;
  }

  bool log_handler_statistics() const
  {
    return log_handler_statistics_;
  }

  void log_participant_statistics(bool flag)
  {
    log_participant_statistics_ = flag;
  }

  bool log_participant_statistics() const
  {
    return log_participant_statistics_;
  }

  void log_domain_statistics(bool flag)
  {
    log_domain_statistics_ = flag;
  }

  bool log_domain_statistics() const
  {
    return log_domain_statistics_;
  }

private:
  OpenDDS::DCPS::TimeDuration statistics_interval_;
  OpenDDS::DCPS::RepoId application_participant_guid_;
  OpenDDS::DCPS::TimeDuration lifespan_;
  DDS::DomainId_t application_domain_;
  bool publish_relay_statistics_;
  bool log_entries_;
  bool log_discovery_;
  bool log_activity_;
  bool log_relay_statistics_;
  bool log_handler_statistics_;
  bool log_participant_statistics_;
  bool log_domain_statistics_;
};

}

#endif // RTPSRELAY_CONFIG_H_
