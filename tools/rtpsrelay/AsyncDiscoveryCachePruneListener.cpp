#include "AsyncDiscoveryCachePruneListener.h"

namespace RtpsRelay {

void AsyncDiscoveryCachePruneListener::on_data_available(DDS::DataReader_ptr reader)
{
  AsyncDiscoveryCachePruneDataReader_var async_disc_prune_reader = AsyncDiscoveryCachePruneDataReader::_narrow(reader);
  if (!async_disc_prune_reader) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: AsyncDiscoveryCachePruneListener::on_data_available: failed to narrow AsyncDiscoveryCachePruneDataReader\n"));
    return;
  }

  AsyncDiscoveryCachePruneSeq datas;
  DDS::SampleInfoSeq infos;
  const auto rc = async_disc_prune_reader->take(datas,
                                                infos,
                                                DDS::LENGTH_UNLIMITED,
                                                DDS::NOT_READ_SAMPLE_STATE,
                                                DDS::ANY_VIEW_STATE,
                                                DDS::ANY_INSTANCE_STATE);
  if (rc == DDS::RETCODE_NO_DATA) {
    return;
  }
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: AsyncDiscoveryCachePruneListener::on_data_available: failed to take samples: %C\n",
      OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    const auto& data = datas[idx];
    const auto& info = infos[idx];
    switch (info.instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      if (info.valid_data) {
        if (data.relay_id() == config_.relay_id()) {
          continue;
        }
        guid_partition_table_.handle_async_disc_cache_prune(data.keys(), data.relay_id());
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      // TODO: remove cache entries for the relay that is no longer alive?
      break;
    }
  }
}

}
