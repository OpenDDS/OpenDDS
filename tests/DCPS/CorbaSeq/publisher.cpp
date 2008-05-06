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
#include <ace/streams.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

using namespace Messenger;

const OpenDDS::DCPS::TransportIdType TCP_IMPL_ID = 1;

int main (int argc, char *argv[]) {
  try {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    DDS::DomainParticipant_var participant =
      dpf->create_participant(411,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil());
    if (CORBA::is_nil (participant.in ())) {
      cerr << "create_participant failed." << endl;
      return 1;
    }

    MessageTypeSupportImpl* servant = new MessageTypeSupportImpl;

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

    OpenDDS::DCPS::TransportImpl_rch tcp_impl =
      TheTransportFactory->create_transport_impl (TCP_IMPL_ID, 
                                                  ::OpenDDS::DCPS::AUTO_CONFIG);

    DDS::Publisher_var pub =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                    DDS::PublisherListener::_nil());
    if (CORBA::is_nil (pub.in ())) {
      cerr << "create_publisher failed." << endl;
      exit(1);
    }

    // Attach the publisher to the transport.
    OpenDDS::DCPS::PublisherImpl* pub_impl =
      dynamic_cast< OpenDDS::DCPS::PublisherImpl*>(pub.in ());
    if (0 == pub_impl) {
      cerr << "Failed to obtain publisher servant" << endl;
      exit(1);
    }

    OpenDDS::DCPS::AttachStatus status = pub_impl->attach_transport(tcp_impl.in());
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

    // Create the datawriter
    DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos (dw_qos);
    DDS::DataWriter_var dw =
      pub->create_datawriter(topic.in (),
                             dw_qos,
                             DDS::DataWriterListener::_nil());
    if (CORBA::is_nil (dw.in ())) {
      cerr << "create_datawriter failed." << endl;
      exit(1);
    }
    Writer* writer = new Writer(dw.in());

    writer->start ();
    while ( !writer->is_finished()) {
      ACE_Time_Value small(0,250000);
      ACE_OS::sleep (small);
    }

    // Cleanup
    writer->end ();
    delete writer;
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in ());
    TheTransportFactory->release();
    TheServiceParticipant->shutdown ();
  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in main.cpp:" << endl
         << e << endl;
    exit(1);
  }

  return 0;
}
