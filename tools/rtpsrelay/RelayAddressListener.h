#ifndef RTPSRELAY_RELAY_ADDRESS_LISTENER_H_
#define RTPSRELAY_RELAY_ADDRESS_LISTENER_H_

#include "ListenerBase.h"
#include "RelayPartitionTable.h"

namespace RtpsRelay {

class RelayAddressListener : public ListenerBase {
public:
  RelayAddressListener(RelayPartitionTable& relay_partition_table);

private:
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;

  RelayPartitionTable& relay_partition_table_;
};

}

#endif // RTPSRELAY_RELAY_ADDRESS_LISTENER_H_
