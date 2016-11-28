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
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/StaticIncludes.h"

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"
#include "ace/OS_NS_unistd.h"

using namespace std;

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  long expected_late = 0;
  try
    {
      DDS::DomainParticipantFactory_var dpf;
      DDS::DomainParticipant_var participant;

      dpf = TheParticipantFactoryWithArgs(argc, argv);
      participant = dpf->create_participant(411,
                                            PARTICIPANT_QOS_DEFAULT,
                                            DDS::DomainParticipantListener::_nil(),
                                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1 ;
      }

      ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("l:"));
      int c;

      while ((c = get_opts ()) != -1)
      {
        switch(c)
        {
        case 'l':
          expected_late = ACE_OS::atoi (get_opts.opt_arg ());
          break;
        case '?':
        default:
          ACE_ERROR_RETURN ((LM_ERROR,
            "usage:  %s "
            "-l expected late samples "
            "\n",
            argv [0]),
            -1);
        }
      }

      Messenger::MessageTypeSupportImpl* mts_servant =
        new Messenger::MessageTypeSupportImpl;

      if (DDS::RETCODE_OK != mts_servant->register_type (participant.in (),
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
        participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
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

      if (CORBA::is_nil (listener.in ())) {
        cerr << "listener is nil." << endl;
        exit(1);
      }

      // Create the Datareaders
      DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);
      dr_qos.latency_budget.duration.sec = 1;
      dr_qos.latency_budget.duration.nanosec = 0;
      DDS::DataReader_var dr = sub->create_datareader(topic.in (),
                                                      dr_qos,
                                                      listener.in (),
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dr.in ())) {
        cerr << "create_datareader failed." << endl;
        exit(1);
      }

      dynamic_cast<OpenDDS::DCPS::DataReaderImpl*> (dr.in ())->
        reset_latency_stats ();
      dynamic_cast<OpenDDS::DCPS::DataReaderImpl*> (dr.in ())->
        statistics_enabled (true);

      ACE_OS::sleep (10);
      if (listener_servant->num_reads () != 10)
        {
          cerr << "ERROR: Incorrect number of samples received." << endl
               << "       Expired data was probably read." << endl;
          exit (1);
        }

      if (listener_servant->num_late () != expected_late)
        {
          cerr << "ERROR: Incorrect number of samples received late." << endl;
          exit (1);
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

  return 0;
}
