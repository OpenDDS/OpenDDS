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
#include <dds/DCPS/Qos_Helper.h>

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include <ace/Time_Value.h>
#include <ace/OS_NS_unistd.h>

using namespace std;

const char PARTITION_A[] = "ZiggieStardust";
const char PARTITION_B[] = "Amadeus";

int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf;
      DDS::DomainParticipant_var participant;

      dpf = TheParticipantFactoryWithArgs(argc, argv);
      participant =
        dpf->create_participant(311,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1 ;
      }

      Messenger::MessageTypeSupportImpl::_var_type mts_servant =
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

      // Create the first subscriber belongs to PARTITION A
      DDS::SubscriberQos sub_qos1;
      participant->get_default_subscriber_qos (sub_qos1);

      sub_qos1.partition.name.length (1);
      sub_qos1.partition.name[0] = PARTITION_A;

      DDS::Subscriber_var sub1 =
        participant->create_subscriber (sub_qos1,
                                        DDS::SubscriberListener::_nil(),
                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub1.in ())) {
        cerr << "Failed to create_subscriber." << endl;
        exit(1);
      }

      // Create the second subscriber belongs to PARTITION B
      DDS::SubscriberQos sub_qos2;
      participant->get_default_subscriber_qos (sub_qos2);

      sub_qos2.partition.name.length (1);
      sub_qos2.partition.name[0] = PARTITION_B;

      DDS::Subscriber_var sub2 =
        participant->create_subscriber (sub_qos2,
                                        DDS::SubscriberListener::_nil(),
                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub2.in ())) {
        cerr << "Failed to create_subscriber." << endl;
        exit(1);
      }

      DDS::DataReaderQos dr_qos;
      sub1->get_default_datareader_qos (dr_qos);
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

      // Create first DataReader with listener.
      DDS::DataReaderListener_var listener1 (new DataReaderListenerImpl);
      if (CORBA::is_nil (listener1.in ()))
      {
        cerr << "ERROR: listener1 is nil." << endl;
        exit(1);
      }

      DDS::DataReader_var dr1 =
        sub1->create_datareader (topic.in (),
                                dr_qos,
                                listener1.in (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      // Create second DataReader with listener.
      DDS::DataReaderListener_var listener2 (new DataReaderListenerImpl);
      if (CORBA::is_nil (listener2.in ()))
      {
        cerr << "ERROR: listener2 is nil." << endl;
        exit(1);
      }

      DDS::DataReader_var dr2 =
        sub2->create_datareader (topic.in (),
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

      if (!listener_servant1) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: listener_servant1 is nil (dynamic_cast failed)!\n")), -1);
      }

      DataReaderListenerImpl* listener_servant2 =
        dynamic_cast<DataReaderListenerImpl*>(listener2.in());

      if (!listener_servant2) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: listener_servant2 is nil (dynamic_cast failed)!\n")), -1);
      }

      int expected = 10;
      cout << "subscriber waiting for partition A completion" << endl;
      // Writer of PARTITION A -> Reader of PARTITION A
      while ( listener_servant1->num_reads() < expected) {
        ACE_OS::sleep (1);
      }

      cout << "subscriber waiting for partition B completion" << endl;
      // Writer switch from PARTITION A to B -> Reader of PARTITION B
      while ( listener_servant2->num_reads() < expected) {
        ACE_OS::sleep (1);
      }

      // ----------------------------------------------
      // Now switch first reader/subscriber from A to B
      // and it should be connected with DataWriter.
      cout << "subscriber switching from A to B partition" << endl;
      sub_qos1.partition.name[0] = PARTITION_B;

      if (sub1->set_qos (sub_qos1) != ::DDS::RETCODE_OK)
      {
        cerr << "ERROR: failed to set partition" << endl;
        exit (1);
      }

      // Continue receive 10 more messages each.
      expected = 20;

      cout << "subscriber waiting for 2nd partition B completion" << endl;
      while ( listener_servant1->num_reads() < expected) {
        ACE_OS::sleep (1);
      }

      while ( listener_servant2->num_reads() < expected) {
        ACE_OS::sleep (1);
      }

      if (listener_servant1->num_reads() > expected
        || listener_servant2->num_reads() > expected)
      {
        cerr << "ERROR: received more (" << listener_servant1->num_reads() << " + " << listener_servant2->num_reads() << ") than expected messages: (" << expected << " + " << expected << ")" << endl;
        exit (1);
      }

      // Now shutdown
      if (!CORBA::is_nil (participant.in ())) {
        participant->delete_contained_entities();
      }
      if (!CORBA::is_nil (dpf.in ())) {
        dpf->delete_participant(participant.in ());
      }

    }
  catch (CORBA::Exception& e)
    {
      cerr << "SUB: Exception caught in main ():" << endl << e << endl;
      return 1;
    }
  TheServiceParticipant->shutdown ();

  return 0;
}
