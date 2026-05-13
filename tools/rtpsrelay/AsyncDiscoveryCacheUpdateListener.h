#ifndef RTPSRELAY_ASYNC_DISCOVERY_CACHE_UPDATE_LISTENER_H_
#define RTPSRELAY_ASYNC_DISCOVERY_CACHE_UPDATE_LISTENER_H_

#include "ReaderListenerBase.h"
#include "GuidPartitionTable.h"

namespace RtpsRelay {

class AsyncDiscoveryCacheUpdateListener : public ReaderListenerBase {
public:
  AsyncDiscoveryCacheUpdateListener(GuidPartitionTable& guid_partition_table, const Config& config)
    : guid_partition_table_(guid_partition_table), config_(config)
  {}

private:
  void on_data_available(DDS::DataReader_ptr reader) override;

  GuidPartitionTable& guid_partition_table_;
  const Config& config_;
};

}

#endif