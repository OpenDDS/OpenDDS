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
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration_rch.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>
#include <ace/streams.h>

const TAO::DCPS::TransportFactory::IdType TCP_TYPE_ID = 1;
const TAO::DCPS::TransportFactory::IdType TCP_IMPL_ID = 1;
const char* pub_ready_filename    = "publisher_ready.txt";
const char* pub_finished_filename = "publisher_finished.txt";
const char* sub_ready_filename    = "subscriber_ready.txt";
const char* sub_finished_filename = "subscriber_finished.txt";


int main (int argc, char *argv[])
{
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var participant;

  try {
    dpf = TheParticipantFactoryWithArgs(argc, argv);
    participant =
      dpf->create_participant(411,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil());
    if (CORBA::is_nil (participant.in ())) {
      cerr << "create_participant failed." << endl;
      return 1 ;
    }

    MessageTypeSupportImpl* mts_servant = new MessageTypeSupportImpl();
    if (DDS::RETCODE_OK != mts_servant->register_type(participant.in (),
                                                      "Message")) {
      cerr << "Failed to register the MessageTypeTypeSupport." << endl;
      exit(1);
    }

    DDS::TopicQos topic_qos;
    participant->get_default_topic_qos(topic_qos);
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                "Message",
                                topic_qos,
                                DDS::TopicListener::_nil());
    if (CORBA::is_nil (topic.in ())) {
      cerr << "Failed to create_topic." << endl;
      exit(1);
    }

    // Initialize the transport
    TheTransportFactory->register_type(TCP_TYPE_ID,
                                       new TAO::DCPS::SimpleTcpFactory());
    TAO::DCPS::SimpleTcpConfiguration_rch reader_config =
      new TAO::DCPS::SimpleTcpConfiguration();
    TAO::DCPS::TransportImpl_rch tcp_impl =
      TheTransportFactory->create(TCP_IMPL_ID, TCP_TYPE_ID);
    if (tcp_impl->configure(reader_config.in()) != 0) {
      cerr << "Failed to init_reader_tranport." << endl;
      exit(1);
    }

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
      reference_to_servant< TAO::DCPS::SubscriberImpl,
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
    DataReaderListenerImpl        listener_servant;
    PortableServer::POA_var poa = TheServiceParticipant->the_poa ();
    CORBA::Object_var obj = poa->servant_to_reference(&listener_servant);
    DDS::DataReaderListener_var listener =
      DDS::DataReaderListener::_narrow (obj.in ());
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
      cerr << "ERROR Unable to create subscriber completed file." << endl;
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

    int expected = 10;
    FILE* writers_completed = 0;
    int timeout_writes = 0;
    while ( listener_servant.num_reads() < expected) {
      // Get the number of the timed out writes from publisher so we
      // can re-calculate the number of expected messages. Otherwise,
      // the blocking timeout test will never exit from this loop.
      if (writers_completed == 0) {
        writers_completed = ACE_OS::fopen (pub_finished_filename, "r");
        if (writers_completed != 0) {
          //writers_completed = ACE_OS::fopen (pub_finished_filename, "r");
          fscanf (writers_completed, "%d\n", &timeout_writes);
          expected -= timeout_writes;
          cout << "timed out writes " << timeout_writes << ", we expect "
               << expected << endl;
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

    // Wait for the publisher to finish
    while (writers_completed == 0) {
      ACE_Time_Value small(0,250000);
      ACE_OS::sleep (small);
      writers_completed = ACE_OS::fopen (pub_finished_filename, "r");
    }
    ACE_OS::fclose(writers_completed);
  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in main ():" << endl << e << endl;
    exit(1);
  }

  // Cleanup
  try {
    if (!CORBA::is_nil (participant.in ())) {
      participant->delete_contained_entities();
    }
    if (!CORBA::is_nil (dpf.in ())) {
      dpf->delete_participant(participant.in ());
    }
  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in cleanup." << endl << e << endl;
    exit(1);
  }
  TheTransportFactory->release();
  TheServiceParticipant->shutdown ();
  return 0;
}
