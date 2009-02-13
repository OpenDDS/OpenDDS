/*
 * $Id$
 */

#include <cstdlib>

#include <ace/Arg_Shifter.h>
#include <ace/Log_Msg.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>

#include "FooTypeTypeSupportImpl.h"

int expected_samples = 0;

void
parse_args(int& argc, char** argv)
{
  ACE_Arg_Shifter_T<char> shifter(argc, argv);

  while (shifter.is_anything_left())
  {
    const char* arg;

    // expected samples
    if ((arg = shifter.get_the_parameter("-n")))
    {
      expected_samples = std::atoi(arg);
      shifter.consume_arg();
    }
    else
    {
      shifter.ignore_arg();
    }
  }
}

int
main(int argc, char** argv)
{
  parse_args(argc, argv);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Initializing subscriber; expected_samples: %s\n"),
             expected_samples));

  DDS::DomainParticipantFactory_var dpf =
    TheParticipantFactoryWithArgs(argc, argv);

  // Create Participant
  DDS::DomainParticipant_var participant =
    dpf->create_participant(42,
                            PARTICIPANT_QOS_DEFAULT,
                            DDS::DomainParticipantListener::_nil());

  if (CORBA::is_nil(participant.in()))
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" create_participant failed!\n")), 1);

  // Create Subscriber
  DDS::Subscriber_var subscriber =
    participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                   DDS::SubscriberListener::_nil());

  if (CORBA::is_nil(subscriber.in()))
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" create_subscriber failed!\n")), 1);

  // Attach Transport
  OpenDDS::DCPS::TransportImpl_rch transport =
    TheTransportFactory->create_transport_impl(1);

  OpenDDS::DCPS::SubscriberImpl* subscriber_i =
    dynamic_cast<OpenDDS::DCPS::SubscriberImpl*>(subscriber.in());

  if (subscriber_i == 0)
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" dynamic_cast failed!\n")), 1);

  OpenDDS::DCPS::AttachStatus status =
    subscriber_i->attach_transport(transport.in());
  
  if (status != OpenDDS::DCPS::ATTACH_OK)
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" attach_transport failed!\n")), 1);

  // Register Type (FooType)
  FooTypeSupport_var ts = new FooTypeSupportImpl;
  if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK)
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" register_type failed!\n")), 1);

  // Create Topic (FooTopic)
  DDS::Topic_var topic =
    participant->create_topic("FooTopic",
                              ts->get_type_name(),
                              TOPIC_QOS_DEFAULT,
                              DDS::TopicListener::_nil());

  if (CORBA::is_nil(topic.in()))
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" create_topic failed!\n")), 1);

  // ...

  participant->delete_contained_entities();
  dpf->delete_participant(participant.in());

  TheServiceParticipant->shutdown();

  return 0;
}
