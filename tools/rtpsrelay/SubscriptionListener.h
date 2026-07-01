#ifndef RTPSRELAY_SUBSCRIPTION_LISTENER_H_
#define RTPSRELAY_SUBSCRIPTION_LISTENER_H_

#include "RelayStatisticsReporter.h"
#include "GuidAddrSet.h"
#include "GuidPartitionTable.h"
#include "EndpointListener.h"

#include <dds/DCPS/DomainParticipantImpl.h>

namespace RtpsRelay {

class SubscriptionListener : public EndpointListener {
public:
  SubscriptionListener(const Config& config,
                       const GuidAddrSet_rch& guid_addr_set,
                       OpenDDS::DCPS::DomainParticipantImpl* participant,
                       GuidPartitionTable& guid_partition_table,
                       RelayStatisticsReporter& stats_reporter);

private:
  void on_data_available(DDS::DataReader_ptr reader) override;

  const Config& config_;
  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  RelayStatisticsReporter& stats_reporter_;
  OpenDDS::DCPS::Atomic<size_t> count_;
};

}

#endif // RTPSRELAY_SUBSCRIPTION_LISTENER_H_
