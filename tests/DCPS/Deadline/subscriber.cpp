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
#include "dds/DCPS/WaitSet.h"
#include "dds/DdsDcpsInfrastructureC.h"

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

static int const num_messages = 10;
static ACE_Time_Value write_interval(0, 500000);

const int NUM_INSTANCE = 2;

// Set up a 5 second recurring deadline.
static DDS::Duration_t const DEADLINE_PERIOD =
{
  5,  // seconds
    0   // nanoseconds
};
// Time to sleep waiting for deadline periods to expire
long const NUM_EXPIRATIONS = 2;
ACE_Time_Value const SLEEP_DURATION(DEADLINE_PERIOD.sec * NUM_EXPIRATIONS + 1);


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


      // ----------------------------------------------
      {
        // Attempt to create a DataReader with intentionally
        // incompatible QoS.
        DDS::DataReaderQos bogus_qos;
        sub->get_default_datareader_qos (bogus_qos);

        // Set up a 2 second recurring deadline.  DataReader creation
        // should fail with this QoS since the requested deadline period
        // will be less than the test configured offered deadline
        // period.
        bogus_qos.deadline.period.sec = 2;
        bogus_qos.deadline.period.nanosec = 0;

        DDS::DataReader_var tmp_dr =
          sub->create_datareader (topic.in (),
                                  bogus_qos,
                                  DDS::DataReaderListener::_nil (),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (CORBA::is_nil (tmp_dr.in ()))
        {
          cerr << "ERROR: DataReader creation with bogus QoS failed."
               << endl;
          exit (1);
        }

        DDS::StatusCondition_var cond = tmp_dr->get_statuscondition();
        cond->set_enabled_statuses(DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS);
        DDS::WaitSet_var ws = new DDS::WaitSet;
        ws->attach_condition(cond);
        DDS::Duration_t four_sec = {4, 0};
        DDS::ConditionSeq active;
        ws->wait(active, four_sec);

        // Check if the incompatible deadline was correctly flagged.
        if ((active.length() == 0) || (active[0] != cond)) {
          cerr << "ERROR: Failed to get requested incompatible qos status" << endl;
          exit (1);
        }

        DDS::RequestedIncompatibleQosStatus incompatible_status;
        if (tmp_dr->get_requested_incompatible_qos_status (incompatible_status) != ::DDS::RETCODE_OK)
        {
          cerr << "ERROR: Failed to get requested incompatible qos status" << endl;
          exit (1);
        }

        DDS::QosPolicyCountSeq const & policies =
          incompatible_status.policies;

        bool incompatible_deadline = false;
        CORBA::ULong const len = policies.length ();
        for (CORBA::ULong i = 0; i < len; ++i)
        {
          if (policies[i].policy_id == DDS::DEADLINE_QOS_POLICY_ID)
          {
            incompatible_deadline = true;
            break;
          }
        }

        if (!incompatible_deadline)
        {
          cerr << "ERROR: A DataReader/Writer association was created " << endl
               << "       despite use of deliberately incompatible deadline "
               << "QoS." << endl;
          exit (1);
        }
      }


      // ----------------------------------------------

      // Create the listener.
      DDS::DataReaderListener_var listener (new DataReaderListenerImpl);
      DataReaderListenerImpl* listener_servant =
        dynamic_cast<DataReaderListenerImpl*>(listener.in());

      if (CORBA::is_nil (listener.in ()))
      {
        cerr << "ERROR: listener is nil." << endl;
        exit(1);
      }

      DDS::DataReaderQos dr_qos; // Good QoS.
      sub->get_default_datareader_qos (dr_qos);

      assert (DEADLINE_PERIOD.sec > 1); // Requirement for the test.

      // First data reader will have a listener to test listener
      // callback on deadline expiration.
      DDS::DataReader_var dr1 =
        sub->create_datareader (topic.in (),
                                dr_qos,
                                listener.in (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      // Second data reader will not have a listener to test proper
      // handling of a nil listener in the deadline handling code.
      DDS::DataReader_var dr2 =
        sub->create_datareader (topic.in (),
                                dr_qos,
                                DDS::DataReaderListener::_nil (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dr1.in ()) || CORBA::is_nil (dr2.in ()))
      {
        cerr << "ERROR: create_datareader failed." << endl;
        exit(1);
      }

      dr_qos.deadline.period.sec     = DEADLINE_PERIOD.sec;
      dr_qos.deadline.period.nanosec = DEADLINE_PERIOD.nanosec;

      // Reset qos to have deadline. The watch dog now starts.
      if (dr1->set_qos (dr_qos) != ::DDS::RETCODE_OK
        || dr2->set_qos (dr_qos) != ::DDS::RETCODE_OK)
      {
        cerr << "ERROR: set deadline qos failed." << endl;
        exit(1);
      }

      Messenger::MessageDataReader_var message_dr1 =
        Messenger::MessageDataReader::_narrow(dr1.in());

      Messenger::MessageDataReader_var message_dr2 =
        Messenger::MessageDataReader::_narrow(dr2.in());

      int max_attempts = 10;
      int attempts = 0;

      // Synchronize with publisher. Wait until both associate with DataWriter.
      while (attempts < max_attempts)
      {
        ::DDS::SubscriptionMatchedStatus status1;
        ::DDS::SubscriptionMatchedStatus status2;
        if (dr1->get_subscription_matched_status (status1) == ::DDS::RETCODE_OK
          && dr2->get_subscription_matched_status (status2) == ::DDS::RETCODE_OK)
        {
          if (status1.total_count == 1 && status2.total_count == 1)
            break;
          ++ attempts;
          ACE_OS::sleep (1);
        }
        else
        {
          cerr << "ERROR: Failed to get subscription matched status" << endl;
          exit (1);
        }
      }

      if (attempts >= max_attempts)
      {
        cerr << "ERROR: failed to make associations. " << endl;
        exit (1);
      }
      // ----------------------------------------------

      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Subscriber: sleep for %d milliseconds\n"),
                            SLEEP_DURATION.msec()));

      // Wait for deadline periods to expire.
      ACE_OS::sleep (SLEEP_DURATION);

      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Subscriber: now verify missed ")
                            ACE_TEXT ("deadline status \n")));

      DDS::RequestedDeadlineMissedStatus deadline_status1;
      if (dr1->get_requested_deadline_missed_status(deadline_status1) != ::DDS::RETCODE_OK)
      {
        cerr << "ERROR: Failed to get requested deadline missed status" << endl;
        exit (1);
      }

      DDS::RequestedDeadlineMissedStatus deadline_status2;
      if (dr2->get_requested_deadline_missed_status(deadline_status2) != ::DDS::RETCODE_OK)
      {
        cerr << "ERROR: Failed to get requested deadline missed status" << endl;
        exit (1);
      }

      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Subscriber: got missed")
                            ACE_TEXT ("deadline status \n")));

      Messenger::Message message;
      message.subject_id = 99;
      ::DDS::InstanceHandle_t dr1_hd1 = message_dr1->lookup_instance (message);
      ::DDS::InstanceHandle_t dr2_hd1 = message_dr2->lookup_instance (message);
      message.subject_id = 100;
      ::DDS::InstanceHandle_t dr1_hd2 = message_dr1->lookup_instance (message);
      ::DDS::InstanceHandle_t dr2_hd2 = message_dr2->lookup_instance (message);

      if (deadline_status1.last_instance_handle != dr1_hd1
        && deadline_status1.last_instance_handle != dr1_hd2)
      {
        cerr << "ERROR: Expected DR1 last instance handle ("
             << dr1_hd1 << " or " << dr1_hd2 << ") did not occur ("
             << deadline_status1.last_instance_handle << ")" << endl;

        exit (1);
      }

      if (deadline_status2.last_instance_handle != dr2_hd1
        && deadline_status2.last_instance_handle != dr2_hd2)
      {
        cerr << "ERROR: Expected DR2 last instance handle ("
             << dr2_hd1 << " or " << dr2_hd2 << ") did not occur ("
             << deadline_status2.last_instance_handle << endl;

        exit (1);
      }

      //The reader deadline period is 5 seconds and writer writes
      //each instance every 9 seconds, so after SLEEP_DURATION(11secs),
      //the deadline missed should be 1 per instance
      if (deadline_status1.total_count != NUM_INSTANCE
          || deadline_status2.total_count != NUM_INSTANCE)
      {
        cerr << "ERROR: Expected number of missed requested "
             << "deadlines (" << NUM_INSTANCE << ") " << "did " << endl
             << "       not occur ("
             << deadline_status1.total_count << " and/or "
             << deadline_status2.total_count << ")." << endl;

        exit (1);
      }

      if (deadline_status1.total_count_change != NUM_INSTANCE
          || deadline_status2.total_count_change != NUM_INSTANCE)
      {
        cerr << "ERROR: Incorrect missed requested "
             << "deadline count change" << endl
             << "       ("
             << deadline_status1.total_count_change
             << " and/or "
             << deadline_status2.total_count_change
             << " instead of " << NUM_EXPIRATIONS * NUM_INSTANCE << ")."
             << endl;

        exit (1);
      }

      // Here the writers should continue writes all samples with
      // .5 second interval.
      ACE_Time_Value no_miss_period = num_messages * write_interval;

      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Subscriber: sleep for %d msec\n"),
                            (SLEEP_DURATION + no_miss_period).msec()));

      // Wait for another set of deadline periods(5 + 11 secs).
      // During this period, the writers continue write all samples with
      // .5 second interval.
      ACE_OS::sleep (SLEEP_DURATION + no_miss_period);

      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Subscriber: now verify missed ")
                            ACE_TEXT ("deadline status \n")));

      if ((dr1->get_requested_deadline_missed_status(deadline_status1) != ::DDS::RETCODE_OK)
        || (dr2->get_requested_deadline_missed_status(deadline_status2) != ::DDS::RETCODE_OK))
      {
        cerr << "ERROR: failed to get requested deadline missed status" << endl;
        exit (1);
      }

      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Subscriber: got missed")
                            ACE_TEXT ("deadline status \n")));

      if (deadline_status1.last_instance_handle != dr1_hd1
        && deadline_status1.last_instance_handle != dr1_hd2)
      {
        cerr << "ERROR: Expected DR1 last instance handle ("
             << dr1_hd1 << " or " << dr1_hd2 << ") did not occur ("
             << deadline_status1.last_instance_handle << ")" << endl;

        exit (1);
      }

      if (deadline_status2.last_instance_handle != dr2_hd1
        && deadline_status2.last_instance_handle != dr2_hd2)
      {
        cerr << "ERROR: Expected DR2 last instance handle ("
             << dr2_hd1 << " or " << dr2_hd2 << ") did not occur ("
             << deadline_status2.last_instance_handle << endl;

        exit (1);
      }

      if (deadline_status1.total_count != 3 * NUM_INSTANCE
          || deadline_status2.total_count != 3 * NUM_INSTANCE)
      {
        cerr << "ERROR: Another expected number of missed requested "
             << "deadlines (" << NUM_INSTANCE << ")" << endl
             << "       did not occur ("
             << deadline_status1.total_count << " and/or "
             << deadline_status2.total_count << ")." << endl;

        exit (1);
      }

      if (deadline_status1.total_count_change != 2 * NUM_INSTANCE
          || deadline_status2.total_count_change != 2 * NUM_INSTANCE)
      {
        cerr << "ERROR: Incorrect missed requested "
             << "deadline count" << endl
             << "       change ("
             << deadline_status1.total_count_change
             << "and/or "
             << deadline_status2.total_count_change
             << " instead of " << NUM_EXPIRATIONS << ")." << endl;

        exit (1);
      }


      int expected = 10;
      while ( listener_servant->num_arrived() < expected) {
        ACE_OS::sleep (1);
      }

      if (!CORBA::is_nil (participant.in ())) {
        participant->delete_contained_entities();
      }
      if (!CORBA::is_nil (dpf.in ())) {
        dpf->delete_participant(participant.in ());
      }

      ACE_OS::sleep(2);

      TheServiceParticipant->shutdown ();
    }
  catch (CORBA::Exception& e)
    {
      cerr << "SUB: Exception caught in main ():" << endl << e << endl;
      return 1;
    }

  return 0;
}
