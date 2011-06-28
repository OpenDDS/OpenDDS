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
#include <dds/DCPS/transport/tcp/TcpInst.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include <ace/streams.h>
#include "ace/Get_Opt.h"

using namespace Messenger;

OpenDDS::DCPS::TransportIdType transport_impl_id = 1;

int
parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("t:"));
  int c;

  while ((c = get_opts ()) != -1)
  {
    switch (c)
    {
    case 't':
      if (ACE_OS::strcmp (get_opts.opt_arg (), ACE_TEXT("udp")) == 0) {
        transport_impl_id = 2;
      }
      else if (ACE_OS::strcmp (get_opts.opt_arg (), ACE_TEXT("multicast")) == 0) {
        transport_impl_id = 3;
      }
      // test with DEFAULT_TCP_ID.
      else if (ACE_OS::strcmp (get_opts.opt_arg (), ACE_TEXT("default_tcp")) == 0) {
        transport_impl_id = OpenDDS::DCPS::DEFAULT_TCP_ID;
      }
      // test with DEFAULT_UDP_ID.
      else if (ACE_OS::strcmp (get_opts.opt_arg (), ACE_TEXT("default_udp")) == 0) {
        transport_impl_id = OpenDDS::DCPS::DEFAULT_UDP_ID;
      }
      else if (ACE_OS::strcmp (get_opts.opt_arg (), ACE_TEXT("default_multicast")) == 0) {
        transport_impl_id = OpenDDS::DCPS::DEFAULT_MULTICAST_ID;
      }
      break;
    case '?':
    default:
      ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("usage:  %s -t <tcp/udp/default> \n"),
        argv [0]),
        -1);
    }
  }
  // Indicates sucessful parsing of the command line
  return 0;
}

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]) {
  try
    {
      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("initialization starting.\n")
        ));
      }

      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);
      DDS::DomainParticipant_var participant =
        dpf->create_participant(411,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1;
      }

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("participant created.\n")
        ));
      }

      if (parse_args (argc, argv) == -1) {
        return -1;
      }

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("arguments extracted.\n")
        ));
      }

      MessageTypeSupportImpl* servant = new MessageTypeSupportImpl();

      if (DDS::RETCODE_OK != servant->register_type(participant.in (), "")) {
        cerr << "register_type failed." << endl;
        exit(1);
      }

      CORBA::String_var type_name = servant->get_type_name ();

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("type support registered.\n")
        ));
      }

      DDS::TopicQos topic_qos;
      participant->get_default_topic_qos(topic_qos);
      DDS::Topic_var topic =
        participant->create_topic ("Movie Discussion List",
                                   type_name.in (),
                                   topic_qos,
                                   DDS::TopicListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ())) {
        cerr << "create_topic failed." << endl;
        exit(1);
      }

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("topic created.\n")
        ));
      }

      OpenDDS::DCPS::TransportImpl_rch tcp_impl =
        TheTransportFactory->create_transport_impl (transport_impl_id,
                                                    ::OpenDDS::DCPS::AUTO_CONFIG);

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("transport created.\n")
        ));
      }

      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ())) {
        cerr << "create_publisher failed." << endl;
        exit(1);
      }

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("publisher created.\n")
        ));
      }

      // Attach the publisher to the transport.
      OpenDDS::DCPS::PublisherImpl* pub_impl =
        dynamic_cast<OpenDDS::DCPS::PublisherImpl*> (pub.in ());
      if (0 == pub_impl) {
        cerr << "Failed to obtain publisher servant" << endl;
        exit(1);
      }

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("servant extracted.\n")
        ));
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

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("transport attached.\n")
        ));
      }

      // Create the datawriter
      DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);
      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in (),
                               dw_qos,
                               DDS::DataWriterListener::_nil(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("datawriter created.\n")
        ));
      }

      size_t msg_cnt = 5;
      size_t sub_cnt = 2;
      Writer* writer = new Writer(dw.in(), msg_cnt, sub_cnt);

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("processing starting.\n")
        ));
      }

      writer->start ();
      while ( !writer->is_finished()) {
        ACE_Time_Value small_time(0,250000);
        ACE_OS::sleep (small_time);
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
