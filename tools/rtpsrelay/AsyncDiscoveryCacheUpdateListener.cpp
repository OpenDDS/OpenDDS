#include "AsyncDiscoveryCacheUpdateListener.h"

namespace RtpsRelay {

void AsyncDiscoveryCacheUpdateListener::on_data_available(DDS::DataReader_ptr reader)
{
  AsyncDiscoveryCacheUpdateDataReader_var async_disc_reader = AsyncDiscoveryCacheUpdateDataReader::_narrow(reader);
  if (!async_disc_reader) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: AsyncDiscoveryCacheUpdateListener::on_data_available: failed to narrow AsyncDiscoveryCacheUpdateDataReader\n"));
    return;
  }

  AsyncDiscoveryCacheUpdateSeq datas;
  DDS::SampleInfoSeq infos;
  const auto rc = async_disc_reader->take(datas,
                                          infos,
                                          DDS::LENGTH_UNLIMITED,
                                          DDS::NOT_READ_SAMPLE_STATE,
                                          DDS::ANY_VIEW_STATE,
                                          DDS::ANY_INSTANCE_STATE);
  if (rc == DDS::RETCODE_NO_DATA) {
    return;
  }
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: AsyncDiscoveryCacheUpdateListener::on_data_available: failed to take samples: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return;
  }

  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    const auto& data = datas[idx];
    const auto& info = infos[idx];
    switch (info.instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      if (info.valid_data) {
        if (data.relay_id() == config_.relay_id()) {
          continue;
        }
        guid_partition_table_.update_remote_cert_partitions_cache(data.entries(), data.relay_id(), now);
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      // TODO: remove cache entries from the remote relay instances?
      break;
    }
  }
}

}