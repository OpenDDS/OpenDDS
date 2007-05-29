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
#include "ace/Get_Opt.h"

using namespace Messenger;

TAO::DCPS::TransportIdType transport_impl_id = 1;

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
        transport_impl_id = TAO::DCPS::DEFAULT_SIMPLE_TCP_ID;
      }
      // test with DEFAULT_SIMPLE_UDP_ID.
      else if (ACE_OS::strcmp (get_opts.opt_arg (), "default_udp") == 0) {
        transport_impl_id = TAO::DCPS::DEFAULT_SIMPLE_UDP_ID;
      }
      else if (ACE_OS::strcmp (get_opts.opt_arg (), "default_mcast_pub") == 0) {
        transport_impl_id = TAO::DCPS::DEFAULT_SIMPLE_MCAST_PUB_ID;
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

int main (int argc, char *argv[]) {
  try
    {
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

      if (parse_args (argc, argv) == -1) {
        return -1;
      }

      MessageTypeSupportImpl* servant = new MessageTypeSupportImpl();
      TAO::DCPS::LocalObject_var safe_servant = servant;

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
        TheTransportFactory->create_transport_impl (transport_impl_id,
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
  }
  catch (CORBA::Exception& e)
    {
       cerr << "PUB: Exception caught in main.cpp:" << endl
         << e << endl;
      exit(1);
    }

  return 0;
}
