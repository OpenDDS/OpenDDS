// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *
 *
 */
// ============================================================================

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"
#include "DataWriterListenerImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/scoped_ptr.h>

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/streams.h>
#include <ace/OS_NS_unistd.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"

#include <memory>
#include <assert.h>

using namespace Messenger;
using namespace std;

// Set up a 4 second recurring deadline.
static DDS::Duration_t const DEADLINE_PERIOD =
{
  4,  // seconds
  0   // nanoseconds
};

static int NUM_EXPIRATIONS = 2;

// Time to sleep waiting for deadline periods to expire
static ACE_Time_Value SLEEP_DURATION(DEADLINE_PERIOD.sec * NUM_EXPIRATIONS + 1);

static int NUM_WRITE_THREADS = 2;

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]){
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);
      DDS::DomainParticipant_var participant =
        dpf->create_participant(11,
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

      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil(), ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ())) {
        cerr << "create_publisher failed." << endl;
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

      assert (DEADLINE_PERIOD.sec > 1); // Requirement for the test.

      // First data writer will have a listener to test listener
      // callback on deadline expiration.
      DDS::DataWriter_var dw =
        pub->create_datawriter (topic.in (),
                                dw_qos,
                                listener.in (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dw.in ()))
      {
        cerr << "ERROR: create_datawriter failed." << endl;
        exit(1);
      }

      dw_qos.deadline.period.sec     = DEADLINE_PERIOD.sec;
      dw_qos.deadline.period.nanosec = DEADLINE_PERIOD.nanosec;

      // Set qos with deadline. The watch dog starts now.
      if (dw->set_qos (dw_qos) != ::DDS::RETCODE_OK)
      {
        cerr << "ERROR: set deadline qos failed." << endl;
        exit(1);
      }

      {
        // Two threads use same datawriter to write different instances.
        OpenDDS::DCPS::scoped_ptr<Writer> writer1 (new Writer (dw.in (), 99, SLEEP_DURATION));
        OpenDDS::DCPS::scoped_ptr<Writer> writer2 (new Writer (dw.in (), 100, SLEEP_DURATION));

        writer1->start ();
        writer2->start ();
        // ----------------------------------------------

        // Wait for fully associate with DataReaders.
        if (writer1->wait_for_start () == false || writer2->wait_for_start () == false)
        {
          cerr << "ERROR: took too long to associate. " << endl;
          exit (1);
        }

        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Publisher: sleep for %d milliseconds\n"),
                              SLEEP_DURATION.msec ()));

        // Wait for a set of deadline periods to expire.
        ACE_OS::sleep (SLEEP_DURATION);

        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Publisher: now verify missed ")
                            ACE_TEXT ("deadline status \n")));

        ::DDS::InstanceHandle_t handle1 = writer1->get_instance_handle ();
        ::DDS::InstanceHandle_t handle2 = writer2->get_instance_handle ();

        DDS::OfferedDeadlineMissedStatus deadline_status;
        if (dw->get_offered_deadline_missed_status(deadline_status) != ::DDS::RETCODE_OK)
        {
           cerr << "ERROR: Failed to get offered deadline missed status" << endl;
           exit (1);
        }

        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Publisher: got missed")
                              ACE_TEXT ("deadline status \n")));

        if (deadline_status.total_count != NUM_EXPIRATIONS * NUM_WRITE_THREADS)
        {
          cerr << "ERROR: Unexpected number of missed offered "
            << "deadlines (" << deadline_status.total_count
            << " instead of " << NUM_EXPIRATIONS * NUM_WRITE_THREADS << ") "
            << endl;

          exit (1);
        }

        if (deadline_status.total_count_change != NUM_EXPIRATIONS * NUM_WRITE_THREADS)
        {
          cerr << "ERROR: Incorrect missed offered "
            << "deadline count change ("
            << deadline_status.total_count_change
            << ") instead of " << NUM_EXPIRATIONS * NUM_WRITE_THREADS
            << endl;

          exit (1);
        }

        if (deadline_status.last_instance_handle != handle1
          && deadline_status.last_instance_handle != handle2)
        {
          cerr << "ERROR: Unexpected last instance handle "
            << deadline_status.last_instance_handle << " instead of "
            << handle1 << " or "
            << handle2 << endl;
          exit (1);
        }

        writer1->wait ();
        writer2->wait ();

        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Publisher: sleep for %d milliseconds\n"),
                              SLEEP_DURATION.msec()));

        // Wait for another set of deadline periods to expire.
        ACE_OS::sleep (SLEEP_DURATION);

        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Publisher: now verify missed ")
                            ACE_TEXT ("deadline status \n")));

        if (dw->get_offered_deadline_missed_status(deadline_status) != ::DDS::RETCODE_OK)
        {
           cerr << "ERROR: Failed to get offered deadline missed status" << endl;
           exit (1);
        }

        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Publisher: got missed")
                              ACE_TEXT ("deadline status \n")));

        if (deadline_status.total_count != (NUM_EXPIRATIONS + 2) * NUM_WRITE_THREADS)
        {
          cerr << "ERROR: Unexpected number of missed offered "
            << "deadlines (" << deadline_status.total_count
            << " instead of " << (NUM_EXPIRATIONS + 2) * NUM_WRITE_THREADS << ") "
            << endl;

          exit (1);
        }

        if (deadline_status.total_count_change != NUM_WRITE_THREADS * 2)
        {
          cerr << "ERROR: Incorrect missed offered "
            << "deadline count change ("
            << deadline_status.total_count_change
            << ") instead of " << NUM_WRITE_THREADS * 2
            << endl;

          exit (1);
        }

        if (deadline_status.last_instance_handle != handle1
          && deadline_status.last_instance_handle != handle2)
        {
          cerr << "ERROR: Unexpected last instance handle "
            << deadline_status.last_instance_handle << " instead of "
            << handle1 << " or "
            << handle2 << endl;
          exit (1);
        }


        // Wait for datareader finish.
        while (1)
        {
          ::DDS::InstanceHandleSeq handles;
          dw->get_matched_subscriptions (handles);
          if (handles.length () == 0)
            break;
          else
            ACE_OS::sleep(1);
        }
      }

      participant->delete_contained_entities();
      dpf->delete_participant(participant.in ());
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
