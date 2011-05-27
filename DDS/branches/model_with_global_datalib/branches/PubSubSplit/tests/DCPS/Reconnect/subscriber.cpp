// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "DataReaderListener.h"
#include "MessageTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Subscription/SubscriberImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>
#include <dds/DCPS/transport/framework/TransportDebug.h>

#include <ace/streams.h>
#include <ace/Get_Opt.h>

const TAO::DCPS::TransportIdType TCP_IMPL_ID = 1;
const char* pub_ready_filename    = "publisher_ready.txt";
const char* pub_finished_filename = "publisher_finished.txt";
const char* sub_ready_filename    = "subscriber_ready.txt";
const char* sub_finished_filename = "subscriber_finished.txt";

int num_expected_reads = 10;
int num_reads_before_crash = 0;
int num_reads_deviation = 0;
int read_delay_ms = 0;
int expected_lost_sub_notification = 0;
int actual_lost_sub_notification = 0;
int end_with_publisher = 0;

/// parse the command line arguments
int parse_args (int argc, char *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, "vn:a:r:i:l:e:");
  int c;

  while ((c = get_opts ()) != -1)
    switch (c)
      {
      case 'v':
        TURN_ON_VERBOSE_DEBUG;
        break;
      case 'n':
        num_expected_reads = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'a':
        num_reads_before_crash = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'r':
        num_reads_deviation = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'i':
        read_delay_ms = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'l':
        expected_lost_sub_notification = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'e':
        end_with_publisher = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case '?':
      default:
        ACE_ERROR_RETURN ((LM_ERROR,
                           "usage:  %s "
                           "-n <num_expected_reads> "
                           "-a <num_reads_before_crash> "
                           "-d <num_reads_deviation> "
                           "-i <read_delay_ms> "
                           "-l <expected_lost_sub_notification> "
                           "-e <end_with_publisher> "
                           "-v "
                           "\n",
                           argv [0]),
                          -1);
      }
  // Indicates sucessful parsing of the command line
  return 0;
}


