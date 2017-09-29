// -*- C++ -*-

// ============================================================================
/**
 *  @file   publisher.cpp
 *
 */
// ============================================================================

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"
#include "DataWriterListenerImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include "dds/DCPS/StaticIncludes.h"
#include "dds/DCPS/unique_ptr.h"

#include <ace/streams.h>
#include "ace/OS_NS_unistd.h"

#include <memory>

using namespace Messenger;

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[]) {
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs (argc, argv);
      DDS::DomainParticipant_var participant =
        dpf->create_participant (111,
                                 PARTICIPANT_QOS_DEFAULT,
                                 DDS::DomainParticipantListener::_nil(),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ()))
      {
        cerr << "create_participant failed." << endl;
        return 1;
      }

      MessageTypeSupport_var servant = new MessageTypeSupportImpl ();

      if (DDS::RETCODE_OK != servant->register_type(participant.in (), ""))
      {
        cerr << "register_type failed." << endl;
        exit (1);
      }

      CORBA::String_var type_name = servant->get_type_name ();

      DDS::TopicQos topic_qos;
      participant->get_default_topic_qos (topic_qos);
      DDS::Topic_var topic =
        participant->create_topic ("Movie Discussion List",
                                   type_name.in (),
                                   topic_qos,
                                   DDS::TopicListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ()))
      {
        cerr << "create_topic failed." << endl;
        exit (1);
      }

      DDS::Publisher_var pub =
        participant->create_publisher (PUBLISHER_QOS_DEFAULT,
                                       DDS::PublisherListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ()))
      {
        cerr << "create_publisher failed." << endl;
        exit (1);
      }

      // Configure DataWriter QoS policies.
      DDS::Duration_t max_block_time = {2, 0};
      DDS::Duration_t deadline_time = {3, 0};
      DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);
      dw_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
      dw_qos.reliability.max_blocking_time = max_block_time;
      dw_qos.durability.kind = DDS::TRANSIENT_DURABILITY_QOS;
      dw_qos.durability_service.history_kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      dw_qos.durability_service.history_depth = 10;
      dw_qos.durability_service.max_samples = 10;
      dw_qos.durability_service.max_samples_per_instance = 10;
      dw_qos.durability_service.max_instances = 2;
      dw_qos.deadline.period = deadline_time;

      // Create a DataWriter.
      ACE_Atomic_Op<ACE_SYNCH_MUTEX, bool> publication_matched;
      ::DDS::DataWriterListener_var dwl (
          new DataWriterListenerImpl (publication_matched));

        // Upon exiting this scope, all unsent data should be
        // transferred to OpenDDS's data durability cache since the
        // run_test.pl script should not have started the subscriber
        // until it detects the "Done writing" log text.
        DDS::DataWriter_var dw_tmp =
          pub->create_datawriter (topic.in (),
                                  dw_qos,
                                  dwl.in (),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (CORBA::is_nil (dw_tmp.in ()))
        {
          cerr << "create_datawriter failed." << endl;
          exit (1);
        }

      int const max_attempts = 50;
      int attempts;
      for (attempts = 1;
           attempts != max_attempts && publication_matched.value () == false;
           ++attempts)
      {
        ACE_OS::sleep (5);
      }

      if (attempts == max_attempts)
      {
        cerr << "ERROR: subscriptions failed to match." << endl;
        exit (1);
      }

        // Write samples using multiple threads.
        OpenDDS::DCPS::unique_ptr<Writer> writer (new Writer (dw_tmp.in ()));
        if (!writer->start () || !writer->end ())
        {
          // Error logging performed in above method call.
          exit (1);
        }

        // Explicitly destroy the DataWriter.
        if (pub->delete_datawriter (dw_tmp.in ())
            == ::DDS::RETCODE_PRECONDITION_NOT_MET)
        {
          cerr << "Unable to delete DataWriter" << endl;
          exit (1);
        }

        ACE_DEBUG ((LM_INFO,
                    ACE_TEXT ("(%P|%t) Deleted DataWriter.\n")));

      participant->delete_contained_entities();
      dpf->delete_participant(participant.in ());
      TheServiceParticipant->shutdown ();
  }
  catch (CORBA::Exception& e)
    {
       cerr << "PUB: Exception caught in main.cpp:" << endl
         << e << endl;
      exit (1);
    }

  return 0;
}
