// -*- C++ -*-
//
// $Id$

#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>

using namespace Messenger;

DataReaderListenerImpl::DataReaderListenerImpl ()
  : num_arrived_(0)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl ()
{
}

void
DataReaderListenerImpl::on_data_available (DDS::DataReader_ptr)
  throw (CORBA::SystemException)
{
  num_arrived_ ++;
}

void
DataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr /* reader */,
    DDS::RequestedDeadlineMissedStatus const & status)
  throw (CORBA::SystemException)
{
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT("(%P|%t)DataReaderListenerImpl::on_requested_deadline_missed:")
              ACE_TEXT("total_count=%d total_count_change=%d last_instance_handle=%d\n"),
    status.total_count, status.total_count_change, status.last_instance_handle));
}

void
DataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
  throw (CORBA::SystemException)
{
  //cerr << "DataReaderListenerImpl::on_requested_incompatible_qos" << endl;
}

void
DataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
  throw (CORBA::SystemException)
{
  //cerr << "DataReaderListenerImpl::on_liveliness_changed" << endl;
}

void
DataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
  throw (CORBA::SystemException)
{
  //cerr << "DataReaderListenerImpl::on_subscription_matched" << endl;
}

void
DataReaderListenerImpl::on_sample_rejected (
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
  throw (CORBA::SystemException)
{
  //cerr << "DataReaderListenerImpl::on_sample_rejected" << endl;
}

void
DataReaderListenerImpl::on_sample_lost (
    DDS::DataReader_ptr,
    const DDS::SampleLostStatus&)
  throw (CORBA::SystemException)
{
  //cerr << "DataReaderListenerImpl::on_sample_lost" << endl;
}

void
DataReaderListenerImpl::on_subscription_disconnected (
    DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus &)
  throw (CORBA::SystemException)
{
  //cerr << "DataReaderListenerImpl::on_subscription_disconnected" << endl;
}

void
DataReaderListenerImpl::on_subscription_reconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionReconnectedStatus &)
  throw (CORBA::SystemException)
{
  //cerr << "DataReaderListenerImpl::on_subscription_reconnected" << endl;
}

void
DataReaderListenerImpl::on_subscription_lost (
    DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::SubscriptionLostStatus &)
  throw (CORBA::SystemException)
{
  //cerr << "DataReaderListenerImpl::on_subscription_lost" << endl;
}

void DataReaderListenerImpl::on_budget_exceeded (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::BudgetExceededStatus&)
  throw (CORBA::SystemException)
{
  //cerr << "DataReaderListenerImpl::on_budget_exceeded" << endl;
}

void
DataReaderListenerImpl::on_connection_deleted (DDS::DataReader_ptr)
  throw (CORBA::SystemException)
{
  //cerr << "DataReaderListenerImpl::on_connection_deleted" << endl;
}
