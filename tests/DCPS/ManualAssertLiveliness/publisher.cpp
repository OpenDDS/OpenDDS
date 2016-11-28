// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
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
#include "dds/DCPS/StaticIncludes.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"
#include "ace/OS_NS_unistd.h"

using namespace Messenger;
using namespace std;

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
        "[-t <assert_liveliness_period>]\n"
        "[-n <num_messages>]\n"
        "[-l]\n"
        "[-c <num_callbacks>]\n",
        argv [0]),
        -1);
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]) {
  int status = 0;
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
      DDS::DomainParticipant_var participant2 =
        dpf->create_participant(411,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant2.in ())) {
        cerr << "create_participant failed." << endl;
        return 1;
      }

      OpenDDS::DCPS::TransportConfig_rch cfg = TheTransportRegistry->get_config("part1");
      if (!cfg.is_nil()) {
        TheTransportRegistry->bind_config(cfg, participant);
      }
      cfg = TheTransportRegistry->get_config("part2");
      if (!cfg.is_nil()) {
        TheTransportRegistry->bind_config(cfg, participant2);
      }

      if (parse_args (argc, argv) == -1) {
        return -1;
      }

      MessageTypeSupport_var mts = new MessageTypeSupportImpl();
      MessageTypeSupport_var mts2 = new MessageTypeSupportImpl();

      if (DDS::RETCODE_OK != mts->register_type(participant.in (), "")) {
        cerr << "register_type failed." << endl;
        exit(1);
      }
      if (DDS::RETCODE_OK != mts2->register_type(participant2.in (), "")) {
        cerr << "register_type failed." << endl;
        exit(1);
      }

      CORBA::String_var type_name = mts->get_type_name ();
      CORBA::String_var type_name2 = mts2->get_type_name ();

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
      DDS::Topic_var topic2 =
        participant2->create_topic ("Movie Discussion List",
                                   type_name2.in (),
                                   TOPIC_QOS_DEFAULT,
                                   DDS::TopicListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic2.in ())) {
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
      DDS::Publisher_var pub2 =
        participant2->create_publisher(PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub2.in ())) {
        cerr << "create_publisher failed." << endl;
        exit(1);
      }

      DataWriterListenerImpl * dwl1_servant = new DataWriterListenerImpl;
      ::DDS::DataWriterListener_var dwl1 (dwl1_servant);
      DataWriterListenerImpl * dwl2_servant = new DataWriterListenerImpl;
      ::DDS::DataWriterListener_var dwl2 (dwl2_servant);
      DataWriterListenerImpl * dwl3_servant = new DataWriterListenerImpl;
      ::DDS::DataWriterListener_var dwl3 (dwl3_servant);
      DataWriterListenerImpl * dwl4_servant = new DataWriterListenerImpl;
      ::DDS::DataWriterListener_var dwl4 (dwl4_servant);

      // Create the datawriters
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);
      ::DDS::DataWriterQos dw2_qos;
      pub2->get_default_datawriter_qos (dw2_qos);

      dw_qos.liveliness.kind = ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
      dw_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
      dw_qos.liveliness.lease_duration.nanosec = 0;
      dw2_qos.liveliness.kind = ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
      dw2_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
      dw2_qos.liveliness.lease_duration.nanosec = 0;

      // Create the datawriter
      DDS::DataWriter_var dw1 =
        pub->create_datawriter(topic.in (),
                               dw_qos,
                               dwl1.in(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw1.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }
      // Create the datawriter
      DDS::DataWriter_var dw2 =
        pub2->create_datawriter(topic2.in (),
                               dw2_qos,
                               dwl2.in(),
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
                               dwl3.in(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw3.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }
      // Create the datawriter
      DDS::DataWriter_var dw4 =
        pub->create_datawriter(topic.in (),
                               dw_qos,
                               dwl4.in(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw4.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }

      {
        Write_Samples writer1(dw1, "Manual_By_Participant_Writer_1");
        Assert_Participant_Liveliness writer2(dw2, "Manual_By_Participant_Writer_2");
        Write_Samples writer3(dw3, "Manual_By_Topic_Writer_1");
        Assert_Writer_Liveliness writer4(dw4, "Manual_By_Topic_Writer_2");

        writer1.start();
        writer2.start();
        writer3.start();
        writer4.start();

        writer1.end();
        writer2.end();
        writer3.end();
        writer4.end();
      }

      const unsigned long actual = dwl1_servant->num_liveliness_lost_callbacks() +
        dwl2_servant->num_liveliness_lost_callbacks() +
        dwl3_servant->num_liveliness_lost_callbacks() +
        dwl4_servant->num_liveliness_lost_callbacks();

      if (liveliness_lost_test
          && static_cast<int>(actual) != num_liveliness_lost_callbacks)
      {
        cerr << "ERROR: did not receive expected liveliness lost callbacks. "
          << actual << "/" <<
          num_liveliness_lost_callbacks << endl;
        status = 1;
      }

      ACE_OS::sleep(1);
      ACE_DEBUG((LM_INFO, "publisher deleting entities\n"));
      participant->delete_contained_entities();
      dpf->delete_participant(participant.in ());
      participant2->delete_contained_entities();
      dpf->delete_participant(participant2.in ());
      TheServiceParticipant->shutdown ();
  }
  catch (CORBA::Exception& e)
    {
      cerr << "PUB: Exception caught in main.cpp:" << endl
         << e << endl;
      exit(1);
    }

  return status;
}
