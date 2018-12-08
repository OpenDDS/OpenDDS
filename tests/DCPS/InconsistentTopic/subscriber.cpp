// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 */
// ============================================================================


#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"
#include "ace/OS_NS_unistd.h"

using namespace Messenger;
using namespace std;

int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf;
      DDS::DomainParticipant_var participant;

      dpf = TheParticipantFactoryWithArgs(argc, argv);
      participant = dpf->create_participant(111,
                                            PARTICIPANT_QOS_DEFAULT,
                                            DDS::DomainParticipantListener::_nil(),
                                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1 ;
      }

      Message1TypeSupport_var mts = new Message1TypeSupportImpl();

      if (DDS::RETCODE_OK != mts->register_type(participant.in (), "")) {
          cerr << "Failed to register the Message1TypeTypeSupport." << endl;
          exit(1);
        }

      CORBA::String_var type_name = mts->get_type_name ();

      DDS::Topic_var topic = participant->create_topic("Movie Discussion List",
                                                       type_name.in (),
                                                       TOPIC_QOS_DEFAULT,
                                                       DDS::TopicListener::_nil(),
                                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ())) {
        cerr << "Failed to create_topic." << endl;
        exit(1);
      }

      // Create the subscriber
      DDS::Subscriber_var sub =
        participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                       DDS::SubscriberListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub.in ())) {
        cerr << "Failed to create_subscriber." << endl;
        exit(1);
      }

      // Create the Datareaders
      DDS::DataReader_var dr = sub->create_datareader(topic.in (),
                                                      DATAREADER_QOS_DEFAULT,
                                                      DDS::DataReaderListener::_nil(),
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dr.in ())) {
        cerr << "create_datareader failed." << endl;
        exit(1);
      }

      // Try to get the inconsistent_topic_status once a second for a number of seconds, when we got it
      // we can stop getting it
      DDS::InconsistentTopicStatus status;
      for (size_t i = 0; i < 10; ++i) {
        DDS::Duration_t timeout;
        timeout.sec = 0;
        timeout.nanosec = 0;
        DDS::Topic_var topic2 = participant->find_topic ("Movie Discussion List", timeout);
        if (CORBA::is_nil (topic2.in ())) {
          cerr << "not able to find topic." << endl;
          exit(1);
        }

        DDS::ReturnCode_t const retcode = topic2->get_inconsistent_topic_status (status);

        if (retcode != DDS::RETCODE_OK) {
          cerr << "not able to retrieve topic status." << endl;
          exit(1);
        }

        if (status.total_count_change != 0) {
          break;
        }

        ACE_OS::sleep (1);
      }

      if (status.total_count_change != 0) {
        cerr << "subscriber should have an inconsistent topic total changed count not equal zero." << endl;
        exit(1);
      } else {
        cout << "subscriber total count: " << status.total_count << endl;
      }

      ACE_OS::sleep (1);

      if (!CORBA::is_nil (participant.in ())) {
        participant->delete_contained_entities();
      }
      if (!CORBA::is_nil (dpf.in ())) {
        dpf->delete_participant(participant.in ());
      }

      TheServiceParticipant->shutdown ();
    }
  catch (CORBA::Exception& e)
    {
      cerr << "SUB: Exception caught in main ():" << endl << e << endl;
      return 1;
    }

  return 0;
}
