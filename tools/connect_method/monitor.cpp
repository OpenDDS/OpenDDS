// -*- C++ -*-
// ============================================================================
/**
 *  @file   monitor.cpp
 *
 *  $Id: monitor.cpp,v 1.1 2006/03/31 16:53:17 don Exp $
 *
 *  (c) Copyright 2008, Object Computing, Inc.
 *  All Rights Reserved.
 *
 */
// ============================================================================

#include "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <iostream>

int DOMAIN_ID = 42; // same as test domain on bit example

int main (int argc, char *argv[])
{
  try {
    // Initialize, and create a DomainParticipant
    // (same as publisher)
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    DDS::DomainParticipant_var participant =
      dpf->create_participant(DOMAIN_ID,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (0 == participant)
    {
      std::cerr << "create_participant failed." << std::endl;
      ACE_OS::exit(1);
    }

    // Create a subscriber for the two topics.  Make sure it is non-nil
    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     0,
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (0 == sub)
    {
      std::cerr << "create_subscriber failed." << std::endl;
      ACE_OS::exit(1);
    }

    // Get the Built-In Subscriber for Built-In Topics
    DDS::Subscriber_var bit_subscriber =
      participant->get_builtin_subscriber() ;

	DDS::DataReader_var pub_loc_dr =
		bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
	if (0 == pub_loc_dr)
	{
		std::cerr << "Could not get " << OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC
			<< " DataReader." << std::endl;
		ACE_OS::exit(1);
	}
	
	DDS::DataReaderListener_var pub_loc_listener =
		new ParticipantLocationBuiltinTopicDataDataReaderListenerImpl();

	CORBA::Long retcode =
		pub_loc_dr->set_listener(pub_loc_listener,
			OpenDDS::DCPS::DEFAULT_STATUS_MASK);
	if (retcode != DDS::RETCODE_OK)
	{
		std::cerr << "set_listener for " << OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC << " failed." << std::endl;
		ACE_OS::exit(1);
	}

	// Wait for events from the Publisher; shut down when "System Shutdown" received
    std::cout << "Subscriber: waiting for events" << std::endl;
    while (true )
    {
      ACE_OS::sleep(1);
    }

    // Cleanup
    try {
      if (0 != participant) {
        participant->delete_contained_entities();
      }
      if (0 != dpf) {
        dpf->delete_participant(participant);
      }
    } catch (CORBA::Exception& e) {
      std::cerr << "Exception caught in cleanup." << std::endl << e << std::endl;
      ACE_OS::exit(1);
    }
    TheServiceParticipant->shutdown ();
  } catch (CORBA::Exception& e) {
    std::cerr << "Exception caught in main.cpp:" << std::endl
         << e << std::endl;
    ACE_OS::exit(1);
  }

  return 0;
}
