#include "ConnectionRecordLogger.h"

#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DiscoveryBase.h"
#include "dds/DCPS/GuidConverter.h"

namespace OpenDDS {
namespace Test {

#ifndef DDS_HAS_MINIMUM_BIT

class Listener : public DDS::DataReaderListener {
  void on_requested_deadline_missed (::DDS::DataReader_ptr,
                                     const ::DDS::RequestedDeadlineMissedStatus&)
  {}

  void on_requested_incompatible_qos (::DDS::DataReader_ptr,
                                      const ::DDS::RequestedIncompatibleQosStatus&)
  {}

  void on_sample_rejected (::DDS::DataReader_ptr,
                           const ::DDS::SampleRejectedStatus&)
  {}

  void on_liveliness_changed (::DDS::DataReader_ptr,
                              const ::DDS::LivelinessChangedStatus&)
  {}

  void on_data_available (::DDS::DataReader_ptr reader)
  {
    OpenDDS::DCPS::ConnectionRecordDataReader_var r = OpenDDS::DCPS::ConnectionRecordDataReader::_narrow(reader);
    if (!r) {
      return;
    }

    OpenDDS::DCPS::ConnectionRecord data;
    DDS::SampleInfo sample_info;
    while (r->take_next_sample(data, sample_info) == DDS::RETCODE_OK) {
      DCPS::RepoId guid;
      std::memcpy(&guid, data.guid, sizeof(guid));

      switch (sample_info.instance_state) {
      case DDS::ALIVE_INSTANCE_STATE:
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("connect guid=%C address=%C protocol=%C timestamp=%d.%09d\n"),
                   DCPS::LogGuid(guid).c_str(),
                   data.address.in(),
                   data.protocol.in(),
                   sample_info.source_timestamp.sec,
                   sample_info.source_timestamp.nanosec));
        break;
      case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
      case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("disconnect guid=%C address=%C protocol=%C timestamp=%d.%09d\n"),
                   DCPS::LogGuid(guid).c_str(),
                   data.address.in(),
                   data.protocol.in(),
                   sample_info.source_timestamp.sec,
                   sample_info.source_timestamp.nanosec));
        break;
      }

      // Time_t source_timestamp;
    }
  }

  void on_subscription_matched (::DDS::DataReader_ptr,
                                const ::DDS::SubscriptionMatchedStatus&)
  {}

  void on_sample_lost (::DDS::DataReader_ptr,
                       const ::DDS::SampleLostStatus&)
  {}
};
#endif

void install_connection_record_logger(DDS::DomainParticipant_var participant)
{
  ACE_UNUSED_ARG(participant);

#ifndef DDS_HAS_MINIMUM_BIT
  DDS::Subscriber_var bit_sub = participant->get_builtin_subscriber();
  DDS::DataReader_var d = bit_sub->lookup_datareader(DCPS::BUILT_IN_CONNECTION_RECORD_TOPIC);
  if (!d) {
    return;
  }

  DDS::DataReaderListener_var listener(new Listener());
  d->set_listener(listener, DCPS::DEFAULT_STATUS_MASK);
#endif
}

} // Test
} // OpenDDS
