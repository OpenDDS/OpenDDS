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
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/transport/framework/TransportDebug.h>
#include "dds/DCPS/StaticIncludes.h"

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include <ace/Get_Opt.h>
#include "ace/OS_NS_sys_stat.h"
#include "ace/OS_NS_unistd.h"

using namespace std;

const char* pub_ready_filename    = "publisher_ready.txt";
const char* pub_finished_filename = "publisher_finished.txt";
const char* sub_ready_filename    = "subscriber_ready.txt";
const char* sub_finished_filename = "subscriber_finished.txt";

bool verbose = false;
int write_delay_ms = 1000;

/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("vi:"));
  int c;

  while ((c = get_opts ()) != -1)
    switch (c)
      {
      case 'v':
        verbose = true;
        break;
      case 'i':
        write_delay_ms = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case '?':
      default:
        ACE_ERROR_RETURN ((LM_ERROR,
                           "usage:  %s "
                           "-i <write_delay_ms> "
                           "-v <verbose>"
                           "\n",
                           argv [0]),
                          -1);
      }
  // Indicates successful parsing of the command line
  return 0;
}


int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);

      if( parse_args(argc, argv) != 0)
        return 1;

      DDS::DomainParticipant_var participant =
        dpf->create_participant(411,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1;
      }

      Messenger::MessageTypeSupportImpl* servant =
        new Messenger::MessageTypeSupportImpl;

      if (DDS::RETCODE_OK != servant->register_type(participant.in (), "")) {
        cerr << "register_type failed." << endl;
        exit(1);
      }

      CORBA::String_var type_name = servant->get_type_name ();

      DDS::TopicQos topic_qos;
      participant->get_default_topic_qos(topic_qos);
      DDS::Topic_var topic =
        participant->create_topic ("Movie Discussion List",
                                   type_name.in (),
                                   topic_qos,
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

      // Create the datawriter
      DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);
      // Make it KEEP_ALL history so we can verify the received
      // data without dropping.
      // dw_qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      dw_qos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
      //dw_qos.resource_limits.max_samples_per_instance = num_writes;

      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in (),
                               dw_qos,
                               DDS::DataWriterListener::_nil(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }

      Writer writer (dw.in(), sub_finished_filename, verbose, write_delay_ms);

      // Indicate that the publisher is ready
      FILE* writers_ready = ACE_OS::fopen (pub_ready_filename, ACE_TEXT("w"));
      if (writers_ready == 0) {
        cerr << "ERROR Unable to create publisher ready file" << endl;
        exit(1);
      }
      ACE_OS::fclose(writers_ready);


      // Wait for the subscriber to be ready.
      ACE_stat stats;
      while (ACE_OS::stat (sub_ready_filename, &stats) == -1)
        {
          ACE_Time_Value small_time(0,250000);
          ACE_OS::sleep (small_time);
        }

      writer.start ();
      writer.wait ();

      participant->delete_contained_entities();
      dpf->delete_participant(participant);
      TheServiceParticipant->shutdown();
    }
  catch (CORBA::Exception& e)
    {
      cerr << "Exception caught in main.cpp:" << endl
           << e << endl;
      exit(1);
    }

  return 0;
}