int main (int argc, char *argv[])
{
  try {
    DDS::DomainParticipantFactory_var dpf;
    DDS::DomainParticipant_var participant;

    dpf = TheParticipantFactoryWithArgs(argc, argv);
    if( parse_args(argc, argv) != 0)
      return 1;

    participant =
      dpf->create_participant(411,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil());
    if (CORBA::is_nil (participant.in ())) {
      cerr << "create_participant failed." << endl;
      return 1 ;
    }

    MessageTypeSupportImpl* mts_servant = new MessageTypeSupportImpl();
    PortableServer::ServantBase_var safe_servant = mts_servant;

    if (DDS::RETCODE_OK != mts_servant->register_type(participant.in (),
                                                      "")) {
      cerr << "Failed to register the MessageTypeTypeSupport." << endl;
      exit(1);
    }

    CORBA::String_var type_name = mts_servant->get_type_name ();

    DDS::TopicQos topic_qos;
    participant->get_default_topic_qos(topic_qos);
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                type_name.in (),
                                topic_qos,
                                DDS::TopicListener::_nil());
    if (CORBA::is_nil (topic.in ())) {
      cerr << "Failed to create_topic." << endl;
      exit(1);
    }

    // Initialize the transport
    TAO::DCPS::TransportImpl_rch tcp_impl =
      TheTransportFactory->create_transport_impl (TCP_IMPL_ID, ::TAO::DCPS::AUTO_CONFIG);

    // Create the subscriber and attach to the corresponding
    // transport.
    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     DDS::SubscriberListener::_nil());
    if (CORBA::is_nil (sub.in ())) {
      cerr << "Failed to create_subscriber." << endl;
      exit(1);
    }

    // Attach the subscriber to the transport.
    TAO::DCPS::SubscriberImpl* sub_impl =
      ::TAO::DCPS::reference_to_servant< TAO::DCPS::SubscriberImpl,
                                         DDS::Subscriber_ptr> (sub.in ());
    if (0 == sub_impl) {
      cerr << "Failed to obtain subscriber servant\n" << endl;
      exit(1);
    }

    TAO::DCPS::AttachStatus status = sub_impl->attach_transport(tcp_impl.in());
    if (status != TAO::DCPS::ATTACH_OK) {
      std::string status_str;
      switch (status) {
        case TAO::DCPS::ATTACH_BAD_TRANSPORT:
          status_str = "ATTACH_BAD_TRANSPORT";
          break;
        case TAO::DCPS::ATTACH_ERROR:
          status_str = "ATTACH_ERROR";
          break;
        case TAO::DCPS::ATTACH_INCOMPATIBLE_QOS:
          status_str = "ATTACH_INCOMPATIBLE_QOS";
          break;
        default:
          status_str = "Unknown Status";
          break;
      }
      cerr << "Failed to attach to the transport. Status == "
           << status_str.c_str() << endl;
      exit(1);
    }

    // activate the listener
    DataReaderListenerImpl listener_servant;
    DDS::DataReaderListener_var listener =
      ::TAO::DCPS::servant_to_reference (&listener_servant);
    if (CORBA::is_nil (listener.in ())) {
      cerr << "listener is nil." << endl;
      exit(1);
    }

    // Create the Datareaders
    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos (dr_qos);
    DDS::DataReader_var dr = sub->create_datareader(topic.in (),
                                                    dr_qos,
                                                    listener.in ());
    if (CORBA::is_nil (dr.in ())) {
      cerr << "create_datareader failed." << endl;
      exit(1);
    }

    // Indicate that the subscriber is ready
    FILE* readers_ready = ACE_OS::fopen (sub_ready_filename, "w");
    if (readers_ready == 0) {
      cerr << "ERROR Unable to create subscriber ready file." << endl;
      exit(1);
    }
    ACE_OS::fclose(readers_ready);

    // Wait for the publisher to be ready
    FILE* writers_ready = 0;
    do {
      ACE_Time_Value small(0,250000);
      ACE_OS::sleep (small);
      writers_ready = ACE_OS::fopen (pub_ready_filename, "r");
    } while (0 == writers_ready);
    ACE_OS::fclose(writers_ready);

    // Since the publisher continue sending while the subscriber crashes,
    // some messages may be lost, we lower the num_expected_reads by 2.
    num_expected_reads -= num_reads_deviation;

    FILE* writers_completed = 0;
    int timeout_writes = 0;
    while ( listener_servant.num_reads() < num_expected_reads) {
      // Get the number of the timed out writes from publisher so we
      // can re-calculate the number of expected messages. Otherwise,
      // the blocking timeout test will never exit from this loop.
      if (writers_completed == 0) {
        writers_completed = ACE_OS::fopen (pub_finished_filename, "r");
        if (writers_completed != 0) {
          if (end_with_publisher)
          {
            // Since we are in the "bp_timeout" test case that publisher
            // close connection when backpressure last longer than
            // max_output_pause_period, the publisher ends as it finishes
            // sending. As the subscriber sees the publisher is done, it
            // changes the read_delay_ms to 0 so it can read all received
            // messages and them announce it completed.

            int old_read_delay_ms = read_delay_ms;
            read_delay_ms = 0;
            // Give time to finish reading.
            ACE_OS::sleep (old_read_delay_ms/1000 * 2);
            break;
          }

          //writers_completed = ACE_OS::fopen (pub_finished_filename, "r");
          fscanf (writers_completed, "%d\n", &timeout_writes);
          num_expected_reads -= timeout_writes;
          cout << "timed out writes " << timeout_writes << ", we expect "
               << num_expected_reads << endl;
        }
      }
      ACE_OS::sleep (1);
    }

    // Indicate that the subscriber is done
    FILE* readers_completed = ACE_OS::fopen (sub_finished_filename, "w");
    if (readers_completed == 0) {
      cerr << "ERROR Unable to create subscriber completed file." << endl;
      exit(1);
    }
    ACE_OS::fclose(readers_completed);

    // Wait for 5 seconds to (>passive_reconnect_duration) 
    // to give transport time to detect the connection lost due to 
    // backpressure timeout before shutdown the datareader.
    if (end_with_publisher)
      ACE_OS::sleep (5);

    if (!CORBA::is_nil (participant.in ())) {
      participant->delete_contained_entities();
    }
    if (!CORBA::is_nil (dpf.in ())) {
      dpf->delete_participant(participant.in ());
    }
    TheTransportFactory->release();
    TheServiceParticipant->shutdown ();

  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in main ():" << endl << e << endl;
    return 1;
  }

  if (actual_lost_sub_notification != expected_lost_sub_notification)
  {
    ACE_ERROR ((LM_ERROR, "(%P|%t)ERROR: on_subscription_lost called %d times "
      "and expected %d times\n", actual_lost_sub_notification,
      expected_lost_sub_notification));
    return 1;
  }

  return 0;
}
