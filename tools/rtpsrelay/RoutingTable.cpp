#include "RoutingTable.h"

#include <array>

#include <dds/DCPS/Marked_Default_Qos.h>

bool RoutingTable::initialize(DDS::DomainParticipant_var a_participant,
                              const std::string& a_topic_name)
{
  RtpsRelay::RoutingEntryTypeSupportImpl::_var_type routing_table_ts =
    new RtpsRelay::RoutingEntryTypeSupportImpl;
  if (routing_table_ts->register_type(a_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to register type\n"));
    return false;
  }

  CORBA::String_var type_name = routing_table_ts->get_type_name();
  DDS::Topic_var topic = a_participant->create_topic(a_topic_name.c_str(), type_name,
                                                     TOPIC_QOS_DEFAULT, nullptr,
                                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to create topic\n"));
    return false;
  }

  DDS::Publisher_var publisher = a_participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr,
                                                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!publisher) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to create publisher\n"));
    return false;
  }

  DDS::Subscriber_var subscriber = a_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr,
                                                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!subscriber) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to create subscriber\n"));
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
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to create data writer\n"));
      return false;
    }

    writer_ = RtpsRelay::RoutingEntryDataWriter::_narrow(writer);

    if (!writer) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to narrow data writer\n"));
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
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to create data reader\n"));
      return false;
    }

    reader_ = RtpsRelay::RoutingEntryDataReader::_narrow(reader);

    if (!reader) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::initialize failed to narrow data reader\n"));
      return false;
    }
  }

  return true;
}

bool RoutingTable::fetch(RtpsRelay::RoutingEntry& a_entry) const
{
  auto handle = reader_->lookup_instance(a_entry);
  if (handle == DDS::HANDLE_NIL) {
    return false;
  }

  RtpsRelay::RoutingEntrySeq received_data(1);
  DDS::SampleInfoSeq info_seq(1);
  const auto rc = reader_->read_instance(received_data, info_seq, 1, handle,
                                         DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  if (rc != DDS::RETCODE_OK) {
    return false;
  }

  a_entry = received_data[0];
  return true;
}

std::string RoutingTable::horizontal_relay_address(const std::string& a_guid) const
{
  RtpsRelay::RoutingEntry entry;
  entry.guid(a_guid);
  if (fetch(entry)) {
    return entry.horizontal_relay_address();
  }
  return std::string();
}

std::string RoutingTable::address(const std::string& a_guid) const
{
  RtpsRelay::RoutingEntry entry;
  entry.guid(a_guid);
  if (fetch(entry)) {
    return entry.address();
  }
  return std::string();
}

void RoutingTable::update(const std::string& a_guid,
                          const std::string& a_horizontal_relay_address,
                          const std::string& a_address,
                          const ACE_Time_Value& a_now)
{
  const RtpsRelay::RoutingEntry entry{a_guid, a_address, a_horizontal_relay_address, 
    static_cast<uint32_t>((a_now + lifespan_).sec())};

  auto reader_handle = reader_->lookup_instance(entry);
  if (reader_handle != DDS::HANDLE_NIL) {
    RtpsRelay::RoutingEntrySeq received_data(1);
    DDS::SampleInfoSeq info_seq(1);
    const auto rc = reader_->read_instance(received_data, info_seq, 1, reader_handle,
                                           DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

    if (rc == DDS::RETCODE_OK &&
        received_data[0].horizontal_relay_address() == a_horizontal_relay_address &&
        received_data[0].address() == a_address &&
        ACE_Time_Value(received_data[0].expiration_timestamp()) - a_now < lifespan_ - renew_after_) {
      // Not different or not ready to renew.
      return;
    }
  }

  const auto rc = writer_->write(entry, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RoutingTable::update failed to write\n"));
    return;
  }
}
