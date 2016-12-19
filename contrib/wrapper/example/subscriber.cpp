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
#include <dds/DCPS/SubscriberImpl.h>
#include <ace/streams.h>
#include <ace/OS_NS_unistd.h>
#include <orbsvcs/Time_Utilities.h>

#include "Domain_Manager.h"
#include "Topic_Manager.h"
#include "Topic_Manager_T.h"

// constants for Stock Quoter domain Id, types, and topic
// (same as publisher)
DDS::DomainId_t QUOTER_DOMAIN_ID = 1066;
const char* QUOTER_QUOTE_TOPIC = "Stock Quotes";
const char* QUOTER_EXCHANGE_EVENT_TOPIC = "Stock Exchange Events";

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]) {

  try
    {
      // create a domain manager, containing DomainParticipant configuration
      Domain_Manager domain_manager (argc,
                                     argv,
                                     QUOTER_DOMAIN_ID);

      // Create DataReaders and DataReaderListeners for the
      // Quote and ExchangeEvent
      DDS::DataReaderListener_var quote_listener (new QuoteDataReaderListenerImpl);

      if (CORBA::is_nil (quote_listener.in ())) {
        cerr << "Quote listener is nil." << endl;
        ACE_OS::exit(1);
      }

      // create a topic manager, related to the quote topic
      Topic_Manager quoter_topic_manager (
        new Topic_Manager_T <StockQuoter::QuoteTypeSupport,
                             StockQuoter::QuoteTypeSupportImpl> (
          QUOTER_QUOTE_TOPIC, quote_listener.in ()));

      DDS::DataReaderListener_var exchange_evt_listener (new ExchangeEventDataReaderListenerImpl);
      ExchangeEventDataReaderListenerImpl* listener_servant =
        dynamic_cast<ExchangeEventDataReaderListenerImpl*>(exchange_evt_listener.in());

      if (CORBA::is_nil (exchange_evt_listener.in ())) {
        cerr << "ExchangeEvent listener is nil." << endl;
        ACE_OS::exit(1);
      }

      // create a topic manager, related to the exchange topic
      Topic_Manager exchange_topic_manager (
        new Topic_Manager_T <StockQuoter::ExchangeEventTypeSupport,
                             StockQuoter::ExchangeEventTypeSupportImpl> (
          QUOTER_EXCHANGE_EVENT_TOPIC, exchange_evt_listener.in ()));

      // get a subscription manager
      Subscription_Manager subscription_manager =
        domain_manager.subscription_manager ();

      // create topic and data reader for the topic
      subscription_manager.access_topic (quoter_topic_manager);

      // create topic and data reader for the topic
      subscription_manager.access_topic (exchange_topic_manager);

      // Wait for events from the Publisher; shut down when "close" received
      cout << "Subscriber: waiting for events" << endl;
      while ( ! listener_servant->is_exchange_closed_received() ) {
        ACE_OS::sleep(1);
      }

      cout << "Received CLOSED event from publisher; exiting..." << endl;
    } catch (Manager_Exception& e) {
      cerr << "Manager_Exception caught in main.cpp:" << endl
           << e.reason () << endl;
      ACE_OS::exit(1);
    } catch (CORBA::Exception& e) {
      cerr << "Exception caught in main.cpp:" << endl
           << e << endl;
      ACE_OS::exit(1);
    }

  return 0;
}
