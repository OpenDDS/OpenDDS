// -*- C++ -*-
// *******************************************************************
//
// (c) Copyright 2006, Object Computing, Inc.
// All Rights Reserved.
//
// *******************************************************************

#include "StockQuoterTypeSupportImpl.h"
#include <ace/streams.h>
#include <ace/OS_NS_unistd.h>
#include <orbsvcs/Time_Utilities.h>

#include "Domain_Manager.h"
#include "Topic_Manager.h"
#include "Topic_Manager_T.h"

// constants for Stock Quoter domain Id, types, and topic
DDS::DomainId_t QUOTER_DOMAIN_ID = 1066;
const char* QUOTER_QUOTE_TOPIC = "Stock Quotes";
const char* QUOTER_EXCHANGE_EVENT_TOPIC = "Stock Exchange Events";
const char* STOCK_EXCHANGE_NAME = "Test Stock Exchange";

TimeBase::TimeT get_timestamp() {
  TimeBase::TimeT retval;
  ACE_hrtime_t t = ACE_OS::gethrtime ();
  ORBSVCS_Time::hrtime_to_TimeT (retval, t );
  return retval;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[]) {

  try {
    // create a domain manager, containing DomainParticipant configuration
    Domain_Manager domain_manager (argc,
                                   argv,
                                   QUOTER_DOMAIN_ID);

    // create a topic manager, related to the quote topic
    Topic_Manager quoter_topic_manager (
      new Topic_Manager_T <StockQuoter::QuoteTypeSupport,
                           StockQuoter::QuoteTypeSupportImpl> (
        QUOTER_QUOTE_TOPIC));

    // create a topic manager, related to the exchange topic
    Topic_Manager exchange_topic_manager (
       new Topic_Manager_T <StockQuoter::ExchangeEventTypeSupport,
                            StockQuoter::ExchangeEventTypeSupportImpl> (
        QUOTER_EXCHANGE_EVENT_TOPIC));

    // get a publication manager
    Publication_Manager publication_manager =
      domain_manager.publication_manager ();

    // create a quoter topic and an according data writer
    DDS::DataWriter_var quote_base_dw =
      publication_manager.access_topic (quoter_topic_manager);

    // narrow the received data writer
    StockQuoter::QuoteDataWriter_var quote_dw
      = StockQuoter::QuoteDataWriter::_narrow(quote_base_dw.in());

    if (CORBA::is_nil (quote_dw.in ())) {
      cerr << "QuoteDataWriter could not be narrowed"<< endl;
      ACE_OS::exit(1);
    }

    // create an exchange topic and an according data writer
    DDS::DataWriter_var exchange_evt_base_dw =
      publication_manager.access_topic (exchange_topic_manager);

    // narrow the received data writer
    StockQuoter::ExchangeEventDataWriter_var exchange_evt_dw
      = StockQuoter::ExchangeEventDataWriter::_narrow(exchange_evt_base_dw.in());
    if (CORBA::is_nil (exchange_evt_dw.in ())) {
      cerr << "ExchangeEventDataWriter could not be narrowed"<< endl;
      ACE_OS::exit(1);
    }

    // Register the Exchange Event and the two Quoted securities (SPY and MDY)
    StockQuoter::ExchangeEvent ex_evt;
    ex_evt.exchange = STOCK_EXCHANGE_NAME;
    DDS::InstanceHandle_t exchange_handle = exchange_evt_dw->register_instance(ex_evt);

    StockQuoter::Quote spy;
    spy.ticker = CORBA::string_dup("SPY");
    DDS::InstanceHandle_t spy_handle = quote_dw->register_instance(spy);

    StockQuoter::Quote mdy;
    mdy.ticker = CORBA::string_dup("MDY");
    DDS::InstanceHandle_t mdy_handle = quote_dw->register_instance(mdy);

    // Publish...

    StockQuoter::ExchangeEvent opened;
    opened.exchange = STOCK_EXCHANGE_NAME;
    opened.event = StockQuoter::TRADING_OPENED;
    opened.timestamp = get_timestamp();

    cout << "Publishing TRADING_OPENED" << endl;
    DDS::ReturnCode_t ret = exchange_evt_dw->write(opened, exchange_handle);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("(%P|%t)ERROR: OPEN write returned %d.\n"), ret));
    }

    ACE_Time_Value quarterSecond( 0, 250000 );
    for ( int i = 0; i < 20; ++i ) {
      StockQuoter::Quote spy_quote;
      spy_quote.exchange = STOCK_EXCHANGE_NAME;
      spy_quote.ticker = CORBA::string_dup("SPY");
      spy_quote.full_name = CORBA::string_dup("S&P Depository Receipts");
      spy_quote.value = 1200.0 + 10.0*i;
      spy_quote.timestamp = get_timestamp();

      cout << "Publishing SPY Quote: " << spy_quote.value << endl;
      ret = quote_dw->write(spy_quote, spy_handle);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR ((LM_ERROR, ACE_TEXT("(%P|%t)ERROR: SPY write returned %d.\n"), ret));
      }

      ACE_OS::sleep( quarterSecond );

      StockQuoter::Quote mdy_quote;
      mdy_quote.exchange = STOCK_EXCHANGE_NAME;
      mdy_quote.ticker = CORBA::string_dup("MDY");
      mdy_quote.full_name = CORBA::string_dup("S&P Midcap Depository Receipts");
      mdy_quote.value = 1400.0 + 10.0*i;
      mdy_quote.timestamp = get_timestamp();

      cout << "Publishing MDY Quote: " << mdy_quote.value <<endl;
      ret = quote_dw->write(mdy_quote, mdy_handle);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR ((LM_ERROR, ACE_TEXT("(%P|%t)ERROR: MDY write returned %d.\n"), ret));
      }

      ACE_OS::sleep( quarterSecond );
    }

    StockQuoter::ExchangeEvent closed;
    closed.exchange = STOCK_EXCHANGE_NAME;
    closed.event = StockQuoter::TRADING_CLOSED;
    closed.timestamp = get_timestamp();

    cout << "Publishing TRADING_CLOSED" << endl;
    ret = exchange_evt_dw->write(closed, exchange_handle);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("(%P|%t)ERROR: CLOSED write returned %d.\n"), ret));
    }

    cout << "Exiting..." << endl;
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
