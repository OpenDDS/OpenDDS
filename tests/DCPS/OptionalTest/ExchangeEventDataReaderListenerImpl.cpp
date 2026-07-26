// -*- C++ -*-
// *******************************************************************
//
// (c) Copyright 2006, Object Computing, Inc.
// All Rights Reserved.
//
// *******************************************************************

#include "ExchangeEventDataReaderListenerImpl.h"
#include "StockQuoterTypeSupportC.h"
#include "StockQuoterTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>

ExchangeEventDataReaderListenerImpl::ExchangeEventDataReaderListenerImpl()
: is_exchange_closed_received_(0)
{
}

ExchangeEventDataReaderListenerImpl::~ExchangeEventDataReaderListenerImpl ()
{
}

// app-specific
CORBA::Boolean
ExchangeEventDataReaderListenerImpl::is_exchange_closed_received()
{
  ACE_Guard<ACE_Mutex> guard(this->lock_);
  return this->is_exchange_closed_received_;
}


void ExchangeEventDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  try {
    StockQuoter::ExchangeEventDataReader_var exchange_evt_dr
      = StockQuoter::ExchangeEventDataReader::_narrow(reader);
    if (CORBA::is_nil (exchange_evt_dr.in ())) {
      cerr << "ExchangeEventDataReaderListenerImpl::on_data_available: _narrow failed." << endl;
      ACE_OS::exit(1);
    }

    int count = 0;
    while(true) {
      StockQuoter::ExchangeEvent exchange_evt;
      DDS::SampleInfo si ;
      DDS::ReturnCode_t status = exchange_evt_dr->take_next_sample(exchange_evt, si) ;

      if (status == DDS::RETCODE_OK) {
        ++count;
        cout << "ExchangeEvent: exchange  = " << exchange_evt.exchange.in() << endl;

        switch ( exchange_evt.event ) {
          case StockQuoter::TRADING_OPENED:
            cout << "               event     = TRADING_OPENED" << endl;
            break;
          case StockQuoter::TRADING_CLOSED: {
            cout << "               event     = TRADING_CLOSED" << endl;
            ACE_Guard<ACE_Mutex> guard(this->lock_);
            this->is_exchange_closed_received_ = 1;
            break;
          }
          case StockQuoter::TRADING_SUSPENDED:
            cout << "               event     = TRADING_SUSPENDED" << endl;
            break;
          case StockQuoter::TRADING_RESUMED:
            cout << "               event     = TRADING_RESUMED" << endl;
            break;
          default:
            cerr << "ERROR: reader received unknown ExchangeEvent: " << exchange_evt.event << endl;
        }

      cout << "               timestamp = " << exchange_evt.timestamp      << endl;

        cout << "SampleInfo.sample_rank        = " << si.sample_rank << endl;
        cout << "SampleInfo.instance_handle    = " << hex << si.instance_handle << endl;
        cout << "SampleInfo.publication_handle = " << hex << si.publication_handle << endl;
      } else if (status == DDS::RETCODE_NO_DATA) {
        cerr << "INFO: reading complete after " << count << " samples." << endl;
        break;
      } else {
        cerr << "ERROR: read ExchangeEvent: Error: " <<  status << endl;
      }
    }

  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in read:" << endl << e << endl;
    ACE_OS::exit(1);
  }
}

void ExchangeEventDataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
  cerr << "ExchangeEventDataReaderListenerImpl::on_requested_deadline_missed" << endl;
}

void ExchangeEventDataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  cerr << "ExchangeEventDataReaderListenerImpl::on_requested_incompatible_qos" << endl;
}

void ExchangeEventDataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
  cerr << "ExchangeEventDataReaderListenerImpl::on_liveliness_changed" << endl;
}

void ExchangeEventDataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
{
  cerr << "ExchangeEventDataReaderListenerImpl::on_subscription_matched" << endl;
}

void ExchangeEventDataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
  cerr << "ExchangeEventDataReaderListenerImpl::on_sample_rejected" << endl;
}

void ExchangeEventDataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  cerr << "ExchangeEventDataReaderListenerImpl::on_sample_lost" << endl;
}
