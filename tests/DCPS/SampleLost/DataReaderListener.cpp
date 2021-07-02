// -*- C++ -*-
//

#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>

using namespace Messenger;

DataReaderListenerImpl::DataReaderListenerImpl()
  : num_data_available_(0),
    num_samples_lost_ (0),
    num_samples_rejected_ (0),
    num_budget_exceeded_ (0)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl ()
{
}

void
DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr /*reader*/)
{
  ++num_data_available_;
}

void
DataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
  cerr << "DataReaderListenerImpl::on_requested_deadline_missed" << endl;
}

void
DataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  cerr << "DataReaderListenerImpl::on_requested_incompatible_qos" << endl;
}

void
DataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
  cerr << "DataReaderListenerImpl::on_liveliness_changed" << endl;
}

void
DataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_matched" << endl;
}

void
DataReaderListenerImpl::on_sample_rejected (
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus& status)
{
  this->num_samples_rejected_ += status.total_count_change;

  cerr << "DataReaderListenerImpl::on_sample_rejected, "
       << " total_count <" << status.total_count
       << "> total_count_change <" << status.total_count_change
       << "> last_reason <" << status.last_reason
       << "> last_instance_handle <" << status.last_instance_handle
       << ">"
       << endl;

  if (this->num_samples_rejected_ != status.total_count) {
    cerr << "ERROR: Incorrected total_count_change <" << status.total_count_change << "> reported for sample rejected" << endl;
  }
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus& status)
{
  this->num_samples_lost_ += status.total_count_change;

  cerr << "DataReaderListenerImpl::on_sample_lost, "
       << " total_count <" << status.total_count
       << "> total_count_change <" << status.total_count_change
       << ">"
       << endl;

  if (this->num_samples_lost_ != status.total_count) {
    cerr << "ERROR: Incorrected total_count_change <" << status.total_count_change << "> reported for sample lost" << endl;
  }
}

void DataReaderListenerImpl::on_subscription_disconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_disconnected" << endl;
}

void DataReaderListenerImpl::on_subscription_reconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionReconnectedStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_reconnected" << endl;
}

void DataReaderListenerImpl::on_subscription_lost (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionLostStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_lost" << endl;
}

void DataReaderListenerImpl::on_budget_exceeded (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::BudgetExceededStatus& status)
{
  this->num_budget_exceeded_ += status.total_count_change;

  cerr << "DataReaderListenerImpl::on_budget_exceeded, "
       << " total_count <" << status.total_count
       << "> total_count_change <" << status.total_count_change
       << ">"
       << endl;

  if (this->num_budget_exceeded_ != status.total_count) {
    cerr << "ERROR: Incorrected total_count_change <" << status.total_count_change << "> reported for budget exceeded" << endl;
  }
}
