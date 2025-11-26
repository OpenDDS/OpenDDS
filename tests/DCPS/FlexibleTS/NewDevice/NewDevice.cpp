#include "NewDeviceTypeSupportImpl.h"

#include <tests/Utils/DistributedConditionSet.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/StaticIncludes.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "../Common.h"

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  // Coordination across processes.
  DistributedConditionSet_rch distributed_condition_set =
    OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  DDS::DomainParticipantFactory_var domain_participant_factory = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipantQos participant_qos;
  domain_participant_factory->get_default_participant_qos(participant_qos);
  participant_qos.user_data.value.length(sizeof HelloWorld::NEWDEV);
  std::memcpy(participant_qos.user_data.value.get_buffer(), HelloWorld::NEWDEV, sizeof HelloWorld::NEWDEV);
  DDS::DomainParticipant_var participant =
    domain_participant_factory->create_participant(HelloWorld::HELLO_WORLD_DOMAIN, participant_qos, 0, 0);

  HelloWorld::MessageTypeSupport_var type_support = new HelloWorld::MessageTypeSupportImpl;
  type_support->register_type(participant, "");
  CORBA::String_var type_name = type_support->get_type_name();

  DDS::Topic_var topic = participant->create_topic(HelloWorld::STATUS_TOPIC_NAME, type_name,
                                                   TOPIC_QOS_DEFAULT, 0, 0);

  DDS::Publisher_var publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, 0, 0);

  DDS::DataWriter_var data_writer = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, 0, 0);

  HelloWorld::MessageDataWriter_var message_data_writer = HelloWorld::MessageDataWriter::_narrow(data_writer);

  DDS::Topic_var topic2 = participant->create_topic(HelloWorld::COMMAND_TOPIC_NAME, type_name,
                                                    TOPIC_QOS_DEFAULT, 0, 0);

  DDS::Subscriber_var subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, 0);

  DDS::DataReaderQos reader_qos;
  subscriber->get_default_datareader_qos(reader_qos);
  reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataReader_var data_reader = subscriber->create_datareader(topic2, reader_qos, 0, 0);

  HelloWorld::MessageDataReader_var message_data_reader = HelloWorld::MessageDataReader::_narrow(data_reader);

  distributed_condition_set->wait_for(HelloWorld::NEWDEV, HelloWorld::CONTROLLER, HelloWorld::CONTROLLER_READY);

  HelloWorld::Message message;
  message.deviceId = HelloWorld::NEWDEV;
  message.st = HelloWorld::Initial;
  message.added = true;
  message_data_writer->write(message, DDS::HANDLE_NIL);

  DDS::ReadCondition_var read_condition =
    data_reader->create_readcondition(DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  DDS::WaitSet_var wait_set = new DDS::WaitSet;
  wait_set->attach_condition(read_condition);

  bool done = false;
  while (!done) {
    DDS::ConditionSeq conditions;
    const DDS::Duration_t timeout = { 1, 0 };
    wait_set->wait(conditions, timeout);

    HelloWorld::MessageSeq messages;
    DDS::SampleInfoSeq infos;
    message_data_reader->take(messages, infos, DDS::LENGTH_UNLIMITED,
                              DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    for (unsigned int idx = 0; idx != messages.length(); ++idx) {
      if (infos[idx].valid_data && 0 == std::strcmp(messages[idx].deviceId.in(), HelloWorld::NEWDEV)) {
        ACE_DEBUG((LM_DEBUG, "received %C %d\n", messages[idx].deviceId.in(), messages[idx].st));
        distributed_condition_set->post(HelloWorld::NEWDEV, HelloWorld::NEWDEV_DONE);
        done = true;
        break;
      }
    }
  }

  wait_set->detach_condition(read_condition);

  participant->delete_contained_entities();
  domain_participant_factory->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return 0;
}
