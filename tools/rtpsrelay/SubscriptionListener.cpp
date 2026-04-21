#include "SubscriptionListener.h"

#include <dds/rtpsrelaylib/Utility.h>

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

SubscriptionListener::SubscriptionListener(const Config& config,
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

void SubscriptionListener::on_data_available(DDS::DataReader_ptr reader)
{
  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();

  DDS::SubscriptionBuiltinTopicDataDataReader_var dr = DDS::SubscriptionBuiltinTopicDataDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SubscriptionListener::on_data_available failed to narrow SubscriptionBuiltinTopicDataDataReader\n"));
    return;
  }

  DDS::SubscriptionBuiltinTopicDataSeq datas;
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
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SubscriptionListener::on_data_available failed to take %C\n", OpenDDS::DCPS::retcode_to_string(ret)));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    OpenDDS::DCPS::ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());
    const auto& data = datas[idx];
    const auto& info = infos[idx];

    using OpenDDS::DCPS::make_part_guid;

    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      if (info.valid_data) {
        const auto repoid = participant_->get_repoid(info.instance_handle);
        const auto r = guid_partition_table_.insert(repoid, data.partition.name);

        // TODO(sonndinh): Move to a common place so PublicationListener can also use it.
        // If this is an endpoint from a participant that has initiated async discovery,
        // the participant needs to be removed from the pending recipients sets of all
        // participants it has initiated async discovery with.
        const auto part_guid = make_part_guid(repoid);
        std::string cert_id;
        {
          GuidAddrSet::Proxy proxy(*guid_addr_set_);
          auto iter = proxy.find(part_guid);
          if (iter != proxy.end()) {
            const auto& initiated_async_disc_with = iter->second.initiated_async_discovery_with;
            for (const auto& other_part : initiated_async_disc_with) {
              auto other_iter = proxy.find(other_part);
              if (other_iter != proxy.end()) {
                // TODO(sonndinh): Probably also need to clean up when part_guid goes away to avoid leak when
                // part_guid goes away before PublicationListener/SubscriptionListener is called back?
                other_iter->second.pending_recipients.erase(part_guid);
              }
            }
            iter->second.initiated_async_discovery_with.clear();
            cert_id = iter->second.identity_info.get_cert_id();
          }
        }

        // Cache all partitions corresponding to this participant
        StringSet all_partitions;
        guid_partition_table_.lookup(all_partitions, part_guid);
        guid_partition_table_.update_cert_partitions_cache(cert_id, all_partitions);

        if (r == GuidPartitionTable::ADDED) {
          if (config_.log_discovery()) {
            GuidAddrSet::Proxy proxy(*guid_addr_set_);
            ACE_DEBUG((LM_INFO,
                       "(%P|%t) INFO: SubscriptionListener::on_data_available "
                       "add local reader %C %C %C into session [%u/%u]\n",
                       guid_to_string(repoid).c_str(), OpenDDS::DCPS::to_json(data).c_str(),
                       proxy.get_session_time(part_guid, now).sec_str().c_str(),
                       idx, infos.length()));
          }
          stats_reporter_.local_readers(++count_, OpenDDS::DCPS::MonotonicTimePoint::now());
        } else if (r == GuidPartitionTable::UPDATED) {
          if (config_.log_discovery()) {
            ACE_DEBUG((LM_INFO, "(%P|%t) INFO: SubscriptionListener::on_data_available update local reader %C %C [%u/%u]\n", guid_to_string(repoid).c_str(), OpenDDS::DCPS::to_json(data).c_str(),
                       idx, infos.length()));
          }
        }
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      {
        const auto repoid = participant_->get_repoid(info.instance_handle);

        if (config_.log_discovery()) {
          GuidAddrSet::Proxy proxy(*guid_addr_set_);
          ACE_DEBUG((LM_INFO, "(%P|%t) INFO: SubscriptionListener::on_data_available "
                     "remove local reader %C %C into session [%u/%u]\n",
                     guid_to_string(repoid).c_str(),
                     proxy.get_session_time(make_part_guid(repoid), now).sec_str().c_str(),
                     idx, infos.length()));
        }

        guid_partition_table_.remove(repoid);

        stats_reporter_.local_readers(--count_, OpenDDS::DCPS::MonotonicTimePoint::now());
      }
      break;
    }
  }
}

}
