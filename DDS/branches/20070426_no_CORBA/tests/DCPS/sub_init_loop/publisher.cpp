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

#include "MessageTypeSupportImpl.h"
#include "Writer.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>
#include <dds/DCPS/transport/framework/TransportDebug.h>

#include <ace/streams.h>
#include <ace/Get_Opt.h>
#include "ace/OS_NS_sys_stat.h"

const TAO::DCPS::TransportIdType TCP_IMPL_ID = 1;

const char* pub_ready_filename    = "publisher_ready.txt";
const char* pub_finished_filename = "publisher_finished.txt";
const char* sub_ready_filename    = "subscriber_ready.txt";
const char* sub_finished_filename = "subscriber_finished.txt";

bool verbose = false;
int write_delay_ms = 1000;

/// parse the command line arguments
int parse_args (int argc, char *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, "vi:");
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
  // Indicates sucessful parsing of the command line
  return 0;
}


int main (int argc, char *argv[]) {
  try {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    if( parse_args(argc, argv) != 0)
      return 1;

    DDS::DomainParticipant_var participant =
      dpf->create_participant(411,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil());
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
                                 DDS::TopicListener::_nil());
    if (CORBA::is_nil (topic.in ())) {
      cerr << "create_topic failed." << endl;
      exit(1);
    }

    TAO::DCPS::TransportImpl_rch tcp_impl =
      TheTransportFactory->create_transport_impl (TCP_IMPL_ID,
                                                  ::TAO::DCPS::AUTO_CONFIG);

    DDS::Publisher_var pub =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                    DDS::PublisherListener::_nil());
    if (CORBA::is_nil (pub.in ())) {
      cerr << "create_publisher failed." << endl;
      exit(1);
    }

    // Attach the publisher to the transport.
    TAO::DCPS::PublisherImpl* pub_impl =
      TAO::DCPS::reference_to_servant<TAO::DCPS::PublisherImpl> (pub.in ());
    if (0 == pub_impl) {
      cerr << "Failed to obtain publisher servant" << endl;
      exit(1);
    }

    TAO::DCPS::AttachStatus status = pub_impl->attach_transport(tcp_impl.in());
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

    // Create the datawriter
    DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos (dw_qos);
    // Make it KEEP_ALL history so we can verify the received
    // data without dropping.
    dw_qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;
    dw_qos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
    //dw_qos.resource_limits.max_samples_per_instance = num_writes;

    DDS::DataWriter_var dw =
      pub->create_datawriter(topic.in (),
                             dw_qos,
           DDS::DataWriterListener::_nil());
    if (CORBA::is_nil (dw.in ())) {
      cerr << "create_datawriter failed." << endl;
      exit(1);
    }
    Writer* writer = new Writer (dw.in(), sub_finished_filename
         , verbose);

    // Indicate that the publisher is ready
    FILE* writers_ready = ACE_OS::fopen (pub_ready_filename, "w");
    if (writers_ready == 0) {
      cerr << "ERROR Unable to create publisher ready file" << endl;
      exit(1);
    }
    ACE_OS::fclose(writers_ready);


    // Wait for the subscriber to be ready.
    ACE_stat stats;
    while (ACE_OS::stat (sub_ready_filename, &stats) == -1)
      {
        ACE_Time_Value small(0,250000);
  ACE_OS::sleep (small);
      }

    // ensure the associations are fully established before writing.
    ACE_OS::sleep(3);
    writer->start ();

    writer->wait ();
    delete writer;

    // Sleep a while before shutdown to avoid the problem of repository
    // crashes when it handles both remove_association from subscriber
    // and publisher at the same time.
    // Cleanup
    //ACE_OS::sleep (2);

    participant->delete_contained_entities();
    dpf->delete_participant(participant.in ());
    TheTransportFactory->release();
    TheServiceParticipant->shutdown ();
  }
  catch (CORBA::Exception& e)
    {
      cerr << "Exception caught in main.cpp:" << endl
     << e << endl;
      exit(1);
    }

  return 0;
}
