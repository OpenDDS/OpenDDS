// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 */
// ============================================================================


#include "SatelliteTypeSupportImpl.h"
#include "../common/TestException.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <iostream>

#include "dds/DCPS/StaticIncludes.h"

#include "common.h"

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    // Initialize, and create a DomainParticipant
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    // Get the Satellite Name from the argument list
    std::string satellite_name;
    if ( argc > 1 ) {
      satellite_name = ACE_TEXT_ALWAYS_CHAR (argv[1]);
    } else {
      satellite_name = "Default Satellite";
    }

    //  Create the DomainParticipant
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

    // Create a publisher for the two topic
    // Make sure it is non-nil
    DDS::Publisher_var pub =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                    0,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(pub.in()))
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) create_publisher failed.\n")));
      return 1;
    }

    // Create and register the Alert type support object
    Satellite::AlertTypeSupport_var alert_ts =
      new Satellite::AlertTypeSupportImpl();
    if (DDS::RETCODE_OK != alert_ts->register_type(participant,
                                                   SATELLITE_ALERT_TYPE))
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) register_type for %C failed.\n"), SATELLITE_ALERT_TYPE));
      return 1;
    }

    // Get the default QoS for the two topic
    DDS::TopicQos topic_qos;
    participant->get_default_topic_qos(topic_qos);

    // .. and another topic for the Alert type
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

    // Get QoS to use for our two DataWriters
    DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos (dw_qos);

    //  Set the Liveliness QoS policy on the two DataWriters
    dw_qos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
    DDS::Duration_t dw_liveliness_duration;
    dw_liveliness_duration.sec = 2;
    dw_liveliness_duration.nanosec = 0;
    dw_qos.liveliness.lease_duration = dw_liveliness_duration;

    // Create a DataWriter for the Alert topic, and make sure it is non-nil
    DDS::DataWriter_var alert_base_dw =
      pub->create_datawriter(alert_topic,
                             dw_qos,
                             0,
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(alert_base_dw.in()))
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) create_datawriter for %C failed.\n"), SATELLITE_ALERT_TOPIC));
      return 1;
    }

    // Indicate that the publisher is ready
    FILE* writers_ready = ACE_OS::fopen((temp_file_prefix + pub_ready_filename).c_str(), ACE_TEXT("w"));
    if (writers_ready == 0)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Unable to create publisher ready file\n")));
    }

    // Wait for the subscriber to be ready.
    FILE* readers_ready = 0;
    do
    {
      ACE_Time_Value small_time(0, 250000);
      ACE_OS::sleep(small_time);
      readers_ready = ACE_OS::fopen((temp_file_prefix + sub_ready_filename).c_str(), ACE_TEXT("r"));
    } while (0 == readers_ready);

    ACE_OS::fclose(writers_ready);
    ACE_OS::fclose(readers_ready);

    // Then, narrow the DataWriter created above to an AlertDataWriter,
    // and make sure the narrow's result is non-nil.
    Satellite::AlertDataWriter_var alert_dw =
      Satellite::AlertDataWriter::_narrow(alert_base_dw);
    if (CORBA::is_nil(alert_dw.in()))
    {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: AlertDataWriter could not be narrowed\n"));
      return 1;
    }

    // Fill in the key fields of the FUEL Alert instance, and register it
    Satellite::Alert fuel_alert;
    fuel_alert.satellite = satellite_name.c_str();
    fuel_alert.item = Satellite::FUEL;
    ::DDS::InstanceHandle_t fuel_alert_handle =  alert_dw->register_instance(fuel_alert);

    Satellite::Alert battery_alert;
    // Fill in the key fields of the BATTERY Alert instance, and register it
    battery_alert.satellite = satellite_name.c_str();
    battery_alert.item = Satellite::BATTERY;
    ::DDS::InstanceHandle_t battery_alert_handle = alert_dw->register_instance(battery_alert);

    // Publish...

    ACE_Time_Value oneSecond( 1, 0 );
    for ( int i = 0; i < 20; ++i ) {

      DDS::ReturnCode_t ret;

      // Publish Alerts periodically

      if ( i % 6 == 0 )
      {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) %C : Publishing Low Fuel Alert\n", satellite_name.c_str()));

        // Publish a Low Fuel alert on the fuel_alert struct.  Check the return
        // value of the write() call to make sure the publication succeeded.
        Satellite::Alert fuel_alert;
        fuel_alert.satellite = satellite_name.c_str();
        fuel_alert.item = Satellite::FUEL;
        fuel_alert.code = Satellite::LOW;
        fuel_alert.index = i/6;
        fuel_alert.message = CORBA::string_dup("Your fuel is low");
        ret = alert_dw->write(fuel_alert, fuel_alert_handle);
        if (ret != ::DDS::RETCODE_OK)
        {
          ACE_ERROR((LM_ERROR, "(%P|%t) Fuel Alert write returned error code %d\n", ret));
        }
      }

      if ( i % 15 == 0 )
      {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) %C : Publishing Dead Battery Alert\n", satellite_name.c_str()));

        // Publish a Dead Battery alert on the battery_alert struct.   Check the
        // return value of the write() call to make sure the publication succeeded.
        Satellite::Alert battery_alert;
        battery_alert.satellite = satellite_name.c_str();
        battery_alert.item = Satellite::BATTERY;
        battery_alert.code = Satellite::DEAD;
        battery_alert.index = i/15;
        battery_alert.message =
          CORBA::string_dup("Your battery is dead; re-orient the solar panels to recharge");
        ret = alert_dw->write(battery_alert, battery_alert_handle);
        if (ret != ::DDS::RETCODE_OK)
        {
          ACE_ERROR((LM_ERROR, "(%P|%t) Battery Alert write returned error code %d\n", ret));
        }
      }

      ACE_OS::sleep( oneSecond );
    }

    // Publish a "System Shutdown" message
    Satellite::Alert shutdown_alert;
    shutdown_alert.satellite = satellite_name.c_str();
    shutdown_alert.item = Satellite::SYSTEM;
    ::DDS::InstanceHandle_t shutdown_alert_handle = alert_dw->register_instance(shutdown_alert);
    shutdown_alert.code = Satellite::SYSTEM_SHUTDOWN;
    shutdown_alert.index = 9999;
    shutdown_alert.message =
      CORBA::string_dup("The Satellite is being shut down");
    DDS::ReturnCode_t retcode = alert_dw->write(shutdown_alert, shutdown_alert_handle);
    if (retcode != ::DDS::RETCODE_OK)
    {
      ACE_ERROR((LM_ERROR, "(%P|%t) System Shutdown write returned error code %d\n", retcode));
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T Writers are finished\n")));

    // Indicate that the publisher is done
    FILE* writers_completed = ACE_OS::fopen((temp_file_prefix + pub_finished_filename).c_str(), ACE_TEXT("w"));
    if (writers_completed == 0)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Unable to create publisher completed file\n")));
    }


    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T waiting for readers to finish\n")));

    // Wait for the subscriber to finish.
    FILE* readers_completed = 0;
    do
    {
      ACE_Time_Value small_time(0, 250000);
      ACE_OS::sleep(small_time);
      readers_completed = ACE_OS::fopen((temp_file_prefix + sub_finished_filename).c_str(), ACE_TEXT("r"));
    } while (0 == readers_completed);

    ACE_OS::fclose(writers_completed);
    ACE_OS::fclose(readers_completed);

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T Readers are finished\n")));

    // Clean up publisher objects
    pub->delete_contained_entities();

    // Cleanup
    try {
      if (0 != participant) {
        participant->delete_contained_entities();
      }
      if (0 != dpf) {
        dpf->delete_participant(participant);
      }
    } catch (CORBA::Exception& e) {
      e._tao_print_exception("Exception caught in cleanup:");
      return 1;
    }
    TheServiceParticipant->shutdown ();

    ACE_DEBUG((LM_DEBUG, "Exiting...\n"));
  }
  catch (const TestException&)
  {
    ACE_ERROR((LM_ERROR, "Exception caught in main.cpp: %C\n"));
    return 1;
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception("Exception caught in main.cpp:");
    return 1;
  }

  return 0;
}
