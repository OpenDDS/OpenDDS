// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *
 *
 */
// ============================================================================


#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/WaitSet.h>

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include <ace/Time_Value.h>
#include <ace/OS_NS_unistd.h>

#include <cassert>

using namespace std;

char synch_fname[] = "dr_unmatch_done";

int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf;
      DDS::DomainParticipant_var participant;

      dpf = TheParticipantFactoryWithArgs(argc, argv);
      participant =
        dpf->create_participant(11,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1 ;
      }

      Messenger::MessageTypeSupportImpl* mts_servant =
        new Messenger::MessageTypeSupportImpl;

      if (DDS::RETCODE_OK != mts_servant->register_type(participant.in (),
                                                        ""))
      {
        cerr << "Failed to register the MessageTypeTypeSupport." << endl;
        exit(1);
      }

      CORBA::String_var type_name = mts_servant->get_type_name ();

      DDS::TopicQos topic_qos;
      participant->get_default_topic_qos(topic_qos);
      DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  type_name.in (),
                                  topic_qos,
                                  DDS::TopicListener::_nil(),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ())) {
        cerr << "Failed to create_topic." << endl;
        exit(1);
      }

      // Create the subscriber and attach to the corresponding
      // transport.
      DDS::Subscriber_var sub =
        participant->create_subscriber (SUBSCRIBER_QOS_DEFAULT,
                                        DDS::SubscriberListener::_nil(),
                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub.in ())) {
        cerr << "Failed to create_subscriber." << endl;
        exit(1);
      }

     DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      // Make reliable
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

      // Set up a 5 second recurring deadline.
      dr_qos.deadline.period.sec     = 5;
      dr_qos.deadline.period.nanosec = 0;

      // Create two listeners. One for each DataReader.
      DDS::DataReaderListener_var listener1 (new DataReaderListenerImpl);
      if (CORBA::is_nil (listener1.in ()))
      {
        cerr << "ERROR: listener1 is nil." << endl;
        exit(1);
      }

      DDS::DataReaderListener_var listener2 (new DataReaderListenerImpl);
      if (CORBA::is_nil (listener2.in ()))
      {
        cerr << "ERROR: listener2 is nil." << endl;
        exit(1);
      }


      // First data reader has 5 second deadline period.
      DDS::DataReader_var dr1 =
        sub->create_datareader (topic.in (),
                                dr_qos,
                                listener1.in (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      // Set up a 3 second recurring deadline.
      dr_qos.deadline.period.sec     = 3;
      dr_qos.deadline.period.nanosec = 0;

      // Second data reader has 3 second deadline period which
      // is not compatible with DataWriter.
      DDS::DataReader_var dr2 =
        sub->create_datareader (topic.in (),
                                dr_qos,
                                listener2.in (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dr1.in ()) || CORBA::is_nil (dr2.in ()))
      {
        cerr << "ERROR: create_datareader failed." << endl;
        exit(1);
      }

      DataReaderListenerImpl* listener_servant1 =
        dynamic_cast<DataReaderListenerImpl*>(listener1.in());
      DataReaderListenerImpl* listener_servant2 =
        dynamic_cast<DataReaderListenerImpl*>(listener2.in());

      int expected = 10;
      // Writer of deadline 4 -> Reader of deadline 5
      while ( listener_servant1->num_reads() < expected) {
        //cout << "subscriber listener1 waiting for " << expected
             //<< " reads, got " << listener_servant1->num_reads() << std::endl;
        ACE_OS::sleep (1);
      }

      // Writer of deadline 4 and Reader of deadline 3 is not
      // compatible so second DataReader should not receive
      // any message from DataWriter.
      if (listener_servant2->num_reads() > 0)
      {
        cerr << "ERROR: second DataReader should not receive message from "
          << "datawriter as their deadline QoS is not compatible" << endl;
        exit (1);
      }

      // Wait for dr1 to be unmatched from the writer (due to writer set_qos).
      ACE_DEBUG((LM_DEBUG, "(%P|%t) check for dr1 unmatch\n"));
      DDS::WaitSet_var ws = new DDS::WaitSet;
      DDS::StatusCondition_var sc = dr1->get_statuscondition();
      sc->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);
      ws->attach_condition(sc);
      DDS::SubscriptionMatchedStatus matched;
      const DDS::Duration_t timeout = {5, 0}; // seconds
      while (dr1->get_subscription_matched_status(matched) == DDS::RETCODE_OK
             && matched.current_count == matched.total_count)
      {
        DDS::ConditionSeq active;
        ACE_DEBUG((LM_DEBUG, "(%P|%t) wait for dr1 unmatch\n"));
        if (ws->wait(active, timeout) == DDS::RETCODE_TIMEOUT)
        {
          cerr << "ERROR: timeout expired while waiting for dr1 to be "
            "unmatched from the writer which now has a 6 second deadline\n";
          exit (1);
        }
      }
      ws->detach_condition(sc);
      // Create synch file.
      FILE* fp = ACE_OS::fopen (synch_fname, ACE_TEXT("w"));
      if (fp != 0)
      {
        ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) dr unmatch is done write to sync file\n")));
        ACE_OS::fclose (fp);
      }
      ACE_DEBUG((LM_DEBUG, "(%P|%t) done dr1 unmatch\n"));

      // Now change second DataReader to have deadline period to be 5 seconds. This
      // value is compatible with DataWriter so it will be matched.
      dr_qos.deadline.period.sec = 5;

      if (dr2->set_qos (dr_qos) != ::DDS::RETCODE_OK)
      {
        cerr << "ERROR: DataReader changed deadline period to make it compatible "
          << "with datawriter" << endl;
        exit (1);
      }

      // second DataReader should receive 20 messages so far.
      while ( listener_servant1->num_reads() < 2 * expected) {
        //cout << "subscriber listener1 waiting for " << 2 * expected
             //<< " reads, got " << listener_servant1->num_reads() << std::endl;
        ACE_OS::sleep (1);
      }

      // second DataReader should receive 10 messages.
      while ( listener_servant2->num_reads() < expected) {
        //cout << "subscriber listener2 waiting for " << expected
             //<< " reads, got " << listener_servant2->num_reads() << std::endl;
        ACE_OS::sleep (1);
      }

      // During this period, the 5 second should have at most 1 missed
      // deadline, but with 3 seconds, it should have at least 2 missed
      // deadline.
      ACE_OS::sleep (9);

      if (listener_servant2->num_deadline_missed () > 1)
      {
        cerr << "ERROR: failed to verify deadline missed count " << endl;
        exit (1);
      }

      if (!CORBA::is_nil (participant.in ())) {
        participant->delete_contained_entities();
      }
      if (!CORBA::is_nil (dpf.in ())) {
        dpf->delete_participant(participant.in ());
      }

      TheServiceParticipant->shutdown ();
    }
  catch (CORBA::Exception& e)
    {
      cerr << "SUB: Exception caught in main ():" << endl << e << endl;
      return 1;
    }
  TheServiceParticipant->shutdown ();

  return 0;
}
