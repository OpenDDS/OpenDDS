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
#include "DataWriterListener.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/transport/framework/TransportDebug.h>
#include <ace/streams.h>
#include <ace/Get_Opt.h>
#include <string>

#include "dds/DCPS/StaticIncludes.h"

const char* pub_ready_filename    = "publisher_ready.txt";
const char* pub_finished_filename = "publisher_finished.txt";
const char* sub_ready_filename    = "subscriber_ready.txt";
const char* sub_finished_filename = "subscriber_finished.txt";

int num_writes_before_crash = 0;
int num_writes = 10;
int write_delay_ms = 1000;
int expected_lost_pub_notification = 0;
int actual_lost_pub_notification = 0;
int expected_deleted_connections = 1;
int num_deleted_connections = 0;

/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT ("va:n:i:l:d:"));
  int c;

  while ((c = get_opts ()) != -1)
    switch (c)
      {
      case 'v':
        TURN_ON_VERBOSE_DEBUG;
        break;
      case 'a':
        num_writes_before_crash = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'n':
        num_writes = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'i':
        write_delay_ms = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'l':
        expected_lost_pub_notification = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'd':
        expected_deleted_connections = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case '?':
      default:
        ACE_ERROR_RETURN ((LM_ERROR,
                           "usage:  %s "
                           "-a <num_reads_before_crash> "
                           "-n <num_writers> "
                           "-i <write_delay_ms> "
                           "-l <expected_lost_pub_notification> "
                           "-d <expected_deleted_connections> "
                           "-v "
                           "\n",
                           argv [0]),
                          -1);
      }
  // Indicates successful parsing of the command line
  return 0;
}


int ACE_TMAIN (int argc, ACE_TCHAR *argv[]) {
  try {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    if( parse_args(argc, argv) != 0)
      return 1;

    ACE_DEBUG((LM_DEBUG, "(%P|%t) publisher.cpp main()\n"));

    DDS::DomainParticipant_var participant =
      dpf->create_participant(411,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (participant.in ())) {
      cerr << "create_participant failed." << endl;
      return 1;
    }

    Messenger::MessageTypeSupportImpl* servant = new Messenger::MessageTypeSupportImpl();
    OpenDDS::DCPS::LocalObject_var safe_servant = servant;

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

    // activate the listener
    DDS::DataWriterListener_var listener (new DataWriterListenerImpl);
    if (CORBA::is_nil (listener.in ())) {
      cerr << "listener is nil." << endl;
      exit(1);
    }

    // Create the datawriter
    DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos (dw_qos);
    // Make it KEEP_ALL history so we can verify the received
    // data without dropping.
    dw_qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;
    dw_qos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
    dw_qos.resource_limits.max_samples_per_instance = num_writes;

    DDS::DataWriter_var dw =
      pub->create_datawriter(topic.in (),
                             dw_qos,
                             listener.in (),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (dw.in ())) {
      cerr << "create_datawriter failed." << endl;
      exit(1);
    }
    Writer* writer = new Writer(dw.in());

    // Indicate that the publisher is ready
    FILE* writers_ready = ACE_OS::fopen (pub_ready_filename, ACE_TEXT ("w"));
    if (writers_ready == 0) {
      cerr << "ERROR Unable to create publisher ready file" << endl;
      exit(1);
    }
    ACE_OS::fclose(writers_ready);

    // Wait for the subscriber to be ready.
    FILE* readers_ready = 0;
    do {
      ACE_Time_Value small_time(0,250000);
      ACE_OS::sleep (small_time);
      readers_ready = ACE_OS::fopen (sub_ready_filename, ACE_TEXT ("r"));
    } while (0 == readers_ready);
    ACE_OS::fclose(readers_ready);

    // ensure the associations are fully established before writing.
    ACE_OS::sleep(3);
    writer->start ();
    while ( !writer->is_finished()) {
      ACE_Time_Value small_time(0,250000);
      ACE_OS::sleep (small_time);
    }

    // Indicate that the publisher is done
    FILE* writers_completed = ACE_OS::fopen (pub_finished_filename, ACE_TEXT ("w"));
    if (writers_completed == 0) {
      cerr << "ERROR Unable to i publisher completed file" << endl;
    } else {
      ACE_OS::fprintf (writers_completed, "%d\n",
                       writer->get_timeout_writes());
    }
    ACE_OS::fclose (writers_completed);

    // Wait for the subscriber to finish.
    FILE* readers_completed = 0;
    do {
      ACE_Time_Value small_time(0,250000);
      ACE_OS::sleep (small_time);
      readers_completed = ACE_OS::fopen (sub_finished_filename, ACE_TEXT ("r"));
    } while (0 == readers_completed);
    ACE_OS::fclose(readers_completed);

    writer->end ();
    delete writer;

    // Sleep a while before shutdown to avoid the problem of repository
    // crashes when it handles both remove_association from subscriber
    // and publisher at the same time.
    // Cleanup
    //ACE_OS::sleep (2);

    participant->delete_contained_entities();
    dpf->delete_participant(participant.in ());
    TheServiceParticipant->shutdown ();
  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in main.cpp:" << endl
         << e << endl;
    exit(1);
  }

  if (actual_lost_pub_notification != expected_lost_pub_notification)
  {
    ACE_ERROR ((LM_ERROR, "(%P|%t) ERROR: on_publication_lost called %d times "
      "and expected %d times\n", actual_lost_pub_notification,
      expected_lost_pub_notification));
    return 1;
  }

  if (num_deleted_connections != expected_deleted_connections)
  {
    ACE_ERROR ((LM_ERROR, "(%P|%t) ERROR: on_connection_deleted called %d times "
      "and expected %d times\n", num_deleted_connections,
      expected_deleted_connections));
    return 1;
  }

  return 0;
}
