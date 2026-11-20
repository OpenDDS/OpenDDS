// -*- C++ -*-
// *******************************************************************
//
// (c) Copyright 2006, Object Computing, Inc.
// All Rights Reserved.
//
// *******************************************************************

#include "StockQuoterTypeSupportImpl.h"
#include "QuoteDataReaderListenerImpl.h"
#include "ExchangeEventDataReaderListenerImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <ace/streams.h>
#include "ace/OS_NS_unistd.h"
#include <orbsvcs/Time_Utilities.h>

#include "dds/DCPS/StaticIncludes.h"

// constants for Stock Quoter domain Id, types, and topic
// (same as publisher)
DDS::DomainId_t QUOTER_DOMAIN_ID = 1066;
const char* QUOTER_QUOTE_TYPE = "Quote Type";
const char* QUOTER_QUOTE_TOPIC = "Stock Quotes";
const char* QUOTER_EXCHANGE_EVENT_TYPE = "Exchange Event Type";
const char* QUOTER_EXCHANGE_EVENT_TOPIC = "Stock Exchange Events";

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]) {

  DDS::DomainParticipantFactory_var dpf = DDS::DomainParticipantFactory::_nil();
  DDS::DomainParticipant_var participant = DDS::DomainParticipant::_nil();

  try {

    // Initialize, and create a DomainParticipant
    // (same as publishe

    dpf = TheParticipantFactoryWithArgs(argc, argv);

    participant = dpf->create_participant(
      QUOTER_DOMAIN_ID,
      PARTICIPANT_QOS_DEFAULT,
      DDS::DomainParticipantListener::_nil(),
      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (participant.in ())) {
      cerr << "create_participant failed." << endl;
      ACE_OS::exit(1);
    }

    // Create a subscriber for the two topics
    // (SUBSCRIBER_QOS_DEFAULT is defined in Marked_Default_Qos.h)
    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     DDS::SubscriberListener::_nil(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (sub.in ())) {
      cerr << "create_subscriber failed." << endl;
      ACE_OS::exit(1);
    }

    // Register the Quote type
    // (same as publisher)
    StockQuoter::QuoteTypeSupport_var quote_servant
      = new StockQuoter::QuoteTypeSupportImpl();

    if (DDS::RETCODE_OK != quote_servant->register_type(participant.in (),
                                                        QUOTER_QUOTE_TYPE)) {
      cerr << "register_type for " << QUOTER_QUOTE_TYPE << " failed." << endl;
      ACE_OS::exit(1);
    }

    // Register the ExchangeEvent type
    // (same as publisher)
    StockQuoter::ExchangeEventTypeSupport_var exchange_evt_servant
      = new StockQuoter::ExchangeEventTypeSupportImpl();

    if (DDS::RETCODE_OK != exchange_evt_servant->register_type(participant.in (),
                                                               QUOTER_EXCHANGE_EVENT_TYPE)) {
      cerr << "register_type for " << QUOTER_EXCHANGE_EVENT_TYPE
           << " failed." << endl;
      ACE_OS::exit(1);
    }

    // Get QoS to use for our two topics
    // (same as publisher)
    DDS::TopicQos default_topic_qos;
    participant->get_default_topic_qos(default_topic_qos);

    // Create a topic for the Quote type...
    // Could also use TOPIC_QOS_DEFAULT instead
    // (same as publisher)
    DDS::Topic_var quote_topic =
      participant->create_topic (QUOTER_QUOTE_TOPIC,
                                 QUOTER_QUOTE_TYPE,
                                 default_topic_qos,
                                 DDS::TopicListener::_nil(),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (quote_topic.in ())) {
      cerr << "create_topic for " << QUOTER_QUOTE_TOPIC << " failed." << endl;
      ACE_OS::exit(1);
    }

    // .. and another topic for the Exchange Event type
    // (same as publisher)
    DDS::Topic_var exchange_evt_topic =
      participant->create_topic (QUOTER_EXCHANGE_EVENT_TOPIC,
                                 QUOTER_EXCHANGE_EVENT_TYPE,
                                 default_topic_qos,
                                 DDS::TopicListener::_nil(),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (exchange_evt_topic.in ())) {
      cerr << "create_topic for " << QUOTER_EXCHANGE_EVENT_TOPIC << " failed." << endl;
      ACE_OS::exit(1);
    }

    // Create DataReaders and DataReaderListeners for the
    // Quote and ExchangeEvent
    DDS::DataReaderListener_var quote_listener (new QuoteDataReaderListenerImpl);

    if (CORBA::is_nil (quote_listener.in ())) {
      cerr << "Quote listener is nil." << endl;
      ACE_OS::exit(1);
    }

    DDS::DataReaderListener_var exchange_evt_listener (new ExchangeEventDataReaderListenerImpl);
    ExchangeEventDataReaderListenerImpl* listener_servant =
      dynamic_cast<ExchangeEventDataReaderListenerImpl*>(exchange_evt_listener.in());

    if (CORBA::is_nil (exchange_evt_listener.in ())) {
      cerr << "ExchangeEvent listener is nil." << endl;
      ACE_OS::exit(1);
    }

    // Create the Quote and ExchangeEvent DataReaders

    // Get the default QoS for our Data Readers
    // Could also use DATAREADER_QOS_DEFAULT
    DDS::DataReaderQos dr_default_qos;
    sub->get_default_datareader_qos (dr_default_qos);

    DDS::DataReader_var quote_dr =
      sub->create_datareader(quote_topic.in (),
                             dr_default_qos,
                             quote_listener.in (),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    DDS::DataReader_var exchange_evt_dr =
      sub->create_datareader(exchange_evt_topic.in (),
                             dr_default_qos,
                             exchange_evt_listener.in (),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // Wait for events from the Publisher; shut down when "close" received
    cout << "Subscriber: waiting for events" << endl;
    while ( ! listener_servant->is_exchange_closed_received() ) {
      ACE_OS::sleep(1);
    }

    cout << "Received CLOSED event from publisher; exiting..." << endl;

  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in main.cpp:" << endl
         << e << endl;
    ACE_OS::exit(1);
  }

  // Cleanup
  try {
    if (!CORBA::is_nil (participant.in ())) {
      participant->delete_contained_entities();
    }
    if (!CORBA::is_nil (dpf.in ())) {
      dpf->delete_participant(participant.in ());
    }
  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in cleanup." << endl << e << endl;
    ACE_OS::exit(1);
  }
  TheServiceParticipant->shutdown();
  return 0;
}
