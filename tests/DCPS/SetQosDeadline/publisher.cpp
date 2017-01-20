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
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/WaitSet.h>
#include "dds/DCPS/StaticIncludes.h"
#include "dds/DCPS/scoped_ptr.h"

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"

#include <memory>

using namespace Messenger;
using namespace std;

char synch_fname[] = "dr_unmatch_done";

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
        ACE_DEBUG((LM_DEBUG, "create_participant failed.\n"));
        return 1;
      }

      MessageTypeSupportImpl* servant = new MessageTypeSupportImpl();

      if (DDS::RETCODE_OK != servant->register_type(participant.in (), "")) {
        ACE_DEBUG((LM_DEBUG, "register_type failed.\n"));
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
        ACE_DEBUG((LM_DEBUG, "create_topic failed.\n"));
        exit(1);
      }

      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ())) {
        ACE_DEBUG((LM_DEBUG, "create_publisher failed.\n"));
        exit(1);
      }

      DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.deadline.period.sec     = 4;
      dw_qos.deadline.period.nanosec = 0;

      // Create DataWriter with 4 second deadline period which
      // should be compatible with first DataReader which has 5
      // seconds deadline period and not with second DataReader
      // which has 3 seconds deadline period.
      DDS::DataWriter_var dw =
        pub->create_datawriter(topic, dw_qos, 0,
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      int const max_attempts = 20000;
      int attempts = 1;
      {
        // Wait for both first DataReader connect and write messages.
        OpenDDS::DCPS::scoped_ptr<Writer> writer (new Writer (dw.in ()));

        while (attempts != max_attempts)
        {
          ::DDS::InstanceHandleSeq handles;
          dw->get_matched_subscriptions(handles);
          if (handles.length() == 1)
            break;
          else
            ACE_OS::sleep(1);
          ++attempts;
        }

        if (attempts == max_attempts)
        {
          ACE_DEBUG((LM_DEBUG, "ERROR: subscriptions failed to match.\n"));
          exit (1);
        }

        writer->start ();
        writer->end ();

        ACE_DEBUG((LM_DEBUG, "Writer changing deadline to incompatible value\n"));

        // Now set DataWriter deadline to be 6 seconds which is not
        // compatible with the existing DataReader. This QoS change
        // should be applied and the association broken.
        dw_qos.deadline.period.sec = 6;

        if (dw->set_qos (dw_qos) != ::DDS::RETCODE_OK) {
          ACE_DEBUG((LM_DEBUG,
            "ERROR: DataWriter could not change deadline period which "
             "should break DataReader associations\n"));
          exit (1);
        } else {

          DDS::WaitSet_var ws = new DDS::WaitSet;
          DDS::StatusCondition_var sc = dw->get_statuscondition();
          sc->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
          ws->attach_condition(sc);
          DDS::PublicationMatchedStatus matched;
          DDS::ConditionSeq active;
          const DDS::Duration_t timeout = {5, 0}; // seconds
          while (dw->get_publication_matched_status(matched) == DDS::RETCODE_OK
                 && matched.current_count) {
            if (ws->wait(active, timeout) == DDS::RETCODE_TIMEOUT) {
              break;
            }
          }
          ws->detach_condition(sc);
          if (matched.current_count != 0) {
            ACE_DEBUG((LM_DEBUG,
              "ERROR: DataWriter changed deadline period which should "
              "break association with all existing DataReaders, but did not\n"));
            exit(1);
          }
        }

        // We know the reader has been disassociated, but the reader itself may
        // not have been notified yet.  Introducing delay here to let the reader
        // sync up with the disassociated state before re-associating.

        // Wait for reader to finish unmatching.
        FILE* fp = ACE_OS::fopen (synch_fname, ACE_TEXT("r"));
        int i = 0;
        while (fp == 0 &&  i < 15)
        {
          ACE_DEBUG ((LM_DEBUG,
            ACE_TEXT("(%P|%t) waiting reader to unmatch...\n")));
          ACE_OS::sleep (1);
          ++i;
          fp = ACE_OS::fopen (synch_fname, ACE_TEXT("r"));
        }
        if (fp != 0)
          ACE_OS::fclose (fp);

        ACE_DEBUG((LM_DEBUG, "Writer restoring deadline to compatible value\n"));

        // change it back
        dw_qos.deadline.period.sec = 5;

        if (dw->set_qos (dw_qos) != ::DDS::RETCODE_OK)
        {
          ACE_DEBUG((LM_DEBUG,
            "ERROR: DataWriter could not change deadline period which "
            "should restore DataReader associations\n"));
          exit (1);
        }
      }

      {
        // Wait for both second DataReader connect which changed deadline period
        // from 3 seconds to 5 seconds.
        OpenDDS::DCPS::scoped_ptr<Writer> writer (new Writer (dw.in ()));
        attempts = 1;
        while (attempts != max_attempts)
        {
          ::DDS::InstanceHandleSeq handles;
          dw->get_matched_subscriptions(handles);
          if (handles.length() == 2)
            break;
          else
            ACE_OS::sleep(1);
          ++attempts;
        }

        if (attempts == max_attempts)
        {
          ACE_DEBUG((LM_DEBUG, "ERROR: subscriptions failed to match.\n"));
          exit(1);
        }

        writer->start ();
        writer->end ();
      }

      {
        // Wait for subscriber exit.
        attempts = 1;
        while (attempts != max_attempts)
        {
          ::DDS::InstanceHandleSeq handles;

          dw->get_matched_subscriptions(handles);
          if (handles.length() == 0)
            break;
          else
            ACE_OS::sleep(1);

          ++ attempts;
        }

        if (attempts == max_attempts)
        {
          ACE_DEBUG((LM_DEBUG, "ERROR: failed to wait for DataReader exit.\n"));
          exit (1);
        }
      }

      participant->delete_contained_entities();
      dpf->delete_participant(participant.in ());
  }
  catch (CORBA::Exception& e)
  {
    cerr << "PUB: Exception caught in main.cpp:" << endl
         << e << endl;
    exit(1);
  }
  TheServiceParticipant->shutdown ();

  return 0;
}
