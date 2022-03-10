/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataReaderListener.h"

#include "SubscriberListener.h"
#include "MessengerTypeSupportImpl.h"

#include <tests/Utils/StatusMatching.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/StaticIncludes.h>

#include <ace/Argv_Type_Converter.h>
#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

unsigned int num_messages = 5;
int access_scope = ::DDS::GROUP_PRESENTATION_QOS;

int
parse_args(int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("n:q:"));

  int c;
  while ((c = get_opts()) != -1) {
    switch (c) {
    case 'n':
      num_messages = ACE_OS::atoi(get_opts.opt_arg());
      break;
    case 'q':
      access_scope = ACE_OS::atoi(get_opts.opt_arg());
      break;
    case '?':
    default:
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("usage: %C ")
                        ACE_TEXT("-n <num_messages>\n"), argv[0]),
                       -1);
    }
  }

  return 0;
}


int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    int error;
    if ((error = parse_args(argc, argv)) != 0) {
      return error;
    }

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(111,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_participant() failed!\n")), -1);
    }

    // Register Type (Messenger::Message)
    Messenger::MessageTypeSupport_var ts =
      new Messenger::MessageTypeSupportImpl();

    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }

    // Create Topic (Movie Discussion List)
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                CORBA::String_var(ts->get_type_name()),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_topic() failed!\n")), -1);
    }

    ::DDS::SubscriberQos subscriber_qos;
    participant->get_default_subscriber_qos(subscriber_qos);
    subscriber_qos.presentation.access_scope
      = (::DDS::PresentationQosPolicyAccessScopeKind) access_scope;
    subscriber_qos.presentation.coherent_access = true;
    subscriber_qos.presentation.ordered_access = true;

    SubscriberListenerImpl* subscriber_listener_svt = new SubscriberListenerImpl();
    DDS::SubscriberListener_var subscriber_listener(subscriber_listener_svt);

    // Create Subscriber
    DDS::Subscriber_var sub =
      participant->create_subscriber(subscriber_qos,
                                     subscriber_listener.in(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(sub.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_subscriber() failed!\n")), -1);
    }

    // Create DataReader
    DataReaderListenerImpl* listener_svt1 = new DataReaderListenerImpl("DataReader1");
    DataReaderListenerImpl* listener_svt2 = new DataReaderListenerImpl("DataReader2");

    DDS::DataReaderListener_var listener1(listener_svt1);
    DDS::DataReaderListener_var listener2(listener_svt2);

    ::DDS::DataReaderQos readerQos;
    sub->get_default_datareader_qos( readerQos);

    readerQos.history.kind                             = ::DDS::KEEP_ALL_HISTORY_QOS;
    readerQos.resource_limits.max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;

    DDS::DataReader_var reader1 =
      sub->create_datareader(topic.in(),
                             readerQos,
                             listener1.in(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader1.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    DDS::DataReader_var reader2 =
      sub->create_datareader(topic.in(),
                             readerQos,
                             listener2.in(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader2.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    // Block until all data received
    if (subscriber_qos.presentation.access_scope == DDS::INSTANCE_PRESENTATION_QOS) {
      listener_svt1->wait_valid_data(10);
      listener_svt2->wait_valid_data(10);
    } else {
      subscriber_listener_svt->wait_valid_data(20);
    }

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());

    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  return 0;
}
