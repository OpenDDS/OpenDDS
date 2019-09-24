#include "PublicationListener.h"

#include "utility.h"

PublicationListener::PublicationListener(OpenDDS::DCPS::DomainParticipantImpl* participant,
                                         RtpsRelay::WriterEntryDataWriter_ptr writer,
                                         const AssociationTable& association_table) :
  participant_(participant),
  writer_(writer),
  association_table_(association_table)
{}

void PublicationListener::on_requested_deadline_missed(DDS::DataReader_ptr /*reader*/,
                                                       const DDS::RequestedDeadlineMissedStatus & /*status*/)
{}

void PublicationListener::on_requested_incompatible_qos(DDS::DataReader_ptr /*reader*/,
                                                        const DDS::RequestedIncompatibleQosStatus & /*status*/)
{}

void PublicationListener::on_sample_rejected(DDS::DataReader_ptr /*reader*/,
                                             const DDS::SampleRejectedStatus & /*status*/)
{}

void PublicationListener::on_liveliness_changed(DDS::DataReader_ptr /*reader*/,
                                                const DDS::LivelinessChangedStatus & /*status*/)
{}

void PublicationListener::on_data_available(DDS::DataReader_ptr reader)
{
  DDS::PublicationBuiltinTopicDataDataReader_var dr = DDS::PublicationBuiltinTopicDataDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: PublicationListener::on_data_available failed to narrow PublicationBuiltinTopicDataDataReader\n"));
    return;
  }

  DDS::PublicationBuiltinTopicDataSeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->read(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: PublicationListener::on_data_available failed to read\n"));
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

void PublicationListener::on_subscription_matched(DDS::DataReader_ptr /*reader*/,
                                                  const DDS::SubscriptionMatchedStatus & /*status*/)
{}

void PublicationListener::on_sample_lost(DDS::DataReader_ptr /*reader*/,
                                         const DDS::SampleLostStatus & /*status*/)
{}

void PublicationListener::write_sample(const DDS::PublicationBuiltinTopicData& data,
                                       const DDS::SampleInfo& info)
{
  const OpenDDS::DCPS::RepoId id = participant_->get_repoid(info.instance_handle);
  RtpsRelay::GUID_t guid;
  std::memcpy(&guid, &id, sizeof(RtpsRelay::GUID_t));

  DDS::DataWriterQos data_writer_qos;
  data_writer_qos.durability = data.durability;
  data_writer_qos.durability_service = data.durability_service;
  data_writer_qos.deadline = data.deadline;
  data_writer_qos.latency_budget = data.latency_budget;
  data_writer_qos.liveliness = data.liveliness;
  data_writer_qos.reliability = data.reliability;
  data_writer_qos.destination_order = data.destination_order;
  // data_writer_qos.history not used.
  // data_writer_qos.resource_limit not used.
  // data_writer_qos.transport_priority not used.
  data_writer_qos.lifespan = data.lifespan;
  data_writer_qos.user_data = data.user_data;
  data_writer_qos.ownership = data.ownership;
  data_writer_qos.ownership_strength = data.ownership_strength;
  // data_writer_qos.writer_data_lifecycle not used.

  DDS::PublisherQos publisher_qos;
  publisher_qos.presentation = data.presentation;
  publisher_qos.partition = data.partition;
  publisher_qos.group_data = data.group_data;
  // publisher_qos.entity_factory not used.

  const RtpsRelay::WriterEntry entry {
    guid,

    data.topic_name.in(),
    data.type_name.in(),
    data_writer_qos,
    publisher_qos,

    association_table_.relay_addresses()
  };

  DDS::ReturnCode_t ret = writer_->write(entry, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: PublicationListener::write_sample failed to write\n"));
  }
}

void PublicationListener::write_dispose(const DDS::SampleInfo& info)
{
  const OpenDDS::DCPS::RepoId id = participant_->get_repoid(info.instance_handle);
  RtpsRelay::GUID_t guid;
  std::memcpy(&guid, &id, sizeof(RtpsRelay::GUID_t));

  RtpsRelay::WriterEntry entry;
  entry.guid(guid);

  DDS::ReturnCode_t ret = writer_->dispose(entry, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: PublicationListener::write_dispose failed to dispose\n"));
  }
}
