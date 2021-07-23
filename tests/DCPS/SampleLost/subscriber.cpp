// -*- C++ -*-

// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 */
// ============================================================================


#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>

#include "dds/DCPS/StaticIncludes.h"
#include "ace/OS_NS_unistd.h"

#include <iostream>

using namespace std;

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  int retval = 0;
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);

      DDS::DomainParticipant_var participant =
        dpf->create_participant (111,
                                 PARTICIPANT_QOS_DEFAULT,
                                 DDS::DomainParticipantListener::_nil(),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ()))
      {
        cerr << "create_participant failed." << endl;
        return 1 ;
      }

      Messenger::MessageTypeSupport_var mts_servant =
        new Messenger::MessageTypeSupportImpl;

      if (DDS::RETCODE_OK != mts_servant->register_type(participant.in (),
                                                        ""))
      {
        cerr << "Failed to register the MessageTypeTypeSupport." << endl;
        exit(1);
      }

      CORBA::String_var type_name = mts_servant->get_type_name ();

      DDS::TopicQos topic_qos;
      participant->get_default_topic_qos(topic_qos);
      DDS::Topic_var topic =
        participant->create_topic ("Movie Discussion List",
                                   type_name.in (),
                                   topic_qos,
                                   DDS::TopicListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ()))
      {
        cerr << "Failed to create_topic." << endl;
        exit(1);
      }

      // Create the subscriber
      DDS::Subscriber_var sub =
        participant->create_subscriber (SUBSCRIBER_QOS_DEFAULT,
                                        DDS::SubscriberListener::_nil(),
                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub.in ()))
      {
        cerr << "Failed to create_subscriber." << endl;
        exit(1);
      }

      // activate the listener
      DDS::DataReaderListener_var listener (new DataReaderListenerImpl);
      DataReaderListenerImpl* const listener_servant =
        dynamic_cast<DataReaderListenerImpl*>(listener.in());

      if (CORBA::is_nil (listener.in ()))
      {
        cerr << "listener is nil." << endl;
        exit(1);
      }
      if (!listener_servant) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: listener_servant is nil (dynamic_cast failed)!\n")), -1);
      }

      // Create the Datareaders
      DDS::DataReaderQos dr_qos;
      DDS::Duration_t deadline_time = {3, 0};
      sub->get_default_datareader_qos (dr_qos);
      dr_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
      dr_qos.history.kind  = ::DDS::KEEP_LAST_HISTORY_QOS;
      dr_qos.history.depth = 4;
      dr_qos.deadline.period = deadline_time;

      DDS::DataReader_var dr = sub->create_datareader (topic.in (),
                                                       dr_qos,
                                                       listener.in (),
                                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dr.in ())) {
        cerr << "create_datareader failed." << endl;
        exit(1);
      }

      int const expected = 10;
      while (listener_servant->num_data_available() < expected) {
        cerr << "Got " << listener_servant->num_data_available() << " number of data available callbacks" << endl;
        ACE_OS::sleep (1);
      }

      cerr << "Got " << listener_servant->num_samples_lost() << " sample lost callbacks" << endl;
      cerr << "Got " << listener_servant->num_samples_rejected() << " sample rejected callbacks" << endl;
      if (listener_servant->num_budget_exceeded() > 0) {
        cerr << "ERROR: Got " << listener_servant->num_budget_exceeded() << " budget exceeded callbacks" << endl;
        ++retval;
      }
      if (listener_servant->num_samples_lost() == 0) {
        cerr << "ERROR: Got no sample lost callbacks" << endl;
        ++retval;
      }

      if (!CORBA::is_nil (participant.in ())) {
        participant->delete_contained_entities();
      }
      if (!CORBA::is_nil (dpf.in ())) {
        dpf->delete_participant(participant.in ());
      }
      ACE_OS::sleep(2);

      TheServiceParticipant->shutdown ();
    }
  catch (CORBA::Exception& e)
    {
      cerr << "SUB: Exception caught in main ():" << endl << e << endl;
      return 1;
    }

  return retval;
}
