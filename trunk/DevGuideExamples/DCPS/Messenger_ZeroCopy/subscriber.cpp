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

#include <ace/streams.h>
#include "ace/Get_Opt.h"

OpenDDS::DCPS::TransportIdType transport_impl_id = 1;

int
parse_args (int argc, char *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, "t:");
  int c;

  while ((c = get_opts ()) != -1)
  {
    switch (c)
    {
    case 't':
      if (ACE_OS::strcmp (get_opts.opt_arg (), "udp") == 0) {
        transport_impl_id = 2;
      }
      else if (ACE_OS::strcmp (get_opts.opt_arg (), "mcast") == 0) {
        transport_impl_id = 3;
      }
      // test with DEFAULT_SIMPLE_TCP_ID.
      else if (ACE_OS::strcmp (get_opts.opt_arg (), "default_tcp") == 0) {
        transport_impl_id = OpenDDS::DCPS::DEFAULT_SIMPLE_TCP_ID;
      }
      // test with DEFAULT_SIMPLE_UDP_ID.
      else if (ACE_OS::strcmp (get_opts.opt_arg (), "default_udp") == 0) {
        transport_impl_id = OpenDDS::DCPS::DEFAULT_SIMPLE_UDP_ID;
      }
      else if (ACE_OS::strcmp (get_opts.opt_arg (), "default_mcast_sub") == 0) {
        transport_impl_id = OpenDDS::DCPS::DEFAULT_SIMPLE_MCAST_SUB_ID;
      }
      break;
    case '?':
    default:
      ACE_ERROR_RETURN ((LM_ERROR,
        "usage:  %s "
        "-t <tcp/udp/default> "
        "\n",
        argv [0]),
        -1);
    }
  }
  // Indicates sucessful parsing of the command line
  return 0;
}


int main (int argc, char *argv[])
{
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

      if (parse_args (argc, argv) == -1) {
        return -1;
      }

      Messenger::MessageTypeSupport_var mts = new Messenger::MessageTypeSupportImpl();

      if (DDS::RETCODE_OK != mts->register_type(participant.in (), "")) {
          cerr << "Failed to register the MessageTypeTypeSupport." << endl;
          exit(1);
        }

      CORBA::String_var type_name = mts->get_type_name ();

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
      OpenDDS::DCPS::TransportImpl_rch tcp_impl = 
        TheTransportFactory->create_transport_impl (transport_impl_id, 
                                                    ::OpenDDS::DCPS::AUTO_CONFIG);

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
      OpenDDS::DCPS::SubscriberImpl* sub_impl =
        ::OpenDDS::DCPS::reference_to_servant< OpenDDS::DCPS::SubscriberImpl,
                                           DDS::Subscriber_ptr> (sub.in ());
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
      DataReaderListenerImpl        listener_servant;
      DDS::DataReaderListener_var listener =
        ::OpenDDS::DCPS::servant_to_reference(&listener_servant);

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
