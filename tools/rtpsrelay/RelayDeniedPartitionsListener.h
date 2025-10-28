#ifndef RTPSRELAY_RELAY_DENIED_PARTITIONS_LISTENER_H_
#define RTPSRELAY_RELAY_DENIED_PARTITIONS_LISTENER_H_

#include "ReaderListenerBase.h"

namespace RtpsRelay {

class RelayDeniedPartitionsListener : public ReaderListenerBase {
public:
  RelayDeniedPartitionsListener() {}

private:
  void on_data_available(DDS::DataReader_ptr reader) override;
};

}

#endif
