#include "GroupTable.h"

#include <dds/DCPS/Marked_Default_Qos.h>

bool GroupTable::initialize(DDS::DomainParticipant_var a_participant,
                            const std::string& a_topic_name)
{
  RtpsRelay::GroupEntryTypeSupportImpl::_var_type routing_table_ts = new RtpsRelay::GroupEntryTypeSupportImpl;
  if (routing_table_ts->register_type(a_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to register type\n"));
    return false;
  }

  CORBA::String_var type_name = routing_table_ts->get_type_name();
  DDS::Topic_var topic = a_participant->create_topic(a_topic_name.c_str(), type_name,
                                                     TOPIC_QOS_DEFAULT, nullptr,
                                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create topic\n"));
    return false;
  }

  DDS::Publisher_var publisher = a_participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr,
                                                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!publisher) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create publisher\n"));
    return false;
  }

  DDS::Subscriber_var subscriber = a_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr,
                                                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!subscriber) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create subscriber\n"));
    return false;
  }

  {
    DDS::DataWriterQos qos;
    publisher->get_default_datawriter_qos(qos);

    qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    qos.lifespan.duration.sec = static_cast<int>(lifespan_.sec());
    qos.lifespan.duration.nanosec = 0;

    DDS::DataWriter_var writer = publisher->create_datawriter(topic, qos, nullptr,
                                                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!writer) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create data writer\n"));
      return false;
    }

    writer_ = RtpsRelay::GroupEntryDataWriter::_narrow(writer);

    if (!writer_) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to narrow data writer\n"));
      return false;
    }
  }

  {
    DDS::DataReaderQos qos;
    subscriber->get_default_datareader_qos(qos);

    qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataReader_var reader = subscriber->create_datareader(topic, qos, nullptr,
                                                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!reader) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create data reader\n"));
      return false;
    }

    reader_ = RtpsRelay::GroupEntryDataReader::_narrow(reader);

    if (!reader_) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to narrow data reader\n"));
      return false;
    }

    DDS::StringSeq one_empty_str;
    one_empty_str.length(1);
    group_query_condition_ = reader_->create_querycondition(DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE,
                                                            DDS::ALIVE_INSTANCE_STATE, "group = %0",
                                                            one_empty_str);

    if (!group_query_condition_) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create group query condition\n"));
      return false;
    }

    guid_query_condition_ = reader_->create_querycondition(DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE,
                                                           DDS::ALIVE_INSTANCE_STATE, "guid = %0",
                                                           one_empty_str);

    if (!guid_query_condition_) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create guid query condition\n"));
      return false;
    }
  }

  return true;
}

void GroupTable::update(const std::string& a_guid, const GroupSet& a_groups, const ACE_Time_Value& a_now)
{
  auto pos1 = a_groups.begin(), limit1 = a_groups.end();

  const auto prev = groups(a_guid);
  auto pos2 = prev.begin(), limit2 = prev.end();

  while (pos1 != limit1 && pos2 != limit2) {
    if (*pos1 < *pos2) {
      insert(a_guid, *pos1, a_now);
      ++pos1;
    } else if(*pos1 > *pos2) {
      remove(a_guid, *pos2);
      ++pos2;
    } else {
      // No change.
      ++pos1;
      ++pos2;
    }
  }

  while (pos1 != limit1) {
    insert(a_guid, *pos1, a_now);
    ++pos1;
  }

  while (pos2 != limit2) {
    remove(a_guid, *pos2);
    ++pos2;
  }
}

GroupTable::GuidSet GroupTable::guids(const std::string& a_group) const
{
  DDS::StringSeq group_query_parameters;
  group_query_parameters.length(1);
  group_query_parameters[0] = a_group.c_str();
  auto rc = group_query_condition_->set_query_parameters(group_query_parameters);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::guids failed to set query parameters\n"));
    return GuidSet();
  }

  RtpsRelay::GroupEntrySeq received_data;
  DDS::SampleInfoSeq info_seq;
  rc = reader_->read_w_condition(received_data, info_seq, DDS::LENGTH_UNLIMITED, group_query_condition_);
  if (rc == DDS::RETCODE_NO_DATA) {
    ACE_DEBUG((LM_WARNING, "(%P|%t) %N:%l WARNING: GroupTable::guids no guids for group %C\n", a_group.c_str()));
    return GuidSet();
  }
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::guids failed to read with condition\n"));
    return GuidSet();
  }

  GuidSet retval;
  for (auto idx = 0u; idx < received_data.length(); ++idx) {
    retval.insert(received_data[idx].guid());
  }

  return retval;
}

GroupTable::GroupSet GroupTable::groups(const std::string& a_guid) const 
{
  DDS::StringSeq guid_query_parameters;
  guid_query_parameters.length(1);
  guid_query_parameters[0] = a_guid.c_str();
  auto rc = guid_query_condition_->set_query_parameters(guid_query_parameters);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::groups failed to set query parameters\n"));
    return GroupSet();
  }

  RtpsRelay::GroupEntrySeq received_data;
  DDS::SampleInfoSeq info_seq;
  rc = reader_->read_w_condition(received_data, info_seq, DDS::LENGTH_UNLIMITED, guid_query_condition_);
  if (rc == DDS::RETCODE_NO_DATA) {
    ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: GroupTable::groups no groups for guid %s\n", a_guid.c_str()));
    return GroupSet();
  }
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::groups failed to read with condition\n"));
    return GroupSet();
  }

  GroupSet retval;
  for (auto idx = 0u; idx < received_data.length(); ++idx) {
    retval.insert(received_data[idx].group());
  }

  return retval;
}

void GroupTable::insert(const std::string& a_guid, const std::string& a_group, const ACE_Time_Value& a_now)
{
  const RtpsRelay::GroupEntry entry{a_guid, a_group, static_cast<uint32_t>((a_now + lifespan_).sec())};

  // Read the existing.
  auto instance_handle = reader_->lookup_instance(entry);
  if (instance_handle != DDS::HANDLE_NIL) {
    RtpsRelay::GroupEntrySeq received_data(1);
    DDS::SampleInfoSeq info_seq(1);
    const auto rc = reader_->read_instance(received_data, info_seq, 1, instance_handle,
                                           DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

    if (rc == DDS::RETCODE_OK &&
        received_data[0].group() == a_group &&
        ACE_Time_Value(received_data[0].expiration_timestamp()) - a_now < lifespan_ - renew_after_) {
      // Not different or not ready to renew.
      return;
    }
  }

  const auto rc = writer_->write(entry, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::insert failed to write\n"));
  }
}

void GroupTable::remove(const std::string& a_guid, const std::string& a_group)
{
  RtpsRelay::GroupEntry entry;
  entry.guid(a_guid);
  entry.group(a_group);

  const auto rc = writer_->dispose(entry, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::remove failed to dispose\n"));
  }
}
