#include "PublicationListener.h"

#include "utility.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

PublicationListener::PublicationListener(const Config& config,
                                         OpenDDS::DCPS::DomainParticipantImpl* participant,
                                         WriterEntryDataWriter_var writer,
                                         DomainStatisticsReporter& stats_reporter)
  : config_(config)
  , participant_(participant)
  , writer_(writer)
  , stats_reporter_(stats_reporter)
  , unregister_(OpenDDS::DCPS::make_rch<Unregister>(OpenDDS::DCPS::ref(*this)))
{}

void PublicationListener::on_data_available(DDS::DataReader_ptr reader)
{
  DDS::PublicationBuiltinTopicDataDataReader_var dr = DDS::PublicationBuiltinTopicDataDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: PublicationListener::on_data_available failed to narrow PublicationBuiltinTopicDataDataReader\n")));
    return;
  }

  DDS::PublicationBuiltinTopicDataSeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->take(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: PublicationListener::on_data_available failed to read\n")));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      write_sample(data[idx], infos[idx]);
      stats_reporter_.add_local_writer(OpenDDS::DCPS::MonotonicTimePoint::now());
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      unregister_instance(infos[idx]);
      stats_reporter_.remove_local_writer(OpenDDS::DCPS::MonotonicTimePoint::now());
      break;
    }
  }
}

void PublicationListener::write_sample(const DDS::PublicationBuiltinTopicData& data,
                                       const DDS::SampleInfo& info)
{
  const auto repoid = participant_->get_repoid(info.instance_handle);
  GUID_t guid;
  assign(guid, repoid);

  DDS::DataWriterQos data_writer_qos;
  data_writer_qos.durability = data.durability;
  data_writer_qos.durability_service = data.durability_service;
  data_writer_qos.deadline = data.deadline;
  data_writer_qos.latency_budget = data.latency_budget;
  data_writer_qos.liveliness = data.liveliness;
  data_writer_qos.reliability = data.reliability;
  data_writer_qos.destination_order = data.destination_order;
  // data_writer_qos.history not used.
  data_writer_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
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

  const WriterEntry entry {
    guid,

    data.topic_name.in(),
    data.type_name.in(),
    data_writer_qos,
    publisher_qos,
  };

  if (config_.log_discovery()) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: PublicationLister::write_sample add local writer %C %C\n"), guid_to_string(repoid).c_str(), OpenDDS::DCPS::to_json(data).c_str()));
  }
  DDS::ReturnCode_t ret = writer_->write(entry, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: PublicationListener::write_sample failed to write\n")));
  }
}

void PublicationListener::unregister_instance(const DDS::SampleInfo& info)
{
  const auto repoid = participant_->get_repoid(info.instance_handle);
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  unregister_queue_.push_back(repoid);
}

void PublicationListener::unregister()
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  if (unregister_queue_.empty()) {
    return;
  }

  const auto repoid = unregister_queue_.front();
  GUID_t guid;
  assign(guid, repoid);

  WriterEntry entry;
  entry.guid(guid);

  if (config_.log_discovery()) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: PublicationListener::unregister remove local writer %C\n"), guid_to_string(repoid).c_str()));
  }
  DDS::ReturnCode_t ret = writer_->unregister_instance(entry, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: PublicationListener::unregister failed to unregister_instance\n")));
  }
}

PublicationListener::Unregister::Unregister(PublicationListener& listener)
  : listener_(listener)
  , unregister_task_(TheServiceParticipant->interceptor(), *this, &PublicationListener::Unregister::execute)
{
  unregister_task_.enable(false, OpenDDS::DCPS::TimeDuration(1));
}

PublicationListener::Unregister::~Unregister()
{
  unregister_task_.disable_and_wait();
}

void PublicationListener::Unregister::execute(const OpenDDS::DCPS::MonotonicTimePoint&)
{
  listener_.unregister();
}

}
