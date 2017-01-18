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
#include "dds/DCPS/scoped_ptr.h"

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
        dpf->create_participant (411,
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
      DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);
      dw_qos.durability.kind = DDS::TRANSIENT_DURABILITY_QOS;
      dw_qos.durability_service.history_kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      dw_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
      dw_qos.resource_limits.max_samples_per_instance = 1000;
      dw_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;

      // -------------------------------------------------------

      {
        // Create a DataWriter.

        // Upon exiting this scope, all unsent data should be
        // transferred to OpenDDS's data durability cache since the
        // run_test.pl script should not have started the subscriber
        // until it detects the "Done writing" log text.
        DDS::DataWriter_var dw_tmp =
          pub->create_datawriter (topic.in (),
                                  dw_qos,
                                  ::DDS::DataWriterListener::_nil (),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (CORBA::is_nil (dw_tmp.in ()))
        {
          cerr << "create_datawriter failed." << endl;
          exit (1);
        }

        // Write samples using multiple threads.
        OpenDDS::DCPS::scoped_ptr<Writer> writer (new Writer (dw_tmp.in ()));
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
      }

      // -------------------------------------------------------

      ACE_Atomic_Op<ACE_SYNCH_MUTEX, bool> publication_matched;
      ::DDS::DataWriterListener_var dwl (
          new DataWriterListenerImpl (publication_matched));

      // Create a new DataWriter.  The data in the durability cache
      // should be transferred to this DataWriter.  Once publication
      // match occurs, the data should be sent to the subscriber.
      DDS::DataWriter_var dw =
        pub->create_datawriter (topic.in (), dw_qos, dwl.in (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

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

      // Wait for DataReader to finish.
      ::DDS::InstanceHandleSeq handles;
      for (attempts = 1; attempts != max_attempts; ++attempts)
      {
        dw->get_matched_subscriptions (handles);
        if (handles.length () == 0)
          break;
        else
          ACE_OS::sleep(1);
      }

      // The data durability cache should no longer contain samples
      // for this domain/topic/type.

      // -------------------------------------------------------

      {
        // Write samples that will not be sent.  Exercise
        // service_cleanup_delay.  We can either do this through the
        // durability member or durability_service member in either of
        // TopicQos or DataWriterQos.  This test arbitrarily uses the
        // DataWriterQos::durability_service member.

        // Cleanup data after this number of seconds.
        CORBA::Long const delay_seconds = 5;

        ::DDS::Duration_t & cleanup_delay =
          dw_qos.durability_service.service_cleanup_delay;
        cleanup_delay.sec     = delay_seconds;
        cleanup_delay.nanosec = 0;

        // Create a dummy topic which will have no subscriptions.
        DDS::Topic_var dummy_topic =
          participant->create_topic ("Dummy Topic",
                                     type_name.in (),
                                     topic_qos,
                                     DDS::TopicListener::_nil(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        DDS::DataWriter_var dummy_dw =
          pub->create_datawriter (dummy_topic.in (),
                                  dw_qos,
                                  ::DDS::DataWriterListener::_nil (),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (CORBA::is_nil (dummy_dw.in ()))
        {
          cerr << "create_datawriter for dummy topic failed." << endl;
          exit (1);
        }

        // Write samples using multiple threads.
        OpenDDS::DCPS::scoped_ptr<Writer> writer (new Writer (dummy_dw.in ()));
        if (!writer->start () || !writer->end ())
        {
          // Error logging performed in above method call.
          exit (1);
        }

        // Explicitly destroy the DataWriter.
        if (pub->delete_datawriter (dummy_dw.in ())
            == ::DDS::RETCODE_PRECONDITION_NOT_MET)
        {
          cerr << "Unable to delete DataWriter" << endl;
          exit (1);
        }

        // Allow durability cleanup to occur
        ACE_OS::sleep (delay_seconds + 3);
      }

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
