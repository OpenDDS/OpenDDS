// -*- C++ -*-
#include "HelloWorldTypeSupportImpl.h"

#include <tests/Utils/DistributedConditionSet.h>
#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/StaticIncludes.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif
#include <dds/DCPS/QOS_XML_Handler/QOS_XML_Loader.h>

# define QOS_CONFIG "qos#TestProfile"

using namespace DDS;

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  // Production apps should check the return values.
  // For tests, it just makes the code noisy.

  // Load QoS from XML
  OpenDDS::DCPS::QOS_XML_Loader _xml_loader;
  DomainParticipantQos dp_qos;
  TopicQos t_qos;
  SubscriberQos s_qos;
  DDS::DataReaderQos dr_qos;

  if (_xml_loader.init(ACE_TEXT(QOS_CONFIG)) == DDS::RETCODE_OK)
  {
    _xml_loader.get_participant_qos(dp_qos, ACE_TEXT(QOS_CONFIG));
    _xml_loader.get_topic_qos(t_qos, ACE_TEXT(QOS_CONFIG), "Message");
    _xml_loader.get_subscriber_qos(s_qos, ACE_TEXT(QOS_CONFIG));
    _xml_loader.get_datareader_qos(dr_qos, ACE_TEXT(QOS_CONFIG), "Message");
  }

  DDS::DomainParticipantFactory_var domain_participant_factory = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipant_var participant =
    domain_participant_factory->create_participant(HelloWorld::HELLO_WORLD_DOMAIN,
                                                   dp_qos,
                                                   0,
                                                   0);

  HelloWorld::MessageTypeSupport_var type_support = new HelloWorld::MessageTypeSupportImpl;
  type_support->register_type(participant, "");
  CORBA::String_var type_name = type_support->get_type_name();

  DDS::Topic_var topic = participant->create_topic(HelloWorld::MESSAGE_TOPIC_NAME,
                                                   type_name,
                                                   t_qos,
                                                   0,
                                                   0);

  if (!topic)
  {
  	printf(">>> BUG_REPORT_ERROR <<< Topic was not created correctly (proof: durability_service.history_depth = %d)\n", t_qos.durability_service.history_depth);
  }

  DDS::Subscriber_var subscriber = participant->create_subscriber(s_qos,
                                                                  0,
                                                                  0);

  DDS::DataReaderQos data_reader_qos;
  subscriber->get_default_datareader_qos(data_reader_qos);
  data_reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataReader_var data_reader = subscriber->create_datareader(topic,
                                                                  dr_qos,
                                                                  0,
                                                                  0);


  /* DO NOTHING */


  participant->delete_contained_entities();
  domain_participant_factory->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return 0;
}
