// -*- C++ -*-
//

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
{
  ++num_arrived_;
}

void
DataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr /* reader */,
    DDS::RequestedDeadlineMissedStatus const & status)
{
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed:")
              ACE_TEXT("total_count=%d total_count_change=%d last_instance_handle=%d\n"),
    status.total_count, status.total_count_change, status.last_instance_handle));
}

void
DataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  //cerr << "DataReaderListenerImpl::on_requested_incompatible_qos" << endl;
}

void
DataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
  //cerr << "DataReaderListenerImpl::on_liveliness_changed" << endl;
}

void
DataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
{
  //cerr << "DataReaderListenerImpl::on_subscription_matched" << endl;
}

void
DataReaderListenerImpl::on_sample_rejected (
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
  //cerr << "DataReaderListenerImpl::on_sample_rejected" << endl;
}

void
DataReaderListenerImpl::on_sample_lost (
    DDS::DataReader_ptr,
    const DDS::SampleLostStatus&)
{
  //cerr << "DataReaderListenerImpl::on_sample_lost" << endl;
}

void
DataReaderListenerImpl::on_subscription_disconnected (
    DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus &)
{
  //cerr << "DataReaderListenerImpl::on_subscription_disconnected" << endl;
}

void
DataReaderListenerImpl::on_subscription_reconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionReconnectedStatus &)
{
  //cerr << "DataReaderListenerImpl::on_subscription_reconnected" << endl;
}

void
DataReaderListenerImpl::on_subscription_lost (
    DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::SubscriptionLostStatus &)
{
  //cerr << "DataReaderListenerImpl::on_subscription_lost" << endl;
}

void DataReaderListenerImpl::on_budget_exceeded (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::BudgetExceededStatus&)
{
  //cerr << "DataReaderListenerImpl::on_budget_exceeded" << endl;
}

void
DataReaderListenerImpl::on_connection_deleted (DDS::DataReader_ptr)
{
  //cerr << "DataReaderListenerImpl::on_connection_deleted" << endl;
}
