/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>

#include "dds/DCPS/StaticIncludes.h"

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"

int num_messages = 5;

int
parse_args(int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("n:"));

  int c;
  while ((c = get_opts()) != -1) {
    switch (c) {
    case 'n':
      num_messages = ACE_OS::atoi (get_opts.opt_arg ());
      break;
    case '?':
    default:
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("usage: %C -n <num_messages>\n"),
                        argv[0]),
                        -1);
    }
  }

  return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
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
      dpf->create_participant(411,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_participant failed!\n")),
                       -1);
    }

    // Register TypeSupport (Messenger::Message)
    Messenger::MessageTypeSupport_var mts =
      new Messenger::MessageTypeSupportImpl();

    if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: register_type failed!\n")),
                       -1);
    }

    // Create Topic
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                CORBA::String_var(mts->get_type_name()),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_topic failed!\n")),
                       -1);
    }

    ::DDS::PublisherQos publisher_qos;
    participant->get_default_publisher_qos (publisher_qos);
    publisher_qos.presentation.access_scope = DDS::GROUP_PRESENTATION_QOS;
    publisher_qos.presentation.coherent_access = true;
    publisher_qos.presentation.ordered_access = true;

    // Create Publisher
    DDS::Publisher_var pub =
      participant->create_publisher(publisher_qos,
                                    DDS::PublisherListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(pub.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_publisher failed!\n")),
                       -1);
    }

    ::DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos (dw_qos);
    dw_qos.history.kind                             = ::DDS::KEEP_ALL_HISTORY_QOS;
    dw_qos.resource_limits.max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;

    // Create DataWriter
    DDS::DataWriter_var dw1 =
      pub->create_datawriter(topic.in(),
                             dw_qos,
                             DDS::DataWriterListener::_nil(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(dw1.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                       -1);
    }

    DDS::DataWriter_var dw2 =
      pub->create_datawriter(topic.in(),
                             dw_qos,
                             DDS::DataWriterListener::_nil(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(dw2.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                       -1);
    }

    // Start writing threads
    Writer* writer1 = new Writer(pub.in(), dw1.in());
    writer1->start();
    Writer* writer2 = new Writer(pub.in(), dw2.in());
    writer2->start();

    while (!writer1->is_finished() || !writer2->is_finished()) {
      ACE_Time_Value small_time(0, 250000);
      ACE_OS::sleep(small_time);
    }

    writer1->end();
    writer2->end();
    delete writer1;
    delete writer2;

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());

    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    ACE_OS::exit(-1);
  }

  return 0;
}
