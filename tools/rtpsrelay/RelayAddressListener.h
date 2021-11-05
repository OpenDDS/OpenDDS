#ifndef RTPSRELAY_RELAY_ADDRESS_LISTENER_H_
#define RTPSRELAY_RELAY_ADDRESS_LISTENER_H_

#include "ListenerBase.h"
#include "RelayPartitionTable.h"
#include "RelayStatisticsReporter.h"

namespace RtpsRelay {

class RelayAddressListener : public ListenerBase {
public:
  RelayAddressListener(RelayPartitionTable& relay_partition_table,
    RelayStatisticsReporter& relay_statistics_reporter);

private:
  void on_data_available(DDS::DataReader_ptr reader) override;
  void on_subscription_matched(DDS::DataReader_ptr reader,
                               const DDS::SubscriptionMatchedStatus& status) override;

  RelayPartitionTable& relay_partition_table_;
  RelayStatisticsReporter& relay_statistics_reporter_;
};

}

#endif // RTPSRELAY_RELAY_ADDRESS_LISTENER_H_
