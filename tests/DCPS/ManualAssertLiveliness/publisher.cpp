// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"
#include "DataWriterListenerImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/transport/tcp/TcpInst.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include <ace/streams.h>
#include "ace/Get_Opt.h"

using namespace Messenger;

int LEASE_DURATION_SEC = 2;
int assert_liveliness_period = LEASE_DURATION_SEC;
int num_messages = 10;
bool liveliness_lost_test = false;
int num_liveliness_lost_callbacks = 2;

int
parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("t:n:lc:"));
  int c;

  while ((c = get_opts ()) != -1)
  {
    switch (c)
    {
    case 't':
      assert_liveliness_period = ACE_OS::atoi (get_opts.opt_arg());
      break;
    case 'n':
      num_messages = ACE_OS::atoi (get_opts.opt_arg());
      break;
    case 'l':
      liveliness_lost_test = true;
      break;
    case 'c':
      num_liveliness_lost_callbacks = ACE_OS::atoi (get_opts.opt_arg());
      break;
    case '?':
    default:
      ACE_ERROR_RETURN ((LM_ERROR,
        "usage:  %s "
        "-l <assert_liveliness_period>\n",
        argv [0]),
        -1);
    }
  }
  // Indicates sucessful parsing of the command line
  return 0;
}

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]) {
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);
      DDS::DomainParticipant_var participant =
        dpf->create_participant(411,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1;
      }

      if (parse_args (argc, argv) == -1) {
        return -1;
      }

      MessageTypeSupport_var mts = new MessageTypeSupportImpl();

      if (DDS::RETCODE_OK != mts->register_type(participant.in (), "")) {
        cerr << "register_type failed." << endl;
        exit(1);
      }

      CORBA::String_var type_name = mts->get_type_name ();

      DDS::Topic_var topic =
        participant->create_topic ("Movie Discussion List",
                                   type_name.in (),
                                   TOPIC_QOS_DEFAULT,
                                   DDS::TopicListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ())) {
        cerr << "create_topic failed." << endl;
        exit(1);
      }

      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ())) {
        cerr << "create_publisher failed." << endl;
        exit(1);
      }

      DataWriterListenerImpl * dwl_servant = new DataWriterListenerImpl;
      ::DDS::DataWriterListener_var dwl (dwl_servant);

      // Create the datawriters
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.liveliness.kind = ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
      dw_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
      dw_qos.liveliness.lease_duration.nanosec = 0;

      // Create the datawriter
      DDS::DataWriter_var dw1 =
        pub->create_datawriter(topic.in (),
                               dw_qos,
                               dwl.in(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw1.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }
      // Create the datawriter
      DDS::DataWriter_var dw2 =
        pub->create_datawriter(topic.in (),
                               dw_qos,
                               dwl.in(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw2.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }

      dw_qos.liveliness.kind = ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
      // Create the datawriter
      DDS::DataWriter_var dw3 =
        pub->create_datawriter(topic.in (),
                               dw_qos,
                               dwl.in(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw2.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }
      // Create the datawriter
      DDS::DataWriter_var dw4 =
        pub->create_datawriter(topic.in (),
                               dw_qos,
                               dwl.in(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw2.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }

      Manual_By_Participant_Writer_1* writer1 = new Manual_By_Participant_Writer_1(dw1.in());
      Manual_By_Participant_Writer_2* writer2 = new Manual_By_Participant_Writer_2(participant.in(), dw2.in());
      Manual_By_Topic_Writer_1* writer3 = new Manual_By_Topic_Writer_1(dw3.in());
      Manual_By_Topic_Writer_2* writer4 = new Manual_By_Topic_Writer_2(dw4.in());

      writer1->start ();
      writer2->start ();
      writer3->start ();
      writer4->start ();

      writer1->end ();
      writer2->end ();
      writer3->end ();
      writer4->end ();

      delete writer1;
      delete writer2;
      delete writer3;
      delete writer4;

      if (liveliness_lost_test
        && dwl_servant->num_liveliness_lost_callbacks () != num_liveliness_lost_callbacks)
      {
        cerr << "ERROR: did not receive expected liveliness lost callbacks. "
          << dwl_servant->num_liveliness_lost_callbacks () << "/" <<
          num_liveliness_lost_callbacks << endl;
      }

      participant->delete_contained_entities();
      dpf->delete_participant(participant.in ());
      TheServiceParticipant->shutdown ();
  }
  catch (CORBA::Exception& e)
    {
      cerr << "PUB: Exception caught in main.cpp:" << endl
         << e << endl;
      exit(1);
    }

  return 0;
}
