// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

#include <ace/streams.h>
#include "ace/Get_Opt.h"

using namespace Messenger;

OpenDDS::DCPS::TransportIdType transport_impl_id = 1;
long num_expected_dispose = 0;
long num_expected_unregister = 0;
long num_expected_data = 10;

int
parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("du"));
  int c;

  while ((c = get_opts ()) != -1)
  {
    switch (c)
    {
    case 'd':
      num_expected_dispose = 1;
      break;
    case 'u':
      //If writer calls unregister_instance, do not expect 
      //the unregister message because the reader defaults to
      //auto dispose upon unregister, it will send dispose which
      //cause the instance release and state not change so 
      //unregister instance message is ignored.
      num_expected_unregister = 0;
      ++ num_expected_dispose;
      break;
    case '?':
    default:
      ACE_ERROR_RETURN ((LM_ERROR,
        "usage:  %s "
        "-d -u"
        "\n"
        "-d for dispose notification test and -u for unregister notification test \n",
        argv [0]),
        -1);
    }
  }
  // Indicates sucessful parsing of the command line
  return 0;
}


int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
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

      if (parse_args (argc, argv) == -1) {
        return -1;
      }

      MessageTypeSupport_var mts = new MessageTypeSupportImpl();

      if (DDS::RETCODE_OK != mts->register_type(participant.in (), "")) {
          cerr << "Failed to register the MessageTypeTypeSupport." << endl;
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

      // Initialize the transport
      OpenDDS::DCPS::TransportImpl_rch transport_impl =
        TheTransportFactory->create_transport_impl (transport_impl_id,
                                                    ::OpenDDS::DCPS::AUTO_CONFIG);

      // Create the subscriber and attach to the corresponding
      // transport.
      DDS::Subscriber_var sub =
        participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                       DDS::SubscriberListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub.in ())) {
        cerr << "Failed to create_subscriber." << endl;
        exit(1);
      }

      // Attach the subscriber to the transport.
      OpenDDS::DCPS::SubscriberImpl* sub_impl =
        dynamic_cast<OpenDDS::DCPS::SubscriberImpl*> (sub.in ());
      if (0 == sub_impl) {
        cerr << "Failed to obtain subscriber servant\n" << endl;
        exit(1);
      }

      OpenDDS::DCPS::AttachStatus status = sub_impl->attach_transport(transport_impl.in());
      if (status != OpenDDS::DCPS::ATTACH_OK) {
        std::string status_str;
        switch (status) {
        case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
          status_str = "ATTACH_BAD_TRANSPORT";
          break;
        case OpenDDS::DCPS::ATTACH_ERROR:
          status_str = "ATTACH_ERROR";
          break;
        case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
          status_str = "ATTACH_INCOMPATIBLE_QOS";
          break;
        default:
          status_str = "Unknown Status";
          break;
        }
        cerr << "Failed to attach to the transport. Status == "
          << status_str.c_str() << endl;
        exit(1);
      }

      // activate the listener
      DDS::DataReaderListener_var listener = new DataReaderListenerImpl;
      DataReaderListenerImpl &listener_servant =
        *dynamic_cast<DataReaderListenerImpl*>(listener.in());

      if (CORBA::is_nil (listener.in ())) {
        cerr << "listener is nil." << endl;
        exit(1);
      }

      // Create the Datareaders
      DDS::DataReader_var dr = sub->create_datareader(topic.in (),
                                                      DATAREADER_QOS_DEFAULT,
                                                      listener.in (),
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dr.in ())) {
        cerr << "create_datareader failed." << endl;
        exit(1);
      }


      int expected = num_expected_data + num_expected_dispose + num_expected_unregister;
cerr << " expected " <<  num_expected_data << "/" << num_expected_dispose << "/" << num_expected_unregister <<endl;
           
      while ( listener_servant.num_reads() < expected ) { 
        cerr << " recv " << listener_servant.num_reads() << endl;
        ACE_OS::sleep (1);
      }

      listener_servant.stop ();

      if (!CORBA::is_nil (participant.in ())) {
        participant->delete_contained_entities();
      }
      if (!CORBA::is_nil (dpf.in ())) {
        dpf->delete_participant(participant.in ());
      }
      ACE_OS::sleep(2);

      TheTransportFactory->release();
      TheServiceParticipant->shutdown ();

      if (listener_servant.num_received_dispose () != num_expected_dispose)
      {
         cerr << "did not receive dispose sample as expected." << num_expected_dispose 
           << "/" << listener_servant.num_received_dispose () << endl;
         return 1;
      }
      if (listener_servant.num_received_unregister () != num_expected_unregister)
      {
         cerr << "did not receive unregister sample as expected." << num_expected_unregister 
           << "/" << listener_servant.num_received_unregister () << endl;
         return 1;
      }

    }
  catch (CORBA::Exception& e)
    {
      cerr << "SUB: Exception caught in main ():" << endl << e << endl;
      return 1;
    }

  return 0;
}
