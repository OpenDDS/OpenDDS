// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *
 *
 */
// ============================================================================


#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>

#include "dds/DCPS/StaticIncludes.h"

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"
#include "ace/OS_NS_unistd.h"

using namespace std;

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  bool ok = true;
  try {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    DDS::DomainParticipant_var participant =
      dpf->create_participant(4,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(participant)) {
      cerr << "create_participant failed." << endl;
      return 1 ;
    }

    DDS::TypeSupport_var ts = new Messenger::MessageTypeSupportImpl;

    if (DDS::RETCODE_OK != ts->register_type(participant, "")) {
        cerr << "Failed to register the MessageTypeTypeSupport." << endl;
        exit(1);
      }

    CORBA::String_var type_name = ts->get_type_name();

    DDS::TopicQos topic_qos;
    participant->get_default_topic_qos(topic_qos);
    DDS::Topic_var topic = participant->create_topic("Movie Discussion List TLT",
                                                     type_name,
                                                     topic_qos,
                                                     DDS::TopicListener::_nil(),
                                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(topic)) {
      cerr << "Failed to create_topic." << endl;
      exit(1);
    }

    // Create the subscriber
    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     DDS::SubscriberListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(sub)) {
      cerr << "Failed to create_subscriber." << endl;
      exit(1);
    }

    // activate the listener
    DDS::DataReaderListener_var listener(new DataReaderListenerImpl);
    DataReaderListenerImpl* listener_servant =
      dynamic_cast<DataReaderListenerImpl*>(listener.in());

    if (CORBA::is_nil(listener)) {
      cerr << "listener is nil." << endl;
      exit(1);
    }

    // Create the Datareaders
    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    DDS::DataReader_var dr = sub->create_datareader(topic,
                                                    dr_qos,
                                                    listener,
                                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(dr)) {
      cerr << "create_datareader volatile failed." << endl;
      exit(1);
    } else {
      cerr << "create_datareader volatile success." << endl;
    }

    dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    DDS::DataReader_var dr2 = sub->create_datareader(topic,
                                                     dr_qos,
                                                     listener,
                                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(dr2)) {
      cerr << "create_datareader durable failed." << endl;
      exit(1);
    } else {
      cerr << "create_datareader durable success." << endl;
    }

    const int expected = 50;
    while (listener_servant->num_reads() < expected) {
      ACE_OS::sleep(1);
    }

    ok = listener_servant->ok_;

    if (!CORBA::is_nil(participant)) {
      participant->delete_contained_entities();
    }

    if (!CORBA::is_nil (dpf)) {
      dpf->delete_participant(participant);
    }

    TheServiceParticipant->shutdown();

  } catch (CORBA::Exception& e) {
    cerr << "SUB: Exception caught in main():" << endl << e << endl;
    return 1;
  }

  return ok ? 0 : 1;
}
