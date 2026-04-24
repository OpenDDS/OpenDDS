#ifndef RTPSRELAY_ENDPOINT_LISTENER_H_
#define RTPSRELAY_ENDPOINT_LISTENER_H_

#include "GuidAddrSet.h"
#include "GuidPartitionTable.h"
#include "ReaderListenerBase.h"

namespace RtpsRelay {

class EndpointListener : public ReaderListenerBase {
protected:
  EndpointListener(const GuidAddrSet_rch& guid_addr_set, GuidPartitionTable& guid_partition_table)
    : guid_addr_set_(guid_addr_set)
    , guid_partition_table_(guid_partition_table)
  {}

  GuidPartitionTable::Result update_partitions_info(OpenDDS::DCPS::GUID_t repoid, const DDS::StringSeq& partitions);

  GuidAddrSet_rch guid_addr_set_;
  GuidPartitionTable& guid_partition_table_;
};

}

#endif
