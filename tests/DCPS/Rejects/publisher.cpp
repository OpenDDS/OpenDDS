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
#include <assert.h>

using namespace Messenger;
using namespace std;


const int MAX_INSTANCES = 2;
const int MAX_SAMPLES = 7;
const int MAX_SAMPLES_PER_INSTANCES = 4;

static ACE_Time_Value SLEEP_DURATION(ACE_Time_Value (1));

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]){
  int result = 0;
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
                                      DDS::PublisherListener::_nil(),
                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ())) {
        cerr << "create_publisher failed." << endl;
        exit(1);
      }

      // ----------------------------------------------

      DDS::DataWriterQos dw_qos; // Good QoS.
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.resource_limits.max_samples_per_instance = MAX_SAMPLES_PER_INSTANCES;
      dw_qos.resource_limits.max_samples = MAX_SAMPLES;
      dw_qos.resource_limits.max_instances = MAX_INSTANCES;
#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
      dw_qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      dw_qos.history.depth = MAX_SAMPLES_PER_INSTANCES;
#endif

      DDS::DataWriter_var dw =
        pub->create_datawriter (topic.in (),
                                dw_qos,
                                DDS::DataWriterListener::_nil (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dw.in ()))
      {
        cerr << "ERROR: create_datawriter failed." << endl;
        exit(1);
      }

      DDS::DataWriter_var dw2 =
        pub->create_datawriter (topic.in (),
                                dw_qos,
                                DDS::DataWriterListener::_nil (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dw2.in ()))
      {
        cerr << "ERROR: create_datawriter failed." << endl;
        exit(1);
      }

      {
        // Two threads use same datawriter to write different instances.
        OpenDDS::DCPS::scoped_ptr<Writer> writer1 (new Writer (dw.in (), 99, SLEEP_DURATION));
        OpenDDS::DCPS::scoped_ptr<Writer> writer2 (new Writer (dw.in (), 100, SLEEP_DURATION));
        OpenDDS::DCPS::scoped_ptr<Writer> writer3 (new Writer (dw.in (), 101, SLEEP_DURATION));
        OpenDDS::DCPS::scoped_ptr<Writer> writer4 (new Writer (dw2.in (), 101, SLEEP_DURATION));

        writer1->start ();
        writer2->start ();

        // ----------------------------------------------

        // Wait for first writer threads to register with DataReaders.
        if (writer1->wait_for_registered () == false
            || writer2->wait_for_registered () == false)
        {
          cerr << "ERROR: first writers took too long to connect. " << endl;
          exit (1);
        }
        writer3->start ();
        if (writer3->wait_for_registered () == false)
        {
          cerr << "ERROR: writer 3 took too long to connect. " << endl;
          exit (1);
        }
        writer4->start ();
        if (writer4->wait_for_registered () == false)
        {
          cerr << "ERROR: writer 4 took too long to connect. " << endl;
          exit (1);
        }

        writer1->start_sending ();
        writer2->start_sending ();
        writer3->start_sending ();
        writer4->start_sending ();

        writer1->wait ();
        writer2->wait ();
        writer3->wait ();
        writer4->wait ();

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

        if (writer1->failed_registration() ) {
          cerr << "ERROR: unexpected failed registration for writer 1. " << endl;
          result = 1;
        }
        if (writer2->failed_registration() ) {
          cerr << "ERROR: unexpected failed registration for writer 2. " << endl;
          result = 1;
        }
        if (!writer3->failed_registration() ) {
          cerr << "ERROR: unexpected registration for writer 3. " << endl;
          result = 1;
        }
        if (writer4->failed_registration() ) {
          cerr << "ERROR: unexpected failed registration for writer 4. " << endl;
          result = 1;
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

  return result;
}
