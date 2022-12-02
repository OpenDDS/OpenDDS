#include "HelloWorldTypeSupportImpl.h"

#include <tests/Utils/DistributedConditionSet.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/XTypes/DynamicTypeSupport.h>
#include <dds/DCPS/XTypes/DynamicDataFactory.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

bool check_rc(DDS::ReturnCode_t rc, const char* what)
{
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "publisher (%P|%t) ERROR: %C: %C\n",
      what, OpenDDS::DCPS::retcode_to_string(rc)));
    return true;
  }
  return false;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  // Production apps should check the return values.
  // For tests, it just makes the code noisy.

  // Coordination across processes.
  DistributedConditionSet_rch distributed_condition_set =
    OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  DDS::DomainParticipantFactory_var domain_participant_factory = TheParticipantFactoryWithArgs(argc, argv);

  bool dynamic = false;
  for (int i = 1; i < argc; ++i) {
    if (0 == std::strcmp("-dynamic", argv[i])) {
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

  HelloWorld::MessageTypeSupport_var type_support = new HelloWorld::MessageTypeSupportImpl();
  CORBA::String_var type_name = type_support->get_type_name();
  DDS::DynamicType_var dt;

  if (dynamic) {
    dt = type_support->get_type();
    DDS::TypeSupport_var dts = new DDS::DynamicTypeSupport(dt);
    if (check_rc(dts->register_type(participant, type_name), "register_type (dynamic) failed")) {
      return 1;
    }
  } else {
    if (check_rc(type_support->register_type(participant, type_name), "register_type (plain) failed")) {
      return 1;
    }
  }

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

  distributed_condition_set->wait_for(HelloWorld::PUBLISHER, HelloWorld::SUBSCRIBER, HelloWorld::SUBSCRIBER_READY);

  if (dynamic) {
    DDS::DynamicDataWriter_var ddw = DDS::DynamicDataWriter::_narrow(data_writer);
    DDS::DynamicData_var dd = DDS::DynamicDataFactory::get_instance()->create_data(dt);
    if (check_rc(dd->set_string_value(0, "Hello, World!"), "set_string_value failed")) {
      return 1;
    }
    if (check_rc(ddw->write(dd, DDS::HANDLE_NIL), "write (dynamic) failed")) {
      return 1;
    }
  } else {
    HelloWorld::MessageDataWriter_var mdw = HelloWorld::MessageDataWriter::_narrow(data_writer);
    HelloWorld::Message msg;
    msg.value = "Hello, World!";
    if (check_rc(mdw->write(msg, DDS::HANDLE_NIL), "write (plain) failed")) {
      return 1;
    }
  }

  distributed_condition_set->wait_for(HelloWorld::PUBLISHER, HelloWorld::SUBSCRIBER, HelloWorld::SUBSCRIBER_DONE);

  participant->delete_contained_entities();
  domain_participant_factory->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return 0;
}
