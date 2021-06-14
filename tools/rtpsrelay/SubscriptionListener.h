#ifndef RTPSRELAY_SUBSCRIPTION_LISTENER_H_
#define RTPSRELAY_SUBSCRIPTION_LISTENER_H_

#include "DomainStatisticsReporter.h"
#include "GuidPartitionTable.h"
#include "ListenerBase.h"

#include <dds/DCPS/DomainParticipantImpl.h>

namespace RtpsRelay {

class SubscriptionListener : public ListenerBase {
public:
  SubscriptionListener(const Config& config,
                       OpenDDS::DCPS::DomainParticipantImpl* participant,
                       GuidPartitionTable& guid_partition_table,
                       DomainStatisticsReporter& stats_reporter);

private:
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;

  const Config& config_;
  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  GuidPartitionTable& guid_partition_table_;
  DomainStatisticsReporter& stats_reporter_;
};

}

#endif // RTPSRELAY_SUBSCRIPTION_LISTENER_H_
