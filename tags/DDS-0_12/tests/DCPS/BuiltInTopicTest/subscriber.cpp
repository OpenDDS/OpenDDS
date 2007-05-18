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
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>

#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"

#include <ace/streams.h>
#include "ace/Get_Opt.h"

TAO::DCPS::TransportIdType transport_impl_id = 1;

int main (int argc, char *argv[])
{
  ACE_DEBUG ((LM_DEBUG, "(%P|%t) subscriber main\n"));
  try
    {
      DDS::DomainParticipantFactory_var dpf;
      DDS::DomainParticipant_var participant;

      dpf = TheParticipantFactoryWithArgs(argc, argv);
      participant = dpf->create_participant(411,
                                            PARTICIPANT_QOS_DEFAULT,
                                            DDS::DomainParticipantListener::_nil());
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1 ;
      }

      MessageTypeSupportImpl* mts_servant = new MessageTypeSupportImpl();
      PortableServer::ServantBase_var safe_servant = mts_servant;

      if (DDS::RETCODE_OK != mts_servant->register_type(participant.in (), "Messenger")) {
          cerr << "Failed to register the MessageTypeTypeSupport." << endl;
          exit(1);
        }

      CORBA::String_var type_name = mts_servant->get_type_name ();

      DDS::TopicQos topic_qos;
      participant->get_default_topic_qos(topic_qos);
      DDS::Topic_var topic = participant->create_topic("Movie Discussion List",
                                                        type_name.in (),
                                                        topic_qos,
                                                        DDS::TopicListener::_nil());
      if (CORBA::is_nil (topic.in ())) {
        cerr << "Failed to create_topic." << endl;
        exit(1);
      }

      // Initialize the transport
      TAO::DCPS::TransportImpl_rch tcp_impl = 
        TheTransportFactory->create_transport_impl (transport_impl_id, 
                                                    ::TAO::DCPS::AUTO_CONFIG);

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
      DataReaderListenerImpl        listener_servant;
      DDS::DataReaderListener_var listener =
        ::TAO::DCPS::servant_to_reference(&listener_servant);

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

      int expected = 10;
      while ( listener_servant.num_reads() < expected) {
        ACE_OS::sleep (1);
      }

      if (!CORBA::is_nil (participant.in ())) {
        participant->delete_contained_entities();
      }
      if (!CORBA::is_nil (dpf.in ())) {
        dpf->delete_participant(participant.in ());
      }

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
