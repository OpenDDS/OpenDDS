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
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/tcp/TcpConfiguration.h>
#include <ace/OS_main.h>
#include <ace/streams.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

using namespace Messenger;

const OpenDDS::DCPS::TransportIdType TCP_IMPL_ID = 1;

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    DDS::DomainParticipantFactory_var dpf;
    DDS::DomainParticipant_var participant;

    dpf = TheParticipantFactoryWithArgs(argc, argv);
    participant =
      dpf->create_participant(411,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (participant.in ())) {
      cerr << "create_participant failed." << endl;
      return 1 ;
    }

    MessageTypeSupportImpl* mts_servant = new MessageTypeSupportImpl;

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
                                DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (topic.in ())) {
      cerr << "Failed to create_topic." << endl;
      exit(1);
    }

    // Initialize the transport
    OpenDDS::DCPS::TransportImpl_rch tcp_impl =
      TheTransportFactory->create_transport_impl (TCP_IMPL_ID, ::OpenDDS::DCPS::AUTO_CONFIG);

    // Create the subscriber and attach to the corresponding
    // transport.
    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     DDS::SubscriberListener::_nil(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (sub.in ())) {
      cerr << "Failed to create_subscriber." << endl;
      exit(1);
    }

    // Attach the subscriber to the transport.
    OpenDDS::DCPS::SubscriberImpl* sub_impl =
      dynamic_cast< OpenDDS::DCPS::SubscriberImpl* > (sub.in ());
    if (0 == sub_impl) {
      cerr << "Failed to obtain subscriber servant\n" << endl;
      exit(1);
    }

    OpenDDS::DCPS::AttachStatus status = sub_impl->attach_transport(tcp_impl.in());
    if (status != OpenDDS::DCPS::ATTACH_OK) {
      std::string status_str;
      switch (status) {
        case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
          status_str = "ATTACH_BAD_TRANSPORT";
          break;
        case OpenDDS::DCPS::ATTACH_ERROR:
          status_str = "ATTACH_ERROR";
          break;
        case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
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
    DDS::DataReaderListener_var listener (new DataReaderListenerImpl);
    if (CORBA::is_nil (listener.in ())) {
      cerr << "listener is nil." << endl;
      exit(1);
    }
    DataReaderListenerImpl* listener_servant =
      dynamic_cast<DataReaderListenerImpl*>(listener.in());

    // Create the Datareaders
    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos (dr_qos);
    DDS::DataReader_var dr
      = sub->create_datareader(topic.in (),
                               dr_qos,
                               listener.in (),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (dr.in ())) {
      cerr << "create_datareader failed." << endl;
      exit(1);
    }


    while ( ! listener_servant->received_all ()) {
      ACE_OS::sleep (1);
    }

    if (! listener_servant->passed ()) {
      cerr << "test failed - see errors." << endl;
      return 1;
    }

    if (!CORBA::is_nil (participant.in ())) {
      participant->delete_contained_entities();
    }
    if (!CORBA::is_nil (dpf.in ())) {
      dpf->delete_participant(participant.in ());
    }

    ::DDS::InstanceHandleSeq handles;
    while (1)
    {
      ACE_OS::sleep(1);
      dr->get_matched_publications(handles);
      if (handles.length() == 0)
        break;
    }

    ACE_OS::sleep(2);

    TheTransportFactory->release();
    TheServiceParticipant->shutdown ();

  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in main ():" << endl << e << endl;
    return 1;
  }

  return 0;
}
