#ifndef RTPSRELAY_DOMAIN_STATISTICS_REPORTER_H_
#define RTPSRELAY_DOMAIN_STATISTICS_REPORTER_H_

#include "Config.h"
#include "utility.h"

#include "lib/QosIndex.h"
#include "lib/RelayTypeSupportImpl.h"

#include <iostream>

namespace RtpsRelay {

class DomainStatisticsReporter {
public:
  DomainStatisticsReporter(const Config& config,
                           DomainStatisticsDataWriter_ptr writer)
    : config_(config)
    , last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , writer_(writer)
  {
    domain_statistics_.application_participant_guid(repoid_to_guid(config_.application_participant_guid()));
    domain_statistics_.local_participants(0);
    domain_statistics_.local_writers(0);
    domain_statistics_.local_readers(0);
    domain_statistics_.total_writers(0);
    domain_statistics_.total_readers(0);
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
    domain_statistics_.total_writers(count);
    report(now);
  }

  void total_readers(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    domain_statistics_.total_readers(count);
    report(now);
  }

private:

  void report(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    const auto d = now - last_report_;
    if (d < config_.statistics_interval()) {
      return;
    }

    domain_statistics_.interval(time_diff_to_duration(d));

    if (config_.log_relay_statistics()) {
      ACE_TCHAR timestamp[OpenDDS::DCPS::AceTimestampSize];
      ACE::timestamp(timestamp, sizeof(timestamp) / sizeof(ACE_TCHAR));

      std::cout << timestamp << ' '
                << "application_participant_guid=" << guid_to_string(guid_to_repoid(domain_statistics_.application_participant_guid())) << ' '
                << "local_participants=" << domain_statistics_.local_participants() << ' '
                << "local_writers=" << domain_statistics_.local_writers() << ' '
                << "local_readers=" << domain_statistics_.local_readers() << ' '
                << "total_writers=" << domain_statistics_.total_writers() << ' '
                << "total_readers=" << domain_statistics_.total_readers() << ' '
                << std::endl;
    }

    if (config_.publish_relay_statistics()) {
      const auto ret = writer_->write(domain_statistics_, DDS::HANDLE_NIL);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: DomainStatisticsReporter::report failed to write statistics\n")));
      }
    }

    last_report_ = now;
  }

  const Config& config_;
  OpenDDS::DCPS::MonotonicTimePoint last_report_;
  DomainStatistics domain_statistics_;
  DomainStatisticsDataWriter_ptr writer_;
};

}

#endif // RTPSRELAY_DOMAIN_STATISTICS_REPORTER_H_
