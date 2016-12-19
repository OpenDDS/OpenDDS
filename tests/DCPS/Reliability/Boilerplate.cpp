/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Boilerplate.h"
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <iostream>

namespace examples { namespace boilerplate {

DDS::DomainParticipant_var
createParticipant(DDS::DomainParticipantFactory_var dpf)
{
  // Create DomainParticipant
  DDS::DomainParticipant_var participant =
    dpf->create_participant(42, // made-up domain ID
                            PARTICIPANT_QOS_DEFAULT,
                            0,  // no listener
                            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Check for failure
  if (!participant) {
    throw std::string("failed to create domain participant");
  }
  return participant;
}

DDS::Topic_var
createTopic(DDS::DomainParticipant_var participant)
{
  // Register TypeSupport (Messenger::Message)
  Reliability::MessageTypeSupport_var ts =
    new Reliability::MessageTypeSupportImpl;

  if (ts->register_type(participant, "") != DDS::RETCODE_OK) {
    throw std::string("failed to register type support");
  }

  CORBA::String_var type_name = ts->get_type_name();

  DDS::Topic_var topic =
    participant->create_topic("transaction_test",
                              type_name,
                              TOPIC_QOS_DEFAULT,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Check for failure
  if (!topic) {
    throw std::string("failed to create topic");
  }
  return topic;
}

DDS::Publisher_var
createPublisher(DDS::DomainParticipant_var participant)
{
  // Create Publisher
  DDS::Publisher_var publisher =
    participant->create_publisher(
        PUBLISHER_QOS_DEFAULT,
        0,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Check for failure
  if (!publisher) {
    throw std::string("failed to create publisher");
  }
  return publisher;
}

DDS::Subscriber_var
createSubscriber(DDS::DomainParticipant_var participant)
{
  // Create Subscriber
  DDS::Subscriber_var subscriber =
      participant->create_subscriber(
          SUBSCRIBER_QOS_DEFAULT,
          0,
          OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Check for failure
  if (!subscriber) {
    throw std::string("failed to create subscriber");
  }

  return subscriber;
}

DDS::DataWriter_var
createDataWriter(
  DDS::Publisher_var publisher,
  DDS::Topic_var topic,
  bool keep_last_one)
{
  // Set qos
  DDS::DataWriterQos dw_qos;
  publisher->get_default_datawriter_qos(dw_qos);
  // RELIABLE/KEEP_ALL/10/10 works
  dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  dw_qos.reliability.max_blocking_time.sec = 1;
  dw_qos.reliability.max_blocking_time.nanosec = 0;

  if (keep_last_one) {
    dw_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
    dw_qos.history.depth = 1;
    dw_qos.resource_limits.max_samples = 1;
    dw_qos.resource_limits.max_samples_per_instance = 1;
    std::cout << "Datawriter QOS keep last one" << std::endl;
  } else {
    dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    dw_qos.resource_limits.max_samples = 1000;
    dw_qos.resource_limits.max_samples_per_instance = 1000;
  }
  // Create DataWriter
  DDS::DataWriter_var writer =
    publisher->create_datawriter(topic,
                                 dw_qos,
                                 0,
                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Check for failure
  if (!writer) {
    throw std::string("failed to create data writer");
  }

  return writer;
}

DDS::DataReader_var
createDataReader(
  DDS::Subscriber_var subscriber,
  DDS::Topic_var topic,
  DDS::DataReaderListener_var listener,
  bool keep_last_one)
{
  // Set qos
  DDS::DataReaderQos dr_qos;
  // RELIABLE/KEEP_LAST/10 works
  subscriber->get_default_datareader_qos(dr_qos);
  dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  if (keep_last_one) {
    dr_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
    dr_qos.history.depth = 1;
    std::cout << "Datareader QOS keep last one" << std::endl;
  } else {
    dr_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
    dr_qos.history.depth = 10;
  }
  // Create DataReader
  DDS::DataReader_var reader =
    subscriber->create_datareader(topic,
                                  dr_qos,
                                  listener,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Check for failure
  if (!reader) {
    throw std::string("failed to create data reader");
  }

  return reader;
}

Reliability::MessageDataWriter_var
narrow(DDS::DataWriter_var writer)
{
  // Safe cast to type-specific data writer
  Reliability::MessageDataWriter_var message_writer =
    Reliability::MessageDataWriter::_narrow(writer);

  // Check for failure
  if (!message_writer) {
    throw std::string("failed to narrow data writer");
  }

  return message_writer;
}

Reliability::MessageDataReader_var
narrow(DDS::DataReader_var reader)
{
  // Safe cast to type-specific data reader
  Reliability::MessageDataReader_var message_reader =
    Reliability::MessageDataReader::_narrow(reader);

  // Check for failure
  if (!message_reader) {
    throw std::string("failed to narrow data reader");
  }

  return message_reader;
}

Reliability::MessageDataWriter_var
narrow(DDS::DataWriter_ptr writer)
{
  // Safe cast to type-specific data writer
  Reliability::MessageDataWriter_var ReliabilityWriter =
    Reliability::MessageDataWriter::_narrow(writer);

  // Check for failure
  if (!ReliabilityWriter) {
    throw std::string("failed to narrow data writer");
  }

  return ReliabilityWriter;
}

Reliability::MessageDataReader_var
narrow(DDS::DataReader_ptr reader)
{
  // Safe cast to type-specific data reader
  Reliability::MessageDataReader_var message_reader =
    Reliability::MessageDataReader::_narrow(reader);

  // Check for failure
  if (!message_reader) {
    throw std::string("failed to narrow data reader");
  }

  return message_reader;
}

void
cleanup(
  DDS::DomainParticipant_var participant,
  DDS::DomainParticipantFactory_var dpf)
{
  std::cout << "delete_contained_entities"<< std::endl;
  // Delete any topics, publishers and subscribers owned by participant
  participant->delete_contained_entities();
  std::cout << "delete_participant"<< std::endl;
  // Delete participant itself
  dpf->delete_participant(participant);
  std::cout << "shutdown"<< std::endl;
  // Shut down info repo connection
  TheServiceParticipant->shutdown();
  std::cout << "shutdown complete"<< std::endl;
}

} } // End namespaces
