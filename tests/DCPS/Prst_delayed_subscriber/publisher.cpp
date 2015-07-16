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
#include "dds/DCPS/StaticIncludes.h"

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"

using namespace Messenger;
using namespace std;

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]) {
  try
    {
      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("initialization starting.\n")
        ));
      }

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

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("participant created.\n")
        ));
      }

      MessageTypeSupportImpl* servant = new MessageTypeSupportImpl;
      Messenger::MessageTypeSupport_var mts = servant;

      if (DDS::RETCODE_OK != servant->register_type(participant, "")) {
        cerr << "register_type failed." << endl;
        exit(1);
      }

      CORBA::String_var type_name = servant->get_type_name();

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("type support registered.\n")
        ));
      }

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

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("topic created.\n")
        ));
      }

      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ())) {
        cerr << "create_publisher failed." << endl;
        exit(1);
      }

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("publisher created.\n")
        ));
      }

      // Create the datawriter
      DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);
      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in (),
                               dw_qos,
                               DDS::DataWriterListener::_nil(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("datawriter created.\n")
        ));
      }

      size_t msg_cnt = 5;
      size_t sub_cnt = 2;
      Writer* writer = new Writer(dw.in(), msg_cnt, sub_cnt);

      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher: ")
          ACE_TEXT("processing starting.\n")
        ));
      }

      writer->start ();
      while ( !writer->is_finished()) {
        ACE_Time_Value small_time(0,250000);
        ACE_OS::sleep (small_time);
      }

      // Cleanup
      writer->end ();
      delete writer;
      participant->delete_contained_entities();
      dpf->delete_participant(participant);
      TheServiceParticipant->shutdown();
  }
  catch (CORBA::Exception& e)
    {
       cerr << "PUB: Exception caught in main.cpp:" << endl
         << e << endl;
      exit(1);
    }

  return 0;
}
