#include "SubscriptionListener.h"

#include "utility.h"

namespace RtpsRelay {

SubscriptionListener::SubscriptionListener(OpenDDS::DCPS::DomainParticipantImpl* participant,
                                           ReaderEntryDataWriter_var writer,
                                           DomainStatisticsReporter& stats_reporter)
  : participant_(participant)
  , writer_(writer)
  , stats_reporter_(stats_reporter)
{}

void SubscriptionListener::on_data_available(DDS::DataReader_ptr reader)
{
  DDS::SubscriptionBuiltinTopicDataDataReader_var dr = DDS::SubscriptionBuiltinTopicDataDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: SubscriptionListener::on_data_available failed to narrow PublicationBuiltinTopicDataDataReader\n")));
    return;
  }

  DDS::SubscriptionBuiltinTopicDataSeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->read(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: SubscriptionListener::on_data_available failed to read\n")));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      write_sample(data[idx], infos[idx]);
      stats_reporter_.add_local_reader(OpenDDS::DCPS::MonotonicTimePoint::now());
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      unregister_instance(infos[idx]);
      stats_reporter_.remove_local_reader(OpenDDS::DCPS::MonotonicTimePoint::now());
      break;
    }
  }
}

void SubscriptionListener::write_sample(const DDS::SubscriptionBuiltinTopicData& data,
                                        const DDS::SampleInfo& info)
{
  const auto repoid = participant_->get_repoid(info.instance_handle);
  GUID_t guid;
  assign(guid, repoid);

  DDS::DataReaderQos data_reader_qos;
  data_reader_qos.durability = data.durability;
  data_reader_qos.deadline = data.deadline;
  data_reader_qos.latency_budget = data.latency_budget;
  data_reader_qos.liveliness = data.liveliness;
  data_reader_qos.reliability = data.reliability;
  data_reader_qos.destination_order = data.destination_order;
  // data_reader_qos.history not used.
  data_reader_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
  // data_reader_qos.resource_limits not used.
  data_reader_qos.user_data = data.user_data;
  data_reader_qos.ownership = data.ownership;
  data_reader_qos.time_based_filter = data.time_based_filter;
  // data_reader_qos.reader_data_lifecycle not used.

  DDS::SubscriberQos subscriber_qos;
  subscriber_qos.presentation = data.presentation;
  subscriber_qos.partition = data.partition;
  subscriber_qos.group_data = data.group_data;
  // subscriber_qos.entity_factory not used.

  const ReaderEntry entry {
    guid,

    data.topic_name.in(),
    data.type_name.in(),
    data_reader_qos,
    subscriber_qos,
  };

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: SubscriptionListener::write_sample add local reader %C\n"), guid_to_string(repoid).c_str()));
  DDS::ReturnCode_t ret = writer_->write(entry, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: SubscriptionListener::write_sample failed to write\n")));
  }
}

void SubscriptionListener::unregister_instance(const DDS::SampleInfo& info)
{
  const auto repoid = participant_->get_repoid(info.instance_handle);
  GUID_t guid;
  assign(guid, repoid);

  ReaderEntry entry;
  entry.guid(guid);

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: SubscriptionListener::unregister_instance remove local reader %C\n"), guid_to_string(repoid).c_str()));
  DDS::ReturnCode_t ret = writer_->unregister_instance(entry, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: SubscriptionListener::unregister_instance failed to unregister_instance\n")));
  }
}

}
