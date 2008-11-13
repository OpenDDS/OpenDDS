// $Id$

#include "DataReaderListener.h"
#include "TestTypeSupportC.h"
#include "TestTypeSupportImpl.h"

Test::DataReaderListener::DataReaderListener()
{
}

Test::DataReaderListener::~DataReaderListener ()
{
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

  Test::Data the_data;
  DDS::SampleInfo si;
  (void) dr->take_next_sample (the_data, si);
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
}

void
Test::DataReaderListener::on_subscription_match (
    DDS::DataReader_ptr reader,
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
Test::DataReaderListener::on_connection_deleted (DDS::DataReader_ptr)
  throw (CORBA::SystemException)
{
}

