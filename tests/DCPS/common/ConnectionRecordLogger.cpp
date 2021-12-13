#include "ConnectionRecordLogger.h"

#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/JsonValueWriter.h>
#include <dds/OpenddsDcpsExtTypeSupportImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Test {

#ifndef DDS_HAS_MINIMUM_BIT

class Listener : public DDS::DataReaderListener {
  virtual void on_requested_deadline_missed (::DDS::DataReader_ptr,
                                             const ::DDS::RequestedDeadlineMissedStatus&)
  {}

  virtual void on_requested_incompatible_qos (::DDS::DataReader_ptr,
                                              const ::DDS::RequestedIncompatibleQosStatus&)
  {}

  virtual void on_sample_rejected (::DDS::DataReader_ptr,
                                   const ::DDS::SampleRejectedStatus&)
  {}

  virtual void on_liveliness_changed (::DDS::DataReader_ptr,
                                      const ::DDS::LivelinessChangedStatus&)
  {}

  virtual void on_data_available (::DDS::DataReader_ptr reader)
  {
    OpenDDS::DCPS::ConnectionRecordDataReader_var r = OpenDDS::DCPS::ConnectionRecordDataReader::_narrow(reader);
    if (!r) {
      return;
    }

    OpenDDS::DCPS::ConnectionRecord sample;
    DDS::SampleInfo sample_info;
    while (r->take_next_sample(sample, sample_info) == DDS::RETCODE_OK) {
#ifdef OPENDDS_RAPIDJSON
      DDS::TopicDescription_var topic = r->get_topicdescription();
      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("%C\n"),
                 DCPS::to_json(topic, sample, sample_info).c_str()));
#endif
    }
  }

  virtual void on_subscription_matched (::DDS::DataReader_ptr,
                                        const ::DDS::SubscriptionMatchedStatus&)
  {}

  virtual void on_sample_lost (::DDS::DataReader_ptr,
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

OPENDDS_END_VERSIONED_NAMESPACE_DECL
