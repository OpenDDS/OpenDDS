// -*- C++ -*-
// *******************************************************************
//
// (c) Copyright 2006, Object Computing, Inc.
// All Rights Reserved.
//
// *******************************************************************

#include "QuoteDataReaderListenerImpl.h"
#include "StockQuoterTypeSupportC.h"
#include "StockQuoterTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>


QuoteDataReaderListenerImpl::QuoteDataReaderListenerImpl()
{
}

QuoteDataReaderListenerImpl::~QuoteDataReaderListenerImpl ()
{
}

void QuoteDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  try {
    StockQuoter::QuoteDataReader_var quote_dr
      = StockQuoter::QuoteDataReader::_narrow(reader);
    if (CORBA::is_nil (quote_dr.in ())) {
      cerr << "QuoteDataReaderListenerImpl::on_data_available: _narrow failed." << endl;
      ACE_OS::exit(1);
    }

    int count = 0;
    while(true) {
      StockQuoter::Quote quote;
      DDS::SampleInfo si ;
      DDS::ReturnCode_t status = quote_dr->take_next_sample(quote, si) ;

      if (status == DDS::RETCODE_OK && si.valid_data) {
        ++count;
        cout << "Quote: ticker    = " << quote.ticker.in()    << endl
             << "       exchange  = " << quote.exchange.in()  << endl
             << "       full name = " << quote.full_name.in() << endl
             << "       value     = " << quote.value          << endl
             << "       timestamp = " << quote.timestamp      << endl;
        cout << "SampleInfo.sample_rank        = " << si.sample_rank << endl;
        cout << "SampleInfo.instance_handle    = " << hex << si.instance_handle << endl;
        cout << "SampleInfo.publication_handle = " << hex << si.publication_handle << endl;
      } else if (status == DDS::RETCODE_NO_DATA) {
        cerr << "INFO: reading complete after " << count << " samples." << endl;
        break;
      } else {
        cerr << "ERROR: read Quote: Error: " <<  status << endl;
      }
    }

  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in read:" << endl << e << endl;
    ACE_OS::exit(1);
  }
}

void QuoteDataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
  cerr << "QuoteDataReaderListenerImpl::on_requested_deadline_missed" << endl;
}

void QuoteDataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  cerr << "QuoteDataReaderListenerImpl::on_requested_incompatible_qos" << endl;
}

void QuoteDataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
  cerr << "QuoteDataReaderListenerImpl::on_liveliness_changed" << endl;
}

void QuoteDataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
{
  cerr << "QuoteDataReaderListenerImpl::on_subscription_matched" << endl;
}

void QuoteDataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
  cerr << "QuoteDataReaderListenerImpl::on_sample_rejected" << endl;
}

void QuoteDataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  cerr << "QuoteDataReaderListenerImpl::on_sample_lost" << endl;
}
