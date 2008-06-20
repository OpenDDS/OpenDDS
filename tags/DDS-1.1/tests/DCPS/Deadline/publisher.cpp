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
#include "DataWriterListenerImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
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
                                DDS::DomainParticipantListener::_nil());
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
                                   DDS::TopicListener::_nil());
      if (CORBA::is_nil (topic.in ())) {
        cerr << "create_topic failed." << endl;
        exit(1);
      }

      OpenDDS::DCPS::TransportImpl_rch tcp_impl =
        TheTransportFactory->create_transport_impl (transport_impl_id,
                                                    ::OpenDDS::DCPS::AUTO_CONFIG);

      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil());
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

      // ----------------------------------------------

      // Create the listener.
      DDS::DataWriterListener_var listener (new DataWriterListenerImpl);
      if (CORBA::is_nil (listener.in ()))
      {
        cerr << "ERROR: listener is nil." << endl;
        exit(1);
      }


      DDS::DataWriterQos dw_qos; // Good QoS.
      pub->get_default_datawriter_qos (dw_qos);

      // Set up a 4 second recurring deadline.
      static DDS::Duration_t const DEADLINE_PERIOD =
        {
          4,  // seconds
          0   // nanoseconds
        };

      assert (DEADLINE_PERIOD.sec > 1); // Requirement for the test.

      // Time to sleep waiting for deadline periods to expire
      long const NUM_EXPIRATIONS = 2;
      ACE_Time_Value const SLEEP_DURATION (
        OpenDDS::DCPS::duration_to_time_value (DEADLINE_PERIOD)
        * 2
        + ACE_Time_Value (1));

      dw_qos.deadline.period.sec     = DEADLINE_PERIOD.sec;
      dw_qos.deadline.period.nanosec = DEADLINE_PERIOD.nanosec;

      // First data writer will have a listener to test listener
      // callback on deadline expiration.
      DDS::DataWriter_var dw1 =
        pub->create_datawriter (topic.in (),
                                dw_qos,
                                listener.in ());

      // Second data writer will not have a listener to test proper
      // handling of a nil listener in the deadline handling code.
      DDS::DataWriter_var dw2 =
        pub->create_datawriter (topic.in (),
                                dw_qos,
                                DDS::DataWriterListener::_nil ());

      if (CORBA::is_nil (dw1.in ()) || CORBA::is_nil (dw2.in ()))
      {
        cerr << "ERROR: create_datawriter failed." << endl;
        exit(1);
      }

      // ----------------------------------------------

      // Wait for deadline periods to expire.
      ACE_OS::sleep (SLEEP_DURATION);

      DDS::OfferedDeadlineMissedStatus deadline_status1 =
        dw1->get_offered_deadline_missed_status();

      DDS::OfferedDeadlineMissedStatus deadline_status2 =
        dw2->get_offered_deadline_missed_status();

      if (deadline_status1.total_count != NUM_EXPIRATIONS
          || deadline_status2.total_count != NUM_EXPIRATIONS)
      {
        cerr << "ERROR: Expected number of missed offered "
             << "deadlines (" << NUM_EXPIRATIONS << ") " << "did " << endl
             << "       not occur ("
             << deadline_status1.total_count << " and/or "
             << deadline_status2.total_count << ")." << endl;

        exit (1);
      }

      if (deadline_status1.total_count_change != NUM_EXPIRATIONS
          || deadline_status2.total_count_change != NUM_EXPIRATIONS)
      {
        cerr << "ERROR: Incorrect missed offered "
             << "deadline count change" << endl
             << "       ("
             << deadline_status1.total_count_change
             << " and/or "
             << deadline_status2.total_count_change
             << " instead of " << NUM_EXPIRATIONS << ")."
             << endl;

        exit (1);
      }

      // Wait for another set of deadline periods to expire.
      ACE_OS::sleep (SLEEP_DURATION);

      deadline_status1 = dw1->get_offered_deadline_missed_status();
      deadline_status2 = dw2->get_offered_deadline_missed_status();

      if (deadline_status1.total_count != NUM_EXPIRATIONS * 2
          || deadline_status2.total_count != NUM_EXPIRATIONS * 2)
      {
        cerr << "ERROR: Another expected number of missed offered "
             << "deadlines (" << NUM_EXPIRATIONS * 2 << ")" << endl
             << "       did not occur ("
             << deadline_status1.total_count << " and/or "
             << deadline_status2.total_count << ")." << endl;

        exit (1);
      }

      if (deadline_status1.total_count_change != NUM_EXPIRATIONS
          || deadline_status2.total_count_change != NUM_EXPIRATIONS)
      {
        cerr << "ERROR: Incorrect missed offered "
             << "deadline count" << endl
             << "       change ("
             << deadline_status1.total_count_change
             << "and/or "
             << deadline_status2.total_count_change
             << " instead of " << NUM_EXPIRATIONS << ")." << endl;

        exit (1);
      }

      {
        // Just write with our first DataWriter since it has a listener.
        std::auto_ptr<Writer> writer (new Writer (dw1.in ()));

        int const max_attempts = 15;
        int attempts = 1; 
        while (attempts != max_attempts)
        {
          // Wait for the third subscription before we proceed.
          DDS::PublicationMatchStatus const publication_status =
                     dw1->get_publication_match_status ();

          if (publication_status.total_count == 3)
            break;

          ACE_OS::sleep (1);
          ++attempts;
        }

        if (attempts == max_attempts)
        {
          cerr << "ERROR: subscriptions failed to match." << endl;
          exit (1);
        }

        writer->start ();

        // Get the offered deadline status to reset the
        // total_count_change field to zero.
        deadline_status1 = dw1->get_offered_deadline_missed_status();

        // Cleanup
        writer->end ();

        deadline_status1 = dw1->get_offered_deadline_missed_status();

        if (deadline_status1.total_count_change != 0)
          {
            cerr << "ERROR: Offered deadlines unexpectedly missed"
                 << endl;

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
