// $Id$

#include "DataReaderListener.h"
#include "TestTypeSupportC.h"
#include "TestTypeSupportImpl.h"

/// Control the spew.
namespace { enum { BE_REALLY_VERBOSE = 1};}

Test::DataReaderListener::DataReaderListener( const bool verbose)
 : verbose_( verbose)
{
}

Test::DataReaderListener::~DataReaderListener ()
{
}

const std::map< long, long>&
Test::DataReaderListener::counts() const
{
  return this->counts_;
}

const std::map< long, long>&
Test::DataReaderListener::bytes() const
{
  return this->bytes_;
}

const std::map< long, long>&
Test::DataReaderListener::priorities() const
{
  return this->priorities_;
}

void
Test::DataReaderListener::on_data_available (DDS::DataReader_ptr reader)
  throw (CORBA::SystemException)
{
  Test::DataDataReader_var dr = Test::DataDataReader::_narrow (reader);
  if (CORBA::is_nil (dr.in ())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT ("(%P|%t) Test::DataReaderListener::on_data_available() - ")
      ACE_TEXT ("data on unexpected reader type.\n")
    ));
    return;
  }

  Test::Data      data;
  DDS::SampleInfo info;

  while( DDS::RETCODE_OK == dr->take_next_sample( data, info)) {
    if( info.valid_data) {
      ++this->counts_[ data.pid];
      this->bytes_[ data.pid] += data.buffer.length();
      this->priorities_[ data.pid] = data.priority; // faster than conditional.
      if( this->verbose_ && BE_REALLY_VERBOSE) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderListener::on_data_available() - ")
          ACE_TEXT("received priority %d sample %d, length %d\n"),
          data.priority,
          data.seq,
          data.buffer.length()
        ));
      }
    } else {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DataReaderListener::on_data_available() - ")
        ACE_TEXT("received an INVALID sample.\n")
      ));
    }
  }
}

void
Test::DataReaderListener::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
  throw (CORBA::SystemException)
{
}

void
Test::DataReaderListener::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
  throw (CORBA::SystemException)
{
}

void
Test::DataReaderListener::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
  throw (CORBA::SystemException)
{
  if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListener::on_liveliness_changed()")
    ));
  }
}

void
Test::DataReaderListener::on_subscription_match (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchStatus &)
  throw (CORBA::SystemException)
{
}

void
Test::DataReaderListener::on_sample_rejected (
    DDS::DataReader_ptr,
    DDS::SampleRejectedStatus const &)
  throw (CORBA::SystemException)
{
}

void
Test::DataReaderListener::on_sample_lost (DDS::DataReader_ptr,
                                          DDS::SampleLostStatus const &)
  throw (CORBA::SystemException)
{
}

void
Test::DataReaderListener::on_subscription_disconnected (
    DDS::DataReader_ptr,
    ::OpenDDS::DCPS::SubscriptionDisconnectedStatus const &)
  throw (CORBA::SystemException)
{
}

void
Test::DataReaderListener::on_subscription_reconnected (
    DDS::DataReader_ptr,
    ::OpenDDS::DCPS::SubscriptionReconnectedStatus const &)
  throw (CORBA::SystemException)
{
}

void
Test::DataReaderListener::on_subscription_lost (
    DDS::DataReader_ptr,
    ::OpenDDS::DCPS::SubscriptionLostStatus const &)
  throw (CORBA::SystemException)
{
}

void
Test::DataReaderListener::on_budget_exceeded (
    DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::BudgetExceededStatus&)
  throw (CORBA::SystemException)
{
}

void
Test::DataReaderListener::on_connection_deleted (DDS::DataReader_ptr)
  throw (CORBA::SystemException)
{
}

