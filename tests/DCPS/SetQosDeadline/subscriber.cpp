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
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/tcp/TcpConfiguration.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include <ace/streams.h>
#include <ace/Time_Value.h>

#include <cassert>

OpenDDS::DCPS::TransportIdType transport_impl_id = 1;

int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf;
      DDS::DomainParticipant_var participant;

      dpf = TheParticipantFactoryWithArgs(argc, argv);
      participant =
        dpf->create_participant(411,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1 ;
      }

      Messenger::MessageTypeSupportImpl* mts_servant =
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
        participant->create_topic("Movie Discussion List",
                                  type_name.in (),
                                  topic_qos,
                                  DDS::TopicListener::_nil(),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ())) {
        cerr << "Failed to create_topic." << endl;
        exit(1);
      }

      // Initialize the transport
      OpenDDS::DCPS::TransportImpl_rch tcp_impl =
        TheTransportFactory->create_transport_impl (
          transport_impl_id,
          ::OpenDDS::DCPS::AUTO_CONFIG);

      // Create the subscriber and attach to the corresponding
      // transport.
      DDS::Subscriber_var sub =
        participant->create_subscriber (SUBSCRIBER_QOS_DEFAULT,
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

      OpenDDS::DCPS::AttachStatus const status =
        sub_impl->attach_transport(tcp_impl.in());
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


      DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      // Set up a 5 second recurring deadline.
      dr_qos.deadline.period.sec     = 5;
      dr_qos.deadline.period.nanosec = 0;

      // Create two listeners. One for each DataReader.
      DDS::DataReaderListener_var listener1 (new DataReaderListenerImpl);
      if (CORBA::is_nil (listener1.in ()))
      {
        cerr << "ERROR: listener1 is nil." << endl;
        exit(1);
      }

      DDS::DataReaderListener_var listener2 (new DataReaderListenerImpl);
      if (CORBA::is_nil (listener2.in ()))
      {
        cerr << "ERROR: listener2 is nil." << endl;
        exit(1);
      }


      // First data reader has 5 second deadline period.
      DDS::DataReader_var dr1 =
        sub->create_datareader (topic.in (),
                                dr_qos,
                                listener1.in (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      // Set up a 3 second recurring deadline.
      dr_qos.deadline.period.sec     = 3;
      dr_qos.deadline.period.nanosec = 0;

      // Second data reader has 3 second deadline period which
      // is not compatible with DataWriter.
      DDS::DataReader_var dr2 =
        sub->create_datareader (topic.in (),
                                dr_qos,
                                listener2.in (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dr1.in ()) || CORBA::is_nil (dr2.in ()))
      {
        cerr << "ERROR: create_datareader failed." << endl;
        exit(1);
      }

      DataReaderListenerImpl* listener_servant1 =
        dynamic_cast<DataReaderListenerImpl*>(listener1.in());
      DataReaderListenerImpl* listener_servant2 =
        dynamic_cast<DataReaderListenerImpl*>(listener2.in());

      int expected = 10;
      // Writer of deadline 4 -> Reader of deadline 5
      while ( listener_servant1->num_reads() < expected) {
        ACE_OS::sleep (1);
      }

      // Writer of deadline 4 and Reader of deadline 3 is not
      // compatible so second DataReader should not receive
      // any message from DataWriter.
      if (listener_servant2->num_reads() > 0)
      {
        cerr << "ERROR: second DataReader should not receive message from "
          << "datawriter as their deadline QoS is not compatible" << endl;
        exit (1);
      }

      // Now change second DataReader to have deadline period to be 10 seconds. This
      // value is compatible with DataWriter so it was accepted.
      dr_qos.deadline.period.sec = 5;

      if (dr2->set_qos (dr_qos) != ::DDS::RETCODE_OK)
      {
        cerr << "ERROR: DataReader changed deadline period to make it compatible "
          << "with datawriter" << endl;
        exit (1);
      }

      // second DataReader should receive 20 messages so far.
      while ( listener_servant1->num_reads() < 2 * expected) {
        ACE_OS::sleep (1);
      }

      // second DataReader should receive 10 messages.
      while ( listener_servant2->num_reads() < expected) {
        ACE_OS::sleep (1);
      }

      // During this period, the 5 second should have at most 1 missed
      // deadline, but with 3 seconds, it should have at least 2 missed
      // deadline.
      ACE_OS::sleep (9);

      if (listener_servant2->num_deadline_missed () > 1)
      {
        cerr << "ERROR: failed to verify deadline missed count " << endl;
        exit (1);
      }

      if (!CORBA::is_nil (participant.in ())) {
        participant->delete_contained_entities();
      }
      if (!CORBA::is_nil (dpf.in ())) {
        dpf->delete_participant(participant.in ());
      }

      TheTransportFactory->release();
      TheServiceParticipant->shutdown ();
    }
  catch (CORBA::Exception& e)
    {
      cerr << "SUB: Exception caught in main ():" << endl << e << endl;
      return 1;
    }

  return 0;
}
