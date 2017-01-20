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
#include "DataWriterListenerImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include "dds/DCPS/StaticIncludes.h"
#include "dds/DCPS/scoped_ptr.h"

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"

#include <memory>

using namespace Messenger;
using namespace std;

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);
      DDS::DomainParticipant_var participant =
        dpf->create_participant(411,
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
        participant->create_publisher (PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ()))
      {
        cerr << "create_publisher failed." << endl;
        exit(1);
      }

      // Create the datawriter
      DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);
      dw_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
      dw_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
      dw_qos.resource_limits.max_samples_per_instance = 1000;
      dw_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;
      dw_qos.lifespan.duration.sec = 20;
      dw_qos.lifespan.duration.nanosec = 0;


      ::DDS::DataWriterListener_var dwl (new DataWriterListenerImpl);

      DDS::DataWriter_var dw =
        pub->create_datawriter (topic.in (),
                                dw_qos,
                                dwl.in(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }

      {
        OpenDDS::DCPS::scoped_ptr<Writer> writer (new Writer (dw.in ()));

        writer->start ();
        while ( !writer->is_finished())
        {
          ACE_Time_Value small_time(30,0);
          ACE_OS::sleep (small_time);
        }

        // Cleanup
        writer->end ();
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

  return 0;
}
