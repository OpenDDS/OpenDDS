#include "WriterListener.h"

namespace RtpsRelay {

WriterListener::WriterListener(const Config& config,
                               AssociationTable& association_table,
                               SpdpHandler& spdp_handler,
                               DomainStatisticsReporter& stats_reporter)
  : config_(config)
  , association_table_(association_table)
  , spdp_handler_(spdp_handler)
  , stats_reporter_(stats_reporter)
{}

void WriterListener::on_data_available(DDS::DataReader_ptr reader)
{
  WriterEntryDataReader_var dr = WriterEntryDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: WriterListener::on_data_available failed to narrow RtpsRelay::WriterEntryDataReader\n")));
    return;
  }

  WriterEntrySeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->take(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: WriterListener::on_data_available failed to read\n")));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      {
        const auto from = guid_to_repoid(data[idx].guid());
        GuidSet to_before, to_after;
        if (config_.log_entries()) {
          ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: WriterListener::on_data_available add global writer %C\n"), guid_to_string(from).c_str()));
        }
        association_table_.lookup_destinations(to_before, from);
        association_table_.insert(data[idx], to_after);
        for (const auto& to : to_before) {
          to_after.erase(to);
        }
        spdp_handler_.replay(from, to_after);
        stats_reporter_.total_writers(association_table_.writer_count(), OpenDDS::DCPS::MonotonicTimePoint::now());
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      if (config_.log_entries()) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: WriterListener::on_data_available remove global writer %C\n"), guid_to_string(guid_to_repoid(data[idx].guid())).c_str()));
      }
      association_table_.remove(data[idx]);
      stats_reporter_.total_writers(association_table_.writer_count(), OpenDDS::DCPS::MonotonicTimePoint::now());
      break;
    }
  }
}

}
