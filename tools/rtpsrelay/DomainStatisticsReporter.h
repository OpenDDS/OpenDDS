#ifndef RTPSRELAY_DOMAIN_STATISTICS_REPORTER_H_
#define RTPSRELAY_DOMAIN_STATISTICS_REPORTER_H_

#include "Config.h"

#include "lib/RelayTypeSupportImpl.h"
#include "lib/Utility.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class DomainStatisticsReporter {
public:
  DomainStatisticsReporter(const Config& config,
                           DomainStatisticsDataWriter_var writer)
    : config_(config)
    , log_last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , publish_last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , writer_(writer)
  {
    DDS::Topic_var topic = writer_->get_topic();
    topic_name_ = topic->get_name();
    log_domain_statistics_.application_participant_guid(repoid_to_guid(config_.application_participant_guid()));
    publish_domain_statistics_.application_participant_guid(repoid_to_guid(config_.application_participant_guid()));
  }

  void add_local_participant(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++log_domain_statistics_.local_participants();
    ++publish_domain_statistics_.local_participants();
    report(now);
  }

  void remove_local_participant(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    --log_domain_statistics_.local_participants();
    --publish_domain_statistics_.local_participants();
    report(now);
  }

  void add_local_writer(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++log_domain_statistics_.local_writers();
    ++publish_domain_statistics_.local_writers();
    report(now);
  }

  void remove_local_writer(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    --log_domain_statistics_.local_writers();
    --publish_domain_statistics_.local_writers();
    report(now);
  }

  void add_local_reader(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++log_domain_statistics_.local_readers();
    ++publish_domain_statistics_.local_readers();
    report(now);
  }

  void remove_local_reader(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    --log_domain_statistics_.local_readers();
    --publish_domain_statistics_.local_readers();
    report(now);
  }

  void report()
  {
    report(OpenDDS::DCPS::MonotonicTimePoint::now(), true);
  }

private:
  void report(const OpenDDS::DCPS::MonotonicTimePoint& now,
              bool force = false)
  {
    log_report(now, force);
    publish_report(now, force);
  }

  void log_report(const OpenDDS::DCPS::MonotonicTimePoint& now,
                  bool force)
  {
    if (config_.log_domain_statistics().is_zero()) {
      return;
    }

    const auto d = now - log_last_report_;
    if (!force && d < config_.log_domain_statistics()) {
      return;
    }

    log_domain_statistics_.interval(time_diff_to_duration(d));

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name_.in(), OpenDDS::DCPS::to_json(log_domain_statistics_).c_str()));

    log_last_report_ = now;
  }

  void publish_report(const OpenDDS::DCPS::MonotonicTimePoint& now,
                      bool force)
  {
    if (config_.publish_domain_statistics().is_zero()) {
      return;
    }

    const auto d = now - publish_last_report_;
    if (!force && d < config_.publish_domain_statistics()) {
      return;
    }

    publish_domain_statistics_.interval(time_diff_to_duration(d));

    const auto ret = writer_->write(publish_domain_statistics_, DDS::HANDLE_NIL);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DomainStatisticsReporter::report failed to write statistics\n")));
    }

    publish_last_report_ = now;
  }

  const Config& config_;

  OpenDDS::DCPS::MonotonicTimePoint log_last_report_;
  DomainStatistics log_domain_statistics_;

  OpenDDS::DCPS::MonotonicTimePoint publish_last_report_;
  DomainStatistics publish_domain_statistics_;

  DomainStatisticsDataWriter_var writer_;
  CORBA::String_var topic_name_;
};

}

#endif // RTPSRELAY_DOMAIN_STATISTICS_REPORTER_H_
