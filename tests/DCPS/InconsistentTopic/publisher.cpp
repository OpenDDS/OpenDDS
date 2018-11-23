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
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include "dds/DCPS/StaticIncludes.h"

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"
#include "ace/OS_NS_unistd.h"

using namespace Messenger;
using namespace std;


int ACE_TMAIN (int argc, ACE_TCHAR *argv[]) {
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);
      DDS::DomainParticipant_var participant =
        dpf->create_participant(111,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1;
      }

      Message2TypeSupport_var mts = new Message2TypeSupportImpl();

      if (DDS::RETCODE_OK != mts->register_type(participant.in (), "")) {
        cerr << "register_type failed." << endl;
        exit(1);
      }

      CORBA::String_var type_name = mts->get_type_name ();

      DDS::Topic_var topic =
        participant->create_topic ("Movie Discussion List",
                                   type_name.in (),
                                   TOPIC_QOS_DEFAULT,
                                   DDS::TopicListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ())) {
        cerr << "create_topic failed." << endl;
        exit(1);
      }

      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil(), ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ())) {
        cerr << "create_publisher failed." << endl;
        exit(1);
      }

      // Create the datawriter
      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in (),
                               DATAWRITER_QOS_DEFAULT,
                               DDS::DataWriterListener::_nil(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dw.in ())) {
        cerr << "create_datawriter failed." << endl;
        exit(1);
      }

      ACE_OS::sleep (5);

      DDS::Duration_t timeout;
      timeout.sec = 0;
      timeout.nanosec = 0;
      DDS::Topic_var topic2 = participant->find_topic ("Movie Discussion List", timeout);
      if (CORBA::is_nil (topic2.in ())) {
        cerr << "not able to find topic." << endl;
        exit(1);
      }

      DDS::InconsistentTopicStatus status;
      DDS::ReturnCode_t const retcode = topic2->get_inconsistent_topic_status (status);

      if (retcode != DDS::RETCODE_OK) {
        cerr << "not able to retrieve topic status." << endl;
        exit(1);
      }

      if (status.total_count == 0) {
        cerr << "should have an inconsistent topic total count not equal zero." << endl;
        exit(1);
      } else {
        cout << "publisher total count: " << status.total_count << endl;
      }

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
