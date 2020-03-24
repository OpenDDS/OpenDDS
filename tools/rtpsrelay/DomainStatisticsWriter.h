#ifndef RTPSRELAY_DOMAIN_STATISTICS_WRITER_H_
#define RTPSRELAY_DOMAIN_STATISTICS_WRITER_H_

#include "lib/RelayTypeSupportImpl.h"
#include "lib/QosIndex.h"
#include "RelayHandler.h"

namespace RtpsRelay {

class DomainStatisticsWriter {
public:
  explicit DomainStatisticsWriter(const RelayHandlerConfig& config)
    : config_(config)
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
    write_sample();
  }

  void remove_local_participant()
  {
    --domain_statistics_.local_participants();
    write_sample();
  }

  void add_local_writer()
  {
    ++domain_statistics_.local_writers();
    write_sample();
  }

  void remove_local_writer()
  {
    --domain_statistics_.local_writers();
    write_sample();
  }

  void add_local_reader()
  {
    ++domain_statistics_.local_readers();
    write_sample();
  }

  void remove_local_reader()
  {
    --domain_statistics_.local_readers();
    write_sample();
  }

  void total_writers(size_t count)
  {
    domain_statistics_.total_writers(count);
    write_sample();
  }

  void total_readers(size_t count)
  {
    domain_statistics_.total_readers(count);
    write_sample();
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
