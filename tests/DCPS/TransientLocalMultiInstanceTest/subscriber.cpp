// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 */
 // ============================================================================


#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include "dds/DCPS/StaticIncludes.h"

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
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_participant failed.\n")));
      return 1;
    }

    DDS::TypeSupport_var ts = new Messenger::MessageTypeSupportImpl;

    if (DDS::RETCODE_OK != ts->register_type(participant, "")) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Failed to register the MessageTypeTypeSupport.\n")));
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Failed to create_topic.\n")));
      exit(1);
    }

    // Create the subscriber
    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
        DDS::SubscriberListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(sub)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Failed to create_subscriber.\n")));
      exit(1);
    }

    // activate the listener
    DDS::DataReaderListener_var listener(new DataReaderListenerImpl);
    DataReaderListenerImpl* listener_servant =
      dynamic_cast<DataReaderListenerImpl*>(listener.in());

    if (CORBA::is_nil(listener)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) listener is nil.\n")));
      exit(1);
    }
    if (!listener_servant) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: listener_servant is nil (dynamic_cast failed)!\n")), -1);
    }

    // Create the Datareaders
    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    DDS::DataReader_var dr2 = sub->create_datareader(topic,
      dr_qos,
      listener,
      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(dr2)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_datareader durable failed.\n")));
      exit(1);
    }
    else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) create_datareader durable success.\n")));
    }

    const int expected = 4;
    while (listener_servant->num_reads() < expected || !listener_servant->received_all_expected_messages()) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) listener current has read: %d samples. (waiting for %d)\n"), listener_servant->num_reads(), expected));
      ACE_OS::sleep(1);
    }

    ok = listener_servant->ok();
    if (ok)
    {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Reader received all samples.\n")));
    }
    else
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: failed to receive expected number of samples\n")));
    }

    if (!CORBA::is_nil(participant)) {
      participant->delete_contained_entities();
    }

    if (!CORBA::is_nil(dpf)) {
      dpf->delete_participant(participant);
    }

    TheServiceParticipant->shutdown();

  }
  catch (CORBA::Exception& e) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) SUB: Exception caught in main(): %C\n"), e._info().c_str()));
    return 1;
  }

  return ok ? 0 : 1;
}
