// -*- C++ -*-
#include "HelloWorldTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <tests/Utils/DistributedConditionSet.h>

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  // Production apps should check the return values.
  // For tests, it just makes the code noisy.

  // Coordination across processes.
  DistributedConditionSet_rch distributed_condition_set =
    OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  DDS::DomainParticipantFactory_var domain_participant_factory = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipant_var participant =
    domain_participant_factory->create_participant(HelloWorld::HELLO_WORLD_DOMAIN,
                                                   PARTICIPANT_QOS_DEFAULT,
                                                   0,
                                                   0);

  HelloWorld::MessageTypeSupport_var type_support = new HelloWorld::MessageTypeSupportImpl();
  type_support->register_type(participant, "");
  CORBA::String_var type_name = type_support->get_type_name ();

  DDS::Topic_var topic = participant->create_topic(HelloWorld::MESSAGE_TOPIC_NAME,
                                                   type_name,
                                                   TOPIC_QOS_DEFAULT,
                                                   0,
                                                   0);

  DDS::Publisher_var publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                               0,
                                                               0);

  DDS::DataWriter_var data_writer = publisher->create_datawriter(topic,
                                                                 DATAWRITER_QOS_DEFAULT,
                                                                 0,
                                                                 0);

  HelloWorld::MessageDataWriter_var message_data_writer = HelloWorld::MessageDataWriter::_narrow(data_writer);

  HelloWorld::Message message;
  message.value = "Hello World!";

  distributed_condition_set->wait_for(HelloWorld::PUBLISHER, HelloWorld::SUBSCRIBER, HelloWorld::SUBSCRIBER_READY);

  message_data_writer->write(message, DDS::HANDLE_NIL);

  distributed_condition_set->wait_for(HelloWorld::PUBLISHER, HelloWorld::SUBSCRIBER, HelloWorld::SUBSCRIBER_DONE);

  participant->delete_contained_entities();
  domain_participant_factory->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return 0;
}
