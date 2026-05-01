#include "EndpointListener.h"

namespace RtpsRelay {

GuidPartitionTable::Result EndpointListener::update_partitions_info(OpenDDS::DCPS::GUID_t repoid, const DDS::StringSeq& partitions)
{
  const auto r = guid_partition_table_.insert(repoid, partitions);

  // If this is an endpoint from a participant that has initiated async discovery,
  // the participant needs to be removed from the pending recipients sets of all
  // participants it has initiated async discovery with.
  const auto part_guid = OpenDDS::DCPS::make_part_guid(repoid);
  std::string cert_id;
  {
    GuidAddrSet::Proxy proxy(*guid_addr_set_);
    const auto iter = proxy.find(part_guid);
    if (iter != proxy.end()) {
      proxy.cleanup_peers_pending_recipients(iter);
      iter->second.initiated_async_discovery_with.clear();
      cert_id = iter->second.identity_info.cert_id();
    }
  }

  // Cache all partitions corresponding to this participant for async discovery
  StringSet all_partitions;
  guid_partition_table_.lookup(all_partitions, part_guid);
  guid_partition_table_.update_cert_partitions_cache(cert_id, all_partitions, part_guid);
  return r;
}

}
