// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 */
// ============================================================================

#include "../common/TestException.h"
#include "SatelliteTypeSupportImpl.h"
#include "AlertDataReaderListenerImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <iostream>

#include "dds/DCPS/StaticIncludes.h"

#include "common.h"


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = 0;

  try {
    // Initialize, and create a DomainParticipant
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    DDS::DomainParticipant_var participant =
      dpf->create_participant(SATELLITE_DOMAIN_ID,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(participant.in()))
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) create_participant failed.\n")));
      return 1;
    }

    // Create a subscriber for the topic.  Make sure it is non-nil
    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     0,
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(sub.in()))
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) create_subscriber failed.\n")));
      return 1;
    }

    // Register the Alert type
    // (same as publisher)
    Satellite::AlertTypeSupport_var alert_ts = new Satellite::AlertTypeSupportImpl();
    if (DDS::RETCODE_OK != alert_ts->register_type(participant,
                                                   SATELLITE_ALERT_TYPE))
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) register_type for %C failed.\n"), SATELLITE_ALERT_TYPE));
      return 1;
    }

    // Get QoS to use for our topic
    DDS::TopicQos topic_qos;
    participant->get_default_topic_qos(topic_qos);

    // Create a topic for the Alert type...
    DDS::Topic_var alert_topic =
      participant->create_topic (SATELLITE_ALERT_TOPIC,
                                 SATELLITE_ALERT_TYPE,
                                 topic_qos,
                                 0,
                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(alert_topic.in()))
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) create_topic for %C failed.\n"), SATELLITE_ALERT_TOPIC));
      return 1;
    }

    // Create DataReaders and DataReaderListeners for the topic

    // Create the Alert listener servant
    DDS::DataReaderListener_var alert_listener =
      new AlertDataReaderListenerImpl();

    // Create the Alert DataReader
        DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);

    dr_qos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
    DDS::Duration_t dr_liveliness_duration;
    dr_liveliness_duration.sec = 2;
    dr_liveliness_duration.nanosec = 0;
    dr_qos.liveliness.lease_duration = dr_liveliness_duration;
    dr_qos.time_based_filter.minimum_separation.sec = 3;
    dr_qos.time_based_filter.minimum_separation.nanosec = 0;

    // Create the data reader (Alert)
    // and attach the listener
    DDS::DataReader_var alert_dr =
      sub->create_datareader(alert_topic,
                             dr_qos,
                             alert_listener,
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // Wait for events from the Publisher;
    // shut down when "System Shutdown" received and publisher is finished
    ACE_DEBUG((LM_DEBUG, "Subscriber: waiting for events\n"));

    // Indicate that the subscriber is ready
    FILE* readers_ready = ACE_OS::fopen((temp_file_prefix + sub_ready_filename).c_str(), ACE_TEXT("w"));
    if (readers_ready == 0)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber completed file\n")));
      return 1;
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T waiting for publisher to be ready\n")));

    // Wait for the publisher to be ready
    FILE* writers_ready = 0;
    do
    {
      ACE_Time_Value small_time(0, 250000);
      ACE_OS::sleep(small_time);
      writers_ready = ACE_OS::fopen((temp_file_prefix + pub_ready_filename).c_str(), ACE_TEXT("r"));
    } while (0 == writers_ready);

    ACE_OS::fclose(readers_ready);
    ACE_OS::fclose(writers_ready);

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T Publisher is ready\n")));

    // Indicate that the subscriber is done
    // (((it is done when ever the publisher is done)))
    FILE* readers_completed = ACE_OS::fopen((temp_file_prefix + sub_finished_filename).c_str(), ACE_TEXT("w"));
    if (readers_completed == 0)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber completed file\n")));
      return 1;
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T waiting for publisher to finish\n")));
    // Wait for the publisher to finish
    FILE* writers_completed = 0;
    do
    {
      ACE_Time_Value small_time(0, 250000);
      ACE_OS::sleep(small_time);
      writers_completed = ACE_OS::fopen((temp_file_prefix + pub_finished_filename).c_str(), ACE_TEXT("r"));
    } while (0 == writers_completed);

    ACE_OS::fclose(readers_completed);
    ACE_OS::fclose(writers_completed);

    //
    // We need to wait for liveliness to go away here.
    //
    ACE_OS::sleep(5);

    ACE_DEBUG((LM_DEBUG, "\n=== TEST COMPLETE ===\n"));

    //
    // Determine the test status at this point.
    //
    AlertDataReaderListenerImpl* drl_servant =
      dynamic_cast<AlertDataReaderListenerImpl*>(alert_listener.in());

    if (!drl_servant)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) Get Alert Data Reader Listener Impl failed.\n")));
      return 1;
    }

    if (drl_servant->liveliness_changed_count() < 2 ||
        drl_servant->liveliness_changed_count() > 3) {
      status = 1;
      // Some error condition.
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: subscriber - ")
        ACE_TEXT("test failed expected liveliness change count check.\n")
        ));
    }
    else if (!drl_servant->verify_last_liveliness_status()) {
      status = 1;
      // Some other error condition.
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: subscriber - ")
        ACE_TEXT("test failed last livelyness status check.\n")
        ));
    }
    else if (drl_servant->error_occurred())
    {
      status = 1;
      // Some other error condition.
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: subscriber - ")
        ACE_TEXT("test failed due to data reader error.\n")
        ));
    }

    sub->delete_contained_entities();

    // Cleanup
    if (0 != participant) {
      participant->delete_contained_entities();
    }
    if (0 != dpf) {
      dpf->delete_participant(participant);
    }
    TheServiceParticipant->shutdown ();

  }
  catch (std::exception& e)
  {
    ACE_ERROR((LM_ERROR, "Exception caught in main.cpp: %C\n", e.what()));
    return 1;
  }
  catch (CORBA::Exception& e)
  {
    e._tao_print_exception("Exception caught in main.cpp:");
    return 1;
  }

  return status;
}
