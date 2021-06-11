#include "PublicationListener.h"

#include "lib/Utility.h"

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

PublicationListener::PublicationListener(const Config& config,
                                         OpenDDS::DCPS::DomainParticipantImpl* participant,
                                         GuidPartitionTable& guid_partition_table,
                                         DomainStatisticsReporter& stats_reporter)
  : config_(config)
  , participant_(participant)
  , guid_partition_table_(guid_partition_table)
  , stats_reporter_(stats_reporter)
{}

void PublicationListener::on_data_available(DDS::DataReader_ptr reader)
{
  DDS::PublicationBuiltinTopicDataDataReader_var dr = DDS::PublicationBuiltinTopicDataDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: PublicationListener::on_data_available failed to narrow PublicationBuiltinTopicDataDataReader\n")));
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
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: PublicationListener::on_data_available failed to take %C\n"), OpenDDS::DCPS::retcode_to_string(ret)));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      {
        const auto& data = datas[idx];
        const auto& info = infos[idx];
        if (info.valid_data) {
          const auto repoid = participant_->get_repoid(info.instance_handle);

          if (config_.log_discovery()) {
            ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: PublicationListener::on_data_available add local writer %C %C\n"), guid_to_string(repoid).c_str(), OpenDDS::DCPS::to_json(data).c_str()));
          }

          guid_partition_table_.insert(repoid, data.partition.name);
          stats_reporter_.add_local_writer(OpenDDS::DCPS::MonotonicTimePoint::now());
        }
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      {
        const auto& info = infos[idx];

        const auto repoid = participant_->get_repoid(info.instance_handle);

        if (config_.log_discovery()) {
          ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: PublicationListener::on_data_available remove local writer %C\n"), guid_to_string(repoid).c_str()));
        }
        guid_partition_table_.remove(repoid);
        stats_reporter_.remove_local_writer(OpenDDS::DCPS::MonotonicTimePoint::now());
      }
      break;
    }
  }
}

}
