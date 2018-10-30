#include "RoutingTable.h"

#include <array>

#include <dds/DCPS/Marked_Default_Qos.h>

int RoutingTable::initialize(DDS::DomainParticipant_var a_participant,
                             std::string const & a_topic_name) {
  RtpsRelay::RoutingEntryTypeSupportImpl::_var_type routing_table_ts =
    new RtpsRelay::RoutingEntryTypeSupportImpl();
  if (routing_table_ts->register_type(a_participant.in(), "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to register type\n"));
    return -1;
  }

  DDS::Topic_var topic = a_participant->create_topic(
                                                   a_topic_name.c_str(),
                                                   routing_table_ts->get_type_name(),
                                                   TOPIC_QOS_DEFAULT,
                                                   nullptr,
                                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(topic.in())) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to create topic\n"));
    return -1;
  }

  DDS::Publisher_var publisher = a_participant->create_publisher(
                                                               PUBLISHER_QOS_DEFAULT,
                                                               nullptr,
                                                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(publisher.in())) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to create publisher\n"));
    return -1;
  }

  DDS::Subscriber_var subscriber = a_participant->create_subscriber(
                                                                  SUBSCRIBER_QOS_DEFAULT,
                                                                  nullptr,
                                                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(subscriber.in())) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to create subscriber\n"));
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
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to create data writer\n"));
      return -1;
    }

    m_writer = RtpsRelay::RoutingEntryDataWriter::_narrow(writer.in());

    if (!writer) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to narrow data writer\n"));
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
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to create data reader\n"));
      return -1;
    }

    m_reader =  RtpsRelay::RoutingEntryDataReader::_narrow(reader.in());

    if (!reader) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to narrow data reader\n"));
      return -1;
    }
  }

  return 0;
}

bool RoutingTable::fetch(RtpsRelay::RoutingEntry & a_entry) const {
  auto handle = m_reader->lookup_instance(a_entry);
  if (!handle) {
    return false;
  }

  ::RtpsRelay::RoutingEntrySeq received_data(1);
  ::DDS::SampleInfoSeq info_seq(1);

  DDS::ReturnCode_t rc = m_reader->read_instance(received_data,
                                                info_seq,
                                                1,
                                                handle,
                                                DDS::ANY_SAMPLE_STATE,
                                                DDS::ANY_VIEW_STATE,
                                                DDS::ALIVE_INSTANCE_STATE);

  if (rc != DDS::RETCODE_OK) {
    return false;
  }

  a_entry = received_data[0];
  return true;
}

std::string RoutingTable::horizontal_relay_address(std::string const & a_guid) const {
  RtpsRelay::RoutingEntry entry;
  entry.guid(a_guid);
  if (fetch(entry)) {
    return entry.horizontal_relay_address();;
  }
  return std::string();
}

std::string RoutingTable::address(std::string const & a_guid) const {
  RtpsRelay::RoutingEntry entry;
  entry.guid(a_guid);
  if (fetch(entry)) {
    return entry.address();
  }
  return std::string();
}

void RoutingTable::update(std::string const & a_guid,
                          std::string const & a_horizontal_relay_address,
                          std::string const & a_address,
                          ACE_Time_Value const & a_now) {
  RtpsRelay::RoutingEntry entry;
  entry.guid(a_guid);
  entry.horizontal_relay_address(a_horizontal_relay_address);
  entry.address(a_address);
  entry.expiration_timestamp((a_now + m_lifespan).sec());

  auto reader_handle = m_reader->lookup_instance(entry);
  if (reader_handle) {
    ::RtpsRelay::RoutingEntrySeq received_data(1);
    ::DDS::SampleInfoSeq info_seq(1);

    DDS::ReturnCode_t rc = m_reader->read_instance(received_data,
                                                   info_seq,
                                                   1,
                                                   reader_handle,
                                                   DDS::ANY_SAMPLE_STATE,
                                                   DDS::ANY_VIEW_STATE,
                                                   DDS::ALIVE_INSTANCE_STATE);

    if (rc == DDS::RETCODE_OK &&
        received_data[0].horizontal_relay_address() == a_horizontal_relay_address &&
        received_data[0].address() == a_address &&
        ACE_Time_Value(received_data[0].expiration_timestamp()) - a_now < m_lifespan - m_renew_after) {
      // Not different or not ready to renew.
      return;
    }
  }


  auto handle = m_writer->register_instance(entry);
  if (!handle) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::update failed to register instance\n"));
    return;
  }

  DDS::ReturnCode_t rc = m_writer->write(entry, handle);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::update failed to write\n"));
    return;
  }
}
