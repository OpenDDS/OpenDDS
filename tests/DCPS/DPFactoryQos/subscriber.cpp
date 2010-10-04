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
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

#include <ace/streams.h>
#include "ace/Get_Opt.h"

using namespace Messenger;

OpenDDS::DCPS::TransportIdType transport_impl_id = 1;

int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf;
      DDS::DomainParticipant_var participant;

      dpf = TheParticipantFactoryWithArgs(argc, argv);

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

      participant = dpf->create_participant(411,
                                            PARTICIPANT_QOS_DEFAULT,
                                            DDS::DomainParticipantListener::_nil(),
                                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1 ;
      }

      if (participant->enable () != ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        cerr << "DomainParticipant can not be enabled because factory autoenable is off." << endl;
        return 1;
      }

      MessageTypeSupport_var mts = new MessageTypeSupportImpl();

      if (DDS::RETCODE_OK != mts->register_type(participant.in (), "")) {
          cerr << "Failed to register the MessageTypeTypeSupport." << endl;
          exit(1);
        }

      CORBA::String_var type_name = mts->get_type_name ();

      DDS::Topic_var topic = participant->create_topic("Movie Discussion List",
                                                       type_name.in (),
                                                       TOPIC_QOS_DEFAULT,
                                                       DDS::TopicListener::_nil(),
                                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ())) {
        cerr << "Failed to create_topic." << endl;
        exit(1);
      }

      if (topic->enable () != ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        cerr << "Topic can not be enabled because DomainParticipant is not enabled." << endl;
        return 1;
      }

      // Initialize the transport
      OpenDDS::DCPS::TransportImpl_rch transport_impl =
        TheTransportFactory->create_transport_impl (transport_impl_id,
                                                    ::OpenDDS::DCPS::AUTO_CONFIG);

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

      if (sub->enable () != ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        cerr << "Publisher can not be enabled because DomainParticipant is not enabled." << endl;
        return 1;
      }

      // Attach the subscriber to the transport.
      OpenDDS::DCPS::SubscriberImpl* sub_impl =
        dynamic_cast<OpenDDS::DCPS::SubscriberImpl*> (sub.in ());
      if (0 == sub_impl) {
        cerr << "Failed to obtain subscriber servant\n" << endl;
        exit(1);
      }

      OpenDDS::DCPS::AttachStatus status = sub_impl->attach_transport(transport_impl.in());
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
      DDS::DataReaderListener_var listener = new DataReaderListenerImpl;
      DataReaderListenerImpl &listener_servant =
        *dynamic_cast<DataReaderListenerImpl*>(listener.in());

      if (CORBA::is_nil (listener.in ())) {
        cerr << "listener is nil." << endl;
        exit(1);
      }

      // Create the Datareaders
      DDS::DataReader_var dr = sub->create_datareader(topic.in (),
                                                      DATAREADER_QOS_DEFAULT,
                                                      listener.in (),
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dr.in ())) {
        cerr << "create_datareader failed." << endl;
        exit(1);
      }

      if (dr->enable () != ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        cerr << "DataReader can not be enabled because Subscriber is not enabled." << endl;
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
        || sub->enable () != ::DDS::RETCODE_OK)
      {
        cerr << "Failed to enable factory." << endl;
        return 1;
      }

      // The datareader is not enabled so it will not able to
      // communicate with datawriter.
      int i = 0;
      while (i < 5 && listener_servant.num_reads() == 0)
      {
        ACE_OS::sleep (1);
        ++i;
      }

      if (listener_servant.num_reads() > 0)
      {
        cerr << "Should not receive any samples since datareader is not enabled." << endl;
        return 1;
      }

      if (dr->enable () != ::DDS::RETCODE_OK)
      {
        cerr << "Failed to enable DataReader." << endl;
        return 1;
      }

      int expected = 10;

      while ( listener_servant.num_reads() < expected ) {
        ACE_OS::sleep (1);
      }

      if (!CORBA::is_nil (participant.in ())) {
        participant->delete_contained_entities();
      }
      if (!CORBA::is_nil (dpf.in ())) {
        dpf->delete_participant(participant.in ());
      }
      ACE_OS::sleep(2);

      TheTransportFactory->release();
      TheServiceParticipant->shutdown ();
    }
  catch (CORBA::Exception& e)
    {
      cerr << "SUB: Exception caught in main ():" << endl << e << endl;
      return 1;
    }

  return 0;
}
