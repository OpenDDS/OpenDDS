#include "SpdpReplayListener.h"

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/TimeTypes.h>

namespace RtpsRelay {

SpdpReplayListener::SpdpReplayListener(SpdpHandler& spdp_handler,
  RelayStatisticsReporter& relay_statistics_reporter)
  : spdp_handler_(spdp_handler)
  , relay_statistics_reporter_(relay_statistics_reporter)
{}

void SpdpReplayListener::on_data_available(DDS::DataReader_ptr reader)
{
  SpdpReplayDataReader_var dr = SpdpReplayDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: SpdpReplayListener::on_data_available failed to narrow RtpsRelay::SpdpReplayDataReader\n")));
    return;
  }

  SpdpReplaySeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->take(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: SpdpReplayListener::on_data_available failed to take %C\n"), OpenDDS::DCPS::retcode_to_string(ret)));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    if (infos[idx].valid_data) {
      spdp_handler_.replay(data[idx]);
    }
  }
}

void SpdpReplayListener::on_subscription_matched(
  DDS::DataReader_ptr /*reader*/, const DDS::SubscriptionMatchedStatus& status)
{
  relay_statistics_reporter_.spdp_replay_pub_count(
    static_cast<uint32_t>(status.current_count), OpenDDS::DCPS::MonotonicTimePoint::now());
}

}
