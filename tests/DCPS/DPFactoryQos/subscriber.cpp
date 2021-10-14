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
#include "ace/Get_Opt.h"
#include "ace/OS_NS_unistd.h"

using namespace Messenger;
using namespace std;
using namespace OpenDDS::DCPS;

int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf;
      DDS::DomainParticipant_var participant;

      dpf = TheParticipantFactoryWithArgs(argc, argv);

      // Default DomainParticipantFactory qos is to auto enable.
      ::DDS::DomainParticipantFactoryQos fqos;
      if (dpf->get_qos (fqos) != ::DDS::RETCODE_OK)
      {
        cerr << "DomainParticipantFactory get_qos failed." << endl;
        return 1;
      }

      if (!fqos.entity_factory.autoenable_created_entities)
      {
        cerr << "The DomainParticipantFactory defaults to autoenable upon entities creation." << endl;
        return 1;
      }

      // Now disable DomainParticipantFactory autoenable
      fqos.entity_factory.autoenable_created_entities = false;
      if (dpf->set_qos (fqos) != ::DDS::RETCODE_OK)
      {
        cerr << "DomainParticipantFactory set_qos failed." << endl;
        return 1;
      }

      participant = dpf->create_participant(111,
                                            PARTICIPANT_QOS_DEFAULT,
                                            DDS::DomainParticipantListener::_nil(),
                                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1 ;
      }

      DDS::DomainParticipantQos dpqos;
      if (participant->get_qos (dpqos) != ::DDS::RETCODE_OK)
      {
        cerr << "DomainParticipant get_qos failed." << endl;
        return 1;
      }

      // The QoS of the dp shouldn't match the marked value which has a magic value
      if (dpqos == PARTICIPANT_QOS_DEFAULT)
      {
        cerr << "ERROR: DomainParticipant QoS matches marked PARTICIPANT_QOS_DEFAULT." << endl;
        return 1;
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

      if (topic->enable () != ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        cerr << "Topic can not be enabled because DomainParticipant is not enabled." << endl;
        return 1;
      }

      // Create the subscriber and attach to the corresponding
      // transport.
      DDS::SubscriberQos sub_qos;
      participant->get_default_subscriber_qos(sub_qos);
      sub_qos.entity_factory.autoenable_created_entities = false;
      DDS::Subscriber_var sub =
        participant->create_subscriber(sub_qos,
                                       DDS::SubscriberListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub.in ())) {
        cerr << "Failed to create_subscriber." << endl;
        exit(1);
      }

      if (sub->enable () != ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        cerr << "Subscriber can not be enabled because DomainParticipant is not enabled." << endl;
        return 1;
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

      DDS::DataReaderQos dr_qos;
      if (dr->get_qos(dr_qos) != ::DDS::RETCODE_OK)
      {
        cerr << "DataReader failed to get_qos when not enabled." << endl;
        return 1;
      }

      dr_qos.user_data.value.length(1);
      dr_qos.user_data.value[0] = 0x42;

      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

      if (dr->set_qos(dr_qos) != ::DDS::RETCODE_OK)
      {
        cerr << "DataReader failed to set_qos when not enabled." << endl;
        return 1;
      }

      if (dr->enable () != ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        cerr << "DataReader can not be enabled because Subscriber is not enabled." << endl;
        return 1;
      }

      // Enable every entity from factory to it's entities and it should succeed.
      if (participant->enable () != ::DDS::RETCODE_OK
        || topic->enable () != ::DDS::RETCODE_OK
        || sub->enable () != ::DDS::RETCODE_OK)
      {
        cerr << "Failed to enable factory." << endl;
        return 1;
      }

      // The datareader is not enabled so it will not able to
      // communicate with datawriter.
      int i = 0;
      while (i < 5 && listener_servant.num_reads() == 0)
      {
        ACE_OS::sleep (1);
        ++i;
      }

      if (listener_servant.num_reads() > 0)
      {
        cerr << "Should not receive any samples since datareader is not enabled." << endl;
        return 1;
      }

      if (dr->enable () != ::DDS::RETCODE_OK)
      {
        cerr << "Failed to enable DataReader." << endl;
        return 1;
      }

      int expected = 10;

      while ( listener_servant.num_reads() < expected ) {
        ACE_OS::sleep (1);
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
