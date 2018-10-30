#include "GroupTable.h"

#include <dds/DCPS/Marked_Default_Qos.h>

int GroupTable::initialize(DDS::DomainParticipant_var a_participant,
                           std::string const & a_topic_name) {
  RtpsRelay::GroupEntryTypeSupportImpl::_var_type routing_table_ts = new RtpsRelay::GroupEntryTypeSupportImpl();
  if (routing_table_ts->register_type(a_participant.in(), "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to register type\n"));
    return -1;
  }

  DDS::Topic_var topic = a_participant->create_topic(
                                                   a_topic_name.c_str(),
                                                   routing_table_ts->get_type_name(),
                                                   TOPIC_QOS_DEFAULT,
                                                   nullptr,
                                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(topic.in())) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create topic\n"));
    return -1;
  }

  DDS::Publisher_var publisher = a_participant->create_publisher(
                                                               PUBLISHER_QOS_DEFAULT,
                                                               nullptr,
                                                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(publisher.in())) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create publisher\n"));
    return -1;
  }

  DDS::Subscriber_var subscriber = a_participant->create_subscriber(
                                                                  SUBSCRIBER_QOS_DEFAULT,
                                                                  nullptr,
                                                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(subscriber.in())) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create subscriber\n"));
    return -1;
  }

  {
    DDS::DataWriterQos qos;
    publisher->get_default_datawriter_qos(qos);

    qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    qos.lifespan.duration.sec = m_lifespan.sec();
    qos.lifespan.duration.nanosec = 0;

    DDS::DataWriter_var writer = publisher->create_datawriter(
                                                              topic.in(),
                                                              qos,
                                                              nullptr,
                                                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(writer.in())) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create data writer\n"));
      return -1;
    }

    m_writer = RtpsRelay::GroupEntryDataWriter::_narrow(writer.in());

    if (! m_writer) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to narrow data writer\n"));
      return -1;
    }
  }

  {
    DDS::DataReaderQos qos;
    subscriber->get_default_datareader_qos(qos);

    qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataReader_var reader = subscriber->create_datareader(
                                                               topic.in(),
                                                               qos,
                                                               nullptr,
                                                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create data reader\n"));
      return -1;
    }

    m_reader =  RtpsRelay::GroupEntryDataReader::_narrow(reader);

    if (!m_reader) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to narrow data reader\n"));
      return -1;
    }

    DDS::StringSeq group_query_parameters;
    group_query_parameters.length(1);
    m_group_query_condition = m_reader->create_querycondition(
                                                            DDS::ANY_SAMPLE_STATE,
                                                            DDS::ANY_VIEW_STATE,
                                                            DDS::ALIVE_INSTANCE_STATE,
                                                            "group = %0",
                                                            group_query_parameters);

    if (!m_group_query_condition) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create group query condition\n"));
      return -1;
    }

    DDS::StringSeq guid_query_parameters;
    guid_query_parameters.length(1);
    m_guid_query_condition = m_reader->create_querycondition(
                                                           DDS::ANY_SAMPLE_STATE,
                                                           DDS::ANY_VIEW_STATE,
                                                           DDS::ALIVE_INSTANCE_STATE,
                                                           "guid = %0",
                                                           guid_query_parameters);

    if (!m_guid_query_condition) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::initialize failed to create guid query condition\n"));
      return -1;
    }
  }

  return 0;
}

void GroupTable::update(std::string const & a_guid, GroupSet const & a_groups, ACE_Time_Value const & a_now) {
  const GroupSet prev = this->groups(a_guid);

  GroupSet::const_iterator pos1 = a_groups.begin(), limit1 = a_groups.end();
  GroupSet::const_iterator pos2 = prev.begin(), limit2 = prev.end();

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

GroupTable::GuidSet GroupTable::guids(std::string const & a_group) const {
  GuidSet retval;
  ::RtpsRelay::GroupEntrySeq received_data;
  ::DDS::SampleInfoSeq info_seq;

  DDS::StringSeq group_query_parameters;
  group_query_parameters.length(1);
  group_query_parameters[0] = a_group.c_str();
  DDS::ReturnCode_t rc = m_group_query_condition->set_query_parameters(group_query_parameters);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::guids failed to set query parameters\n"));
    return retval;
  }
  rc = m_reader->read_w_condition(received_data, info_seq, DDS::LENGTH_UNLIMITED, m_group_query_condition);
  if (rc == DDS::RETCODE_NO_DATA) {
    ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: GroupTable::guids no guids for group %s\n", a_group.c_str()));
  }
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::guids failed to read with condition\n"));
    return retval;
  }

  for (size_t idx = 0; idx != received_data.length(); ++idx) {
    retval.insert(received_data[idx].guid());
  }

  return retval;
}

GroupTable::GroupSet GroupTable::groups(std::string const & a_guid) const {
  GroupSet retval;
  ::RtpsRelay::GroupEntrySeq received_data;
  ::DDS::SampleInfoSeq info_seq;

  DDS::StringSeq guid_query_parameters;
  guid_query_parameters.length(1);
  guid_query_parameters[0] = a_guid.c_str();
  DDS::ReturnCode_t rc = m_guid_query_condition->set_query_parameters(guid_query_parameters);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::groups failed to set query parameters\n"));
    return retval;
  }
  rc = m_reader->read_w_condition(received_data, info_seq, DDS::LENGTH_UNLIMITED, m_guid_query_condition);
  if (rc == DDS::RETCODE_NO_DATA) {
    ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: GroupTable::groups no groups for guid %s\n", a_guid.c_str()));
    return retval;
  }
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::groups failed to read with condition\n"));
    return retval;
  }

  for (size_t idx = 0; idx != received_data.length(); ++idx) {
    retval.insert(received_data[idx].group());
  }

  return retval;
}

void GroupTable::insert(std::string const & a_guid, std::string const & a_group, ACE_Time_Value const & a_now) {
  RtpsRelay::GroupEntry entry;
  entry.guid(a_guid);
  entry.group(a_group);
  entry.expiration_timestamp((a_now + m_lifespan).sec());

  // Read the existing.
  auto reader_handle = m_reader->lookup_instance(entry);
  if (reader_handle) {
    ::RtpsRelay::GroupEntrySeq received_data(1);
    ::DDS::SampleInfoSeq info_seq(1);

    DDS::ReturnCode_t rc = m_reader->read_instance(received_data,
                                                   info_seq,
                                                   1,
                                                   reader_handle,
                                                   DDS::ANY_SAMPLE_STATE,
                                                   DDS::ANY_VIEW_STATE,
                                                   DDS::ALIVE_INSTANCE_STATE);

    if (rc == DDS::RETCODE_OK &&
        received_data[0].group() == a_group &&
        ACE_Time_Value(received_data[0].expiration_timestamp()) - a_now < m_lifespan - m_renew_after) {
      // Not different or not ready to renew.
      return;
    }
  }

  auto writer_handle = m_writer->register_instance(entry);
  if (!writer_handle) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::insert failed to register instance\n"));
    return;
  }

  DDS::ReturnCode_t rc = m_writer->write(entry, writer_handle);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::insert failed to write\n"));
    return;
  }
}

void GroupTable::remove(std::string const & a_guid, std::string const & a_group) {
  RtpsRelay::GroupEntry entry;
  entry.guid(a_guid);
  entry.group(a_group);

  auto handle = m_writer->lookup_instance(entry);
  if (!handle) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::remove failed to lookup instance\n"));
    return;
  }

  DDS::ReturnCode_t rc = m_writer->dispose(entry, handle);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: GroupTable::remove failed to dispose\n"));
  }
}
