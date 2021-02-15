#ifndef RTPSRELAY_DOMAIN_STATISTICS_REPORTER_H_
#define RTPSRELAY_DOMAIN_STATISTICS_REPORTER_H_

#include "Config.h"
#include "utility.h"

#include "lib/QosIndex.h"
#include "lib/RelayTypeSupportImpl.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class DomainStatisticsReporter {
public:
  DomainStatisticsReporter(const Config& config,
                           DomainStatisticsDataWriter_var writer)
    : config_(config)
    , last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , writer_(writer)
  {
    DDS::Topic_var topic = writer_->get_topic();
    topic_name_ = topic->get_name();
    domain_statistics_.application_participant_guid(repoid_to_guid(config_.application_participant_guid()));
  }

  void add_local_participant(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++domain_statistics_.local_participants();
    report(now);
  }

  void remove_local_participant(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    --domain_statistics_.local_participants();
    report(now);
  }

  void add_local_writer(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++domain_statistics_.local_writers();
    report(now);
  }

  void remove_local_writer(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    --domain_statistics_.local_writers();
    report(now);
  }

  void add_local_reader(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++domain_statistics_.local_readers();
    report(now);
  }

  void remove_local_reader(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    --domain_statistics_.local_readers();
    report(now);
  }

  void total_writers(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    domain_statistics_.total_writers(static_cast<uint32_t>(count));
    report(now);
  }

  void total_readers(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    domain_statistics_.total_readers(static_cast<uint32_t>(count));
    report(now);
  }

private:

  void report(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    const auto d = now - last_report_;

    domain_statistics_.interval(time_diff_to_duration(d));

    if (config_.log_relay_statistics()) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name_.in(), OpenDDS::DCPS::to_json(domain_statistics_).c_str()));
    }

    if (config_.publish_relay_statistics()) {
      const auto ret = writer_->write(domain_statistics_, DDS::HANDLE_NIL);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DomainStatisticsReporter::report failed to write statistics\n")));
      }
    }

    last_report_ = now;
  }

  const Config& config_;
  OpenDDS::DCPS::MonotonicTimePoint last_report_;
  DomainStatistics domain_statistics_;
  DomainStatisticsDataWriter_var writer_;
  CORBA::String_var topic_name_;
};

}

#endif // RTPSRELAY_DOMAIN_STATISTICS_REPORTER_H_
