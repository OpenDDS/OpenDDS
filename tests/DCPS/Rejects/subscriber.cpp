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

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include <ace/Time_Value.h>
#include <ace/OS_NS_unistd.h>

using namespace std;

static ACE_Time_Value write_interval(0, 500000);

static ACE_Time_Value SLEEP_DURATION(20);


const int MAX_INSTANCES = 2;
const int MAX_SAMPLES = 7;
const int MAX_SAMPLES_PER_INSTANCES = 4;


int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  int return_result = 0;
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

      Messenger::MessageTypeSupportImpl::_var_type mts_servant =
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

      // Create the listener.
      DDS::DataReaderListener_var listener (new DataReaderListenerImpl);
      DataReaderListenerImpl* listener_servant =
        dynamic_cast<DataReaderListenerImpl*>(listener.in());

      if (CORBA::is_nil (listener.in ()))
      {
        cerr << "ERROR: listener is nil." << endl;
        exit(1);
      }
      if (!listener_servant) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: listener_servant is nil (dynamic_cast failed)!\n")), -1);
      }

      DDS::DataReaderQos dr_qos; // Good QoS.
      sub->get_default_datareader_qos (dr_qos);

      dr_qos.resource_limits.max_samples_per_instance = MAX_SAMPLES_PER_INSTANCES;
      dr_qos.resource_limits.max_samples = MAX_SAMPLES;
      dr_qos.resource_limits.max_instances = MAX_INSTANCES;
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
      dr_qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      dr_qos.history.depth = MAX_SAMPLES_PER_INSTANCES;
#endif

      DDS::DataReader_var dr1 =
        sub->create_datareader (topic.in (),
                                dr_qos,
                                listener.in (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dr1.in ()) )
      {
        cerr << "ERROR: create_datareader failed." << endl;
        exit(1);
      }

      DDS::DataReader_var dr2 =
        sub->create_datareader (topic.in (),
                                dr_qos,
                                DDS::DataReaderListener::_nil (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dr2.in ()) )
      {
        cerr << "ERROR: create_datareader failed." << endl;
        exit(1);
      }

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
          if (status1.total_count == 2 && status2.total_count == 2)
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

      // Wait for publisher to finish sending
      ACE_OS::sleep (SLEEP_DURATION);

      long rej_max_samples = listener_servant->num_rejected_for_max_samples();
      long rej_max_instances = listener_servant->num_rejected_for_max_instances();
      long rej_max_samp_instance = listener_servant->num_rejected_for_max_samples_per_instance();

      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Subscriber: %d rejected for ")
                            ACE_TEXT ("max_samples\n"),
                            rej_max_samples));
      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Subscriber: %d rejected for ")
                            ACE_TEXT ("max_instances\n"),
                            rej_max_instances));
      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Subscriber: %d rejected for ")
                            ACE_TEXT ("max_samples_per_instance\n"),
                            rej_max_samp_instance));
      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) Subscriber: received %d ")
                            ACE_TEXT ("samples\n"),
                            listener_servant->num_arrived() ));

      // 3 instances writing 5 messages each
      // expect 2 rejected for max_samples
      // expect 6 rejected for max_instances (register_instance + 5 messages)
      // expect 1 rejected for max_samples_per_instance

#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
      if (rej_max_samples != 2) {
        cerr << "ERROR: Failed to reject expected for max_samples" << endl;
        return_result = 1;
      }
#endif
      if (rej_max_instances != 6) {
        cerr << "ERROR: Failed to reject expected for max_instances" << endl;
        return_result = 1;
      }
#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
      if (rej_max_samp_instance != 1) {
        cerr << "ERROR: Failed to reject expected for max_samples_per_instance" << endl;
        return_result = 1;
      }
#endif

      Messenger::MessageDataReader_var message_dr1 =
        Messenger::MessageDataReader::_narrow(dr1.in());
      Messenger::MessageDataReader_var message_dr2 =
        Messenger::MessageDataReader::_narrow(dr2.in());
      message_dr1->set_listener(DDS::DataReaderListener::_nil (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      message_dr2->set_listener(DDS::DataReaderListener::_nil (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      listener = DDS::DataReaderListener::_nil ();
      ACE_OS::sleep (2);

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
      return_result = 1;
    }

  return return_result;
}
