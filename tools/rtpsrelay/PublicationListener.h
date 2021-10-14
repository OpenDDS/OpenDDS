#ifndef RTPSRELAY_PUBLICATION_LISTENER_H_
#define RTPSRELAY_PUBLICATION_LISTENER_H_

#include "RelayStatisticsReporter.h"
#include "GuidPartitionTable.h"
#include "ListenerBase.h"
#include "RelayHandler.h"

#include <dds/DCPS/DomainParticipantImpl.h>

namespace RtpsRelay {

class PublicationListener : public ListenerBase {
public:
  PublicationListener(const Config& config,
                      GuidAddrSet& guid_addr_set,
                      OpenDDS::DCPS::DomainParticipantImpl* participant,
                      GuidPartitionTable& guid_partition_table,
                      RelayStatisticsReporter& stats_reporter);

private:
  void on_data_available(DDS::DataReader_ptr reader) override;

  const Config& config_;
  GuidAddrSet& guid_addr_set_;
  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  GuidPartitionTable& guid_partition_table_;
  RelayStatisticsReporter& stats_reporter_;
  size_t count_;
};

}

#endif // RTPSRELAY_PUBLICATION_LISTENER_H_
