// -*- C++ -*-
// *******************************************************************
//
// (c) Copyright 2006, Object Computing, Inc.
// All Rights Reserved.
//
// *******************************************************************

#include "QuoteDataReaderListenerImpl.h"
#include "QuoteTypeSupportC.h"
#include "QuoteTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>


// Implementation skeleton constructor
QuoteDataReaderListenerImpl::QuoteDataReaderListenerImpl()
{
}

// Implementation skeleton destructor
QuoteDataReaderListenerImpl::~QuoteDataReaderListenerImpl ()
{
}

void QuoteDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
  throw (CORBA::SystemException)
{
  try {
    StockQuoter::QuoteDataReader_var quote_dr 
      = StockQuoter::QuoteDataReader::_narrow(reader);
    if (CORBA::is_nil (quote_dr.in ())) {
      cerr << "QuoteDataReaderListenerImpl::on_data_available: _narrow failed." << endl;
      ACE_OS::exit(1);
    }

    StockQuoter::Quote quote;
    DDS::SampleInfo si ;
    DDS::ReturnCode_t status = quote_dr->take_next_sample(quote, si) ;

    if (status == DDS::RETCODE_OK) {
      cout << "Quote: ticker    = " << quote.ticker.in()    << endl
           << "       exchange  = " << quote.exchange.in()  << endl
           << "       full name = " << quote.full_name.in() << endl
           << "       value     = " << quote.value          << endl
           << "       timestamp = " << quote.timestamp      << endl;
      cout << "SampleInfo.sample_rank = " << si.sample_rank << endl;
    } else if (status == DDS::RETCODE_NO_DATA) {
      cerr << "ERROR: reader received DDS::RETCODE_NO_DATA!" << endl;
    } else {
      cerr << "ERROR: read Quote: Error: " <<  status << endl;
    }
  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in read:" << endl << e << endl;
    ACE_OS::exit(1);
  }
}

void QuoteDataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "QuoteDataReaderListenerImpl::on_requested_deadline_missed" << endl;
}

void QuoteDataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
  throw (CORBA::SystemException)
{
  cerr << "QuoteDataReaderListenerImpl::on_requested_incompatible_qos" << endl;
}

void QuoteDataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "QuoteDataReaderListenerImpl::on_liveliness_changed" << endl;
}

void QuoteDataReaderListenerImpl::on_subscription_match (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchStatus &)
  throw (CORBA::SystemException)
{
  cerr << "QuoteDataReaderListenerImpl::on_subscription_match" << endl;
}

void QuoteDataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
  throw (CORBA::SystemException)
{
  cerr << "QuoteDataReaderListenerImpl::on_sample_rejected" << endl;
}

void QuoteDataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
  throw (CORBA::SystemException)
{
  cerr << "QuoteDataReaderListenerImpl::on_sample_lost" << endl;
}
