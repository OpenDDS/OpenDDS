#ifndef RTPSRELAY_DOMAIN_STATISTICS_WRITER_H_
#define RTPSRELAY_DOMAIN_STATISTICS_WRITER_H_

#include "StatisticsReporter.h"

#include "lib/RelayTypeSupportImpl.h"
#include "lib/QosIndex.h"
#include "RelayHandler.h"

namespace RtpsRelay {

class DomainStatisticsWriter : public StatisticsReporter {
public:
  explicit DomainStatisticsWriter(const RelayHandlerConfig& config)
    : StatisticsReporter()
    , config_(config)
  {
    domain_statistics_.application_participant_guid(repoid_to_guid(config_.application_participant_guid()));
    domain_statistics_.local_participants(0);
    domain_statistics_.local_writers(0);
    domain_statistics_.local_readers(0);
    domain_statistics_.total_writers(0);
    domain_statistics_.total_readers(0);
  }

  void add_local_participant()
  {
    ++domain_statistics_.local_participants();
  }

  void remove_local_participant()
  {
    --domain_statistics_.local_participants();
  }

  void add_local_writer()
  {
    ++domain_statistics_.local_writers();
  }

  void remove_local_writer()
  {
    --domain_statistics_.local_writers();
  }

  void add_local_reader()
  {
    ++domain_statistics_.local_readers();
  }

  void remove_local_reader()
  {
    --domain_statistics_.local_readers();
  }

  void total_writers(size_t count)
  {
    domain_statistics_.total_writers(count);
  }

  void total_readers(size_t count)
  {
    domain_statistics_.total_readers(count);
  }

  void report(const OpenDDS::DCPS::MonotonicTimePoint& time_now) override
  {
    ACE_UNUSED_ARG(time_now);
    write_sample();

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
  }

  void reset_stats() override
  {
    // These stats don't reset
  }

private:

  void write_sample()
  {
    if (config_.domain_statistics_writer()) {
      const auto ret = config_.domain_statistics_writer()->write(domain_statistics_, DDS::HANDLE_NIL);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: DomainStatisticsWriter::write_sample failed to write statistics\n")));
      }
    }
  }

  const RelayHandlerConfig& config_;
  DomainStatistics domain_statistics_;
};

}

#endif // RTPSRELAY_PUBLICATION_LISTENER_H_
