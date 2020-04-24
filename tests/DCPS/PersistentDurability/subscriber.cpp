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

      // Create the subscriber and attach to the corresponding
      // transport.
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

      // Create the Datareader
      DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos(dr_qos);
      dr_qos.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;

      printf("--sub------- Call create_datareader()\n");
      fflush(stdout);
      DDS::DataReader_var dr =
        sub->create_datareader(topic, dr_qos, listener,
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dr.in ())) {
        cerr << "create_datareader failed." << endl;
        exit(1);
      }

      printf("--sub------- Wait for 12 msgs\n");
      fflush(stdout);
      for (int i_expected = 12;  listener_servant->num_reads() < i_expected;  /*empty*/)
      {
        ACE_OS::sleep (1);
      }

      printf("--sub------- Wait for 4 secs\n");
      fflush(stdout);
      for (int i_max_secs = 4;  i_max_secs > 0;  --i_max_secs)
      {
        ACE_OS::sleep (1);
      }

      printf("--sub------- Delete and shutdown ...\n");
      fflush(stdout);
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

  return 0;
}
