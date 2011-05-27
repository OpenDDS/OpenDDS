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
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

#include <ace/streams.h>
#include "ace/Get_Opt.h"

using namespace Messenger;

OpenDDS::DCPS::TransportIdType transport_impl_id = 1;

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]) {
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);

      // Default DomainParticipantFactory qos is to auto enable.
      ::DDS::DomainParticipantFactoryQos fqos;
      if (dpf->get_qos (fqos) != ::DDS::RETCODE_OK)
      {
        cerr << "DomainParticipantFactory get_qos failed." << endl;
        return 1;
      }
      
      if (fqos.entity_factory.autoenable_created_entities == 0)
      { 
        cerr << "The DomainParticipantFactory defaults to autoenable upon entities creation." << endl;
        return 1;
      }
      
      // Now disable DomainParticipantFactory autoenable
      fqos.entity_factory.autoenable_created_entities = 0;
      if (dpf->set_qos (fqos) != ::DDS::RETCODE_OK)
      {
        cerr << "DomainParticipantFactory set_qos failed." << endl;
        return 1;
      }

      DDS::DomainParticipant_var participant =
        dpf->create_participant(411,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1;
      }

      if (participant->enable () != ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        cerr << "DomainParticipant can not be enabled because factory autoenable is off." << endl;
        return 1;
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

      if (topic->enable () != ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        cerr << "Topic can not be enabled because DomainParticipant is not enabled." << endl;
        return 1;
      }

      OpenDDS::DCPS::TransportImpl_rch tcp_impl =
        TheTransportFactory->create_transport_impl (transport_impl_id,
                                                    ::OpenDDS::DCPS::AUTO_CONFIG);

      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ())) {
        cerr << "create_publisher failed." << endl;
        exit(1);
      }

      if (pub->enable () != ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        cerr << "Publisher can not be enabled because DomainParticipant is not enabled." << endl;
        return 1;
      }

      // Attach the publisher to the transport.
      OpenDDS::DCPS::PublisherImpl* pub_impl =
        dynamic_cast<OpenDDS::DCPS::PublisherImpl*> (pub.in ());
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
      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in (),
                               DATAWRITER_QOS_DEFAULT,
                               DDS::DataWriterListener::_nil(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }

      if (dw->enable () != ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        cerr << "DataWriter can not be enabled because Publisher is not enabled." << endl;
        return 1;
      }

      // Now enable DomainParticipantFactory autoenable
      fqos.entity_factory.autoenable_created_entities = 1;
      if (dpf->set_qos (fqos) != ::DDS::RETCODE_OK)
      {
        cerr << "DomainParticipantFactory set_qos failed." << endl;
        return 1;
      }

      // Enable every entity from factory to it's entities and it should succeed.
      if (participant->enable () != ::DDS::RETCODE_OK 
        || topic->enable () != ::DDS::RETCODE_OK
        || pub->enable () != ::DDS::RETCODE_OK)
      {
        cerr << "Failed to enable factory." << endl;
        return 1;
      }

      Writer* writer = new Writer(dw.in());
      writer->start ();
      writer->wait ();
      delete writer;
      participant->delete_contained_entities();
      dpf->delete_participant(participant.in ());
      TheTransportFactory->release();
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
