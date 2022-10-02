// -*- C++ -*-
#include "ZeroEnumTypeSupportImpl.h"

#include "tests/Utils/DistributedConditionSet.h"
#include "tests/Utils/StatusMatching.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include "dds/DCPS/StaticIncludes.h"

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  DistributedConditionSet_rch distributed_condition_set =
    OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  DDS::DomainParticipantFactory_var domain_participant_factory = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipant_var participant =
    domain_participant_factory->create_participant(ZeroEnum::HELLO_WORLD_DOMAIN,
                                                   PARTICIPANT_QOS_DEFAULT,
                                                   0,
                                                   0);

  ZeroEnum::MessageTypeSupport_var type_support = new ZeroEnum::MessageTypeSupportImpl();
  type_support->register_type(participant, "");
  CORBA::String_var type_name = type_support->get_type_name();

  DDS::Topic_var topic = participant->create_topic(ZeroEnum::MESSAGE_TOPIC_NAME,
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

  Utils::wait_match(data_writer, 1);
  distributed_condition_set->wait_for(ZeroEnum::MAIN, ZeroEnum::LISTENER, ZeroEnum::ON_RECORDER_MATCHED);

  ZeroEnum::MessageDataWriter_var message_data_writer = ZeroEnum::MessageDataWriter::_narrow(data_writer);

  ZeroEnum::Message message;
  message.value = "Hello World!";
  message.my_enum = ZeroEnum::E1;

  message_data_writer->write(message, DDS::HANDLE_NIL);
  distributed_condition_set->wait_for(ZeroEnum::MAIN, ZeroEnum::LISTENER, ZeroEnum::ON_SAMPLE_DATA_RECEIVED);

  participant->delete_contained_entities();
  domain_participant_factory->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return EXIT_SUCCESS;
}
