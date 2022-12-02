#include "HelloWorldTypeSupportImpl.h"

#include <tests/Utils/DistributedConditionSet.h>
#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/StaticIncludes.h>

#include <dds/DCPS/XTypes/DynamicTypeSupport.h>

#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/Argv_Type_Converter.h>

void read_dynamic(DDS::DataReader_var& dr, bool& done, bool& got_message);
void read_plain(DDS::DataReader_var& dr, bool& done, bool& got_message);

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  // Production apps should check the return values.
  // For tests, it just makes the code noisy.

  // Coordination across processes.
  DistributedConditionSet_rch distributed_condition_set =
    OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  DDS::DomainParticipantFactory_var domain_participant_factory = TheParticipantFactoryWithArgs(argc, argv);

  ACE_Argv_Type_Converter conv(argc, argv);
  char** argva = conv.get_ASCII_argv();
  bool dynamic = false;
  for (int i = 1; i < argc; ++i) {
    if (0 == std::strcmp("-dynamic", argva[i])) {
      dynamic = true;
      break;
    }
  }

  ACE_DEBUG((LM_DEBUG, "Testing %C language binding\n", dynamic ? "dynamic" : "plain"));

  DDS::DomainParticipant_var participant =
    domain_participant_factory->create_participant(HelloWorld::HELLO_WORLD_DOMAIN,
                                                   PARTICIPANT_QOS_DEFAULT,
                                                   0,
                                                   0);

  HelloWorld::MessageTypeSupport_var type_support = new HelloWorld::MessageTypeSupportImpl;
  CORBA::String_var type_name = type_support->get_type_name ();
  DDS::DynamicType_var dt;

  if (dynamic) {
    dt = type_support->get_type();
    DDS::TypeSupport_var dts = new DDS::DynamicTypeSupport(dt);
    dts->register_type(participant, type_name);
  } else {
    type_support->register_type(participant, type_name);
  }

  DDS::Topic_var topic = participant->create_topic(HelloWorld::MESSAGE_TOPIC_NAME,
                                                   type_name,
                                                   TOPIC_QOS_DEFAULT,
                                                   0,
                                                   0);

  DDS::Subscriber_var subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                                  0,
                                                                  0);

  DDS::DataReaderQos data_reader_qos;
  subscriber->get_default_datareader_qos(data_reader_qos);
  data_reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataReader_var data_reader = subscriber->create_datareader(topic,
                                                                  data_reader_qos,
                                                                  0,
                                                                  0);

  Utils::wait_match(data_reader, 1);
  distributed_condition_set->post(HelloWorld::SUBSCRIBER, HelloWorld::SUBSCRIBER_READY);

  DDS::ReadCondition_var read_condition =
    data_reader->create_readcondition(DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  DDS::WaitSet_var wait_set = new DDS::WaitSet;
  wait_set->attach_condition(read_condition);

  bool done = false;
  bool got_message = false;
  while (!done) {
    DDS::ConditionSeq conditions;
    const DDS::Duration_t timeout = { 1, 0 };
    wait_set->wait(conditions, timeout);

    if (dynamic) {
      read_dynamic(data_reader, done, got_message);
    } else {
      read_plain(data_reader, done, got_message);
    }

    if (done) {
      distributed_condition_set->post(HelloWorld::SUBSCRIBER, HelloWorld::SUBSCRIBER_DONE);
    }
  }

  wait_set->detach_condition(read_condition);

  participant->delete_contained_entities();
  domain_participant_factory->delete_participant(participant);
  TheServiceParticipant->shutdown();

  if (!got_message) {
    ACE_ERROR((LM_ERROR, "subscriber (%P|%t) failed to get expected message\n"));
    return 1;
  }

  return 0;
}

void read_plain(DDS::DataReader_var& data_reader, bool& done, bool& got_message)
{
  HelloWorld::MessageDataReader_var message_data_reader = HelloWorld::MessageDataReader::_narrow(data_reader);
  HelloWorld::MessageSeq messages;
  DDS::SampleInfoSeq infos;
  message_data_reader->take(messages, infos, DDS::LENGTH_UNLIMITED,
                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  for (unsigned int idx = 0; idx != messages.length(); ++idx) {
    if (!strcmp(messages[idx].value.in(), "Hello, World!")) {
      got_message = true;
    }
    if (infos[idx].valid_data) {
      ACE_DEBUG((LM_DEBUG, "received %C\n", messages[idx].value.in()));
      done = true;
    }
  }
}

void read_dynamic(DDS::DataReader_var& data_reader, bool& done, bool& got_message)
{
  DDS::DynamicDataReader_var ddr = DDS::DynamicDataReader::_narrow(data_reader);
  DDS::DynamicDataSeq messages;
  DDS::SampleInfoSeq infos;
  ddr->take(messages, infos, DDS::LENGTH_UNLIMITED,
            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  for (unsigned int idx = 0; idx != messages.length(); ++idx) {
    CORBA::String_var str;
    messages[idx]->get_string_value(str, 0);
    if (!strcmp(str.in(), "Hello, World!")) {
      got_message = true;
    }
    if (infos[idx].valid_data) {
      ACE_DEBUG((LM_DEBUG, "received %C\n", str.in()));
      done = true;
    }
  }
}

