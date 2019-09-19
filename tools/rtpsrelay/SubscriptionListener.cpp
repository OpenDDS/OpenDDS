#include "SubscriptionListener.h"

#include "utility.h"

SubscriptionListener::SubscriptionListener(OpenDDS::DCPS::DomainParticipantImpl* participant,
                                           RtpsRelay::ReaderEntryDataWriter_ptr writer,
                                           const AssociationTable& association_table) :
  participant_(participant),
  writer_(writer),
  association_table_(association_table)
{}

void SubscriptionListener::on_requested_deadline_missed(::DDS::DataReader_ptr /*reader*/,
                                                        const ::DDS::RequestedDeadlineMissedStatus & /*status*/)
{}

void SubscriptionListener::on_requested_incompatible_qos(::DDS::DataReader_ptr /*reader*/,
                                                         const ::DDS::RequestedIncompatibleQosStatus & /*status*/)
{}

void SubscriptionListener::on_sample_rejected(::DDS::DataReader_ptr /*reader*/,
                                              const ::DDS::SampleRejectedStatus & /*status*/)
{}

void SubscriptionListener::on_liveliness_changed(::DDS::DataReader_ptr /*reader*/,
                                                 const ::DDS::LivelinessChangedStatus & /*status*/)
{}

void SubscriptionListener::on_data_available(::DDS::DataReader_ptr reader)
{
  DDS::SubscriptionBuiltinTopicDataDataReader_var dr = DDS::SubscriptionBuiltinTopicDataDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: SubscriptionListener::on_data_available failed to narrow PublicationBuiltinTopicDataDataReader\n"));
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
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: SubscriptionListener::on_data_available failed to read\n"));
    return;
  }

  for (size_t idx = 0; idx != infos.length(); ++idx) {
    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      write_sample(data[idx], infos[idx]);
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      write_dispose(infos[idx]);
      break;
    }
  }
}

void SubscriptionListener::on_subscription_matched(::DDS::DataReader_ptr /*reader*/,
                                                   const ::DDS::SubscriptionMatchedStatus & /*status*/)
{}

void SubscriptionListener::on_sample_lost(::DDS::DataReader_ptr /*reader*/,
                                          const ::DDS::SampleLostStatus & /*status*/)
{}

void SubscriptionListener::write_sample(const DDS::SubscriptionBuiltinTopicData& data,
                                        const DDS::SampleInfo& info)
{
  const OpenDDS::DCPS::RepoId id = participant_->get_repoid(info.instance_handle);
  RtpsRelay::GUID_t guid;
  std::memcpy(&guid, &id, sizeof(RtpsRelay::GUID_t));

  DDS::DataReaderQos data_reader_qos;
  data_reader_qos.durability = data.durability;
  data_reader_qos.deadline = data.deadline;
  data_reader_qos.latency_budget = data.latency_budget;
  data_reader_qos.liveliness = data.liveliness;
  data_reader_qos.reliability = data.reliability;
  data_reader_qos.destination_order = data.destination_order;
  // data_reader_qos.history not used.
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

  const RtpsRelay::ReaderEntry entry {
    guid,

      data.topic_name.in(),
      data.type_name.in(),
      data_reader_qos,
      subscriber_qos,

      association_table_.relay_addresses()
      };

  DDS::ReturnCode_t ret = writer_->write(entry, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: SubscriptionListener::write_sample failed to write\n"));
  }
}

void SubscriptionListener::write_dispose(const DDS::SampleInfo& info)
{
  const OpenDDS::DCPS::RepoId id = participant_->get_repoid(info.instance_handle);
  RtpsRelay::GUID_t guid;
  std::memcpy(&guid, &id, sizeof(RtpsRelay::GUID_t));

  RtpsRelay::ReaderEntry entry;
  entry.guid(guid);

  DDS::ReturnCode_t ret = writer_->dispose(entry, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: SubscriptionListener::write_dispose failed to dispose\n"));
  }
}
