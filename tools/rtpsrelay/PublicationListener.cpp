#include "PublicationListener.h"

#include <dds/rtpsrelaylib/Utility.h>

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

PublicationListener::PublicationListener(const Config& config,
                                         const GuidAddrSet_rch& guid_addr_set,
                                         OpenDDS::DCPS::DomainParticipantImpl* participant,
                                         GuidPartitionTable& guid_partition_table,
                                         RelayStatisticsReporter& stats_reporter)
  : config_(config)
  , guid_addr_set_(guid_addr_set)
  , participant_(participant)
  , guid_partition_table_(guid_partition_table)
  , stats_reporter_(stats_reporter)
  , count_(0)
{}

void PublicationListener::on_data_available(DDS::DataReader_ptr reader)
{
  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();

  DDS::PublicationBuiltinTopicDataDataReader_var dr = DDS::PublicationBuiltinTopicDataDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: PublicationListener::on_data_available failed to narrow PublicationBuiltinTopicDataDataReader\n"));
    return;
  }

  DDS::PublicationBuiltinTopicDataSeq datas;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->take(datas,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret == DDS::RETCODE_NO_DATA) {
    return;
  }
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: PublicationListener::on_data_available failed to take %C\n", OpenDDS::DCPS::retcode_to_string(ret)));
    return;
  }

  using OpenDDS::DCPS::make_part_guid;

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    OpenDDS::DCPS::ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());
    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      {
        const auto& data = datas[idx];
        const auto& info = infos[idx];
        if (info.valid_data) {
          const auto repoid = participant_->get_repoid(info.instance_handle);
          const auto r = guid_partition_table_.insert(repoid, data.partition.name);

          if (r == GuidPartitionTable::ADDED) {
            if (config_.log_discovery()) {
              GuidAddrSet::Proxy proxy(*guid_addr_set_);
              ACE_DEBUG((LM_INFO, "(%P|%t) INFO: PublicationListener::on_data_available "
                         "add local writer %C %C %C into session [%u/%u]\n",
                         guid_to_string(repoid).c_str(), OpenDDS::DCPS::to_json(data).c_str(),
                         proxy.get_session_time(make_part_guid(repoid), now).sec_str().c_str(),
                         idx, infos.length()));
            }
            stats_reporter_.local_writers(++count_, OpenDDS::DCPS::MonotonicTimePoint::now());
          } else if (r == GuidPartitionTable::UPDATED) {
            if (config_.log_discovery()) {
              ACE_DEBUG((LM_INFO, "(%P|%t) INFO: PublicationListener::on_data_available update local writer %C %C [%u/%u]\n", guid_to_string(repoid).c_str(), OpenDDS::DCPS::to_json(data).c_str(),
                         idx, infos.length()));
            }
          }
        }
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      {
        const auto& info = infos[idx];

        const auto repoid = participant_->get_repoid(info.instance_handle);

        if (config_.log_discovery()) {
          GuidAddrSet::Proxy proxy(*guid_addr_set_);
          ACE_DEBUG((LM_INFO, "(%P|%t) INFO: PublicationListener::on_data_available "
                     "remove local writer %C %C into session [%u/%u]\n",
                     guid_to_string(repoid).c_str(),
                     proxy.get_session_time(make_part_guid(repoid), now).sec_str().c_str(),
                     idx, infos.length()));
        }
        guid_partition_table_.remove(repoid);
        stats_reporter_.local_writers(--count_, OpenDDS::DCPS::MonotonicTimePoint::now());
      }
      break;
    }
  }
}

}
