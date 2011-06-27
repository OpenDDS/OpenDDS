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
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/tcp/TcpConfiguration.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include <ace/streams.h>
#include "ace/Get_Opt.h"

#include <memory>

using namespace Messenger;

OpenDDS::DCPS::TransportIdType transport_impl_id = 1;

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]){
  try
    {
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

      MessageTypeSupportImpl* servant = new MessageTypeSupportImpl();

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
                                   DDS::TopicListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ())) {
        cerr << "create_topic failed." << endl;
        exit(1);
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

      // Attach the publisher to the transport.
      OpenDDS::DCPS::PublisherImpl* const pub_impl =
        dynamic_cast<OpenDDS::DCPS::PublisherImpl*> (pub.in());
      if (pub_impl == 0) {
        cerr << "Failed to obtain publisher servant" << endl;
        exit(1);
      }

      OpenDDS::DCPS::AttachStatus const status =
        pub_impl->attach_transport(tcp_impl.in());
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

      DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.deadline.period.sec     = 4;
      dw_qos.deadline.period.nanosec = 0;

      // Create DataWriter with 4 second deadline period which
      // should be compatible with first DataReader which has 5
      // seconds deadline period and not with second DataReader
      // which has 3 seconds deadline period.
      DDS::DataWriter_var dw =
        pub->create_datawriter (topic.in (),
                                dw_qos,
                                DDS::DataWriterListener::_nil (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      int const max_attempts = 20000;
      int attempts = 1;
      {
        // Wait for both first DataReader connect and write messages.
        std::auto_ptr<Writer> writer (new Writer (dw.in ()));

        while (attempts != max_attempts)
        {
          ::DDS::InstanceHandleSeq handles;
          dw->get_matched_subscriptions(handles);
          if (handles.length() == 1)
            break;
          else
            ACE_OS::sleep(1);
          ++attempts;
        }

        if (attempts == max_attempts)
        {
          cerr << "ERROR: subscriptions failed to match." << endl;
          exit (1);
        }

        writer->start ();

        // Now set DataWriter deadline to be 6 seconds which is not
        // compatible with the existing DataReader. This QoS change
        // should not be applied and an ERROR should be returned to
        // set_qos().
        dw_qos.deadline.period.sec = 6;

        if (dw->set_qos (dw_qos) != ::DDS::RETCODE_ERROR)
        {
          cerr << "ERROR: DataWriter changed deadline period which should not compatible "
            << "with all existing DataReaders" << endl;

          exit (1);
        }

        writer->end ();
      }

      {
        // Wait for both second DataReader connect which changed deadline period
        // from 3 seconds to 5 seconds.
        std::auto_ptr<Writer> writer (new Writer (dw.in ()));
        attempts = 1;
        while (attempts != max_attempts)
        {
          ::DDS::InstanceHandleSeq handles;
          dw->get_matched_subscriptions(handles);
          if (handles.length() == 2)
            break;
          else
            ACE_OS::sleep(1);
          ++attempts;
        }

        if (attempts == max_attempts)
        {
          cerr << "ERROR: subscriptions failed to match." << endl;
          exit (1);
        }

        writer->start ();
        writer->end ();
      }

      {
        // Wait for subscriber exit.
        attempts = 1;
        while (attempts != max_attempts)
        {
          ::DDS::InstanceHandleSeq handles;

          dw->get_matched_subscriptions(handles);
          if (handles.length() == 0)
            break;
          else
            ACE_OS::sleep(1);

          ++ attempts;
        }

        if (attempts == max_attempts)
        {
          cerr << "ERROR: failed to wait for DataReader exit." << endl;
          exit (1);
        }
      }

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
