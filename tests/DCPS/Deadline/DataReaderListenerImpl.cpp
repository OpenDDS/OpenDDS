#include "DataReaderListenerImpl.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>

using namespace Messenger;

DataReaderListenerImpl::DataReaderListenerImpl()
  : mutex_()
  , matched_condition_(mutex_)
  , matched_(0)
  , num_arrived_(0)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

void
DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  ++num_arrived_;
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_data_available\n")));
}

void
DataReaderListenerImpl::on_requested_deadline_missed(
    DDS::DataReader_ptr /* reader */,
    DDS::RequestedDeadlineMissedStatus const & status)
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed:")
             ACE_TEXT("total_count=%d total_count_change=%d last_instance_handle=%d\n"),
    status.total_count, status.total_count_change, status.last_instance_handle));
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed\n")));
}

void
DataReaderListenerImpl::on_requested_incompatible_qos(
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos\n")));
}

void
DataReaderListenerImpl::on_liveliness_changed(
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_liveliness_changed\n")));
}

void
DataReaderListenerImpl::on_subscription_matched(
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus& status)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  matched_ = status.current_count;
  matched_condition_.broadcast();
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_matched\n")));
}

void
DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_rejected\n")));
}

void
DataReaderListenerImpl::on_sample_lost(
    DDS::DataReader_ptr,
    const DDS::SampleLostStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_lost\n")));
}

void
DataReaderListenerImpl::on_subscription_disconnected(
    DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_disconnected\n")));
}

void
DataReaderListenerImpl::on_subscription_reconnected(
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionReconnectedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_reconnected\n")));
}

void
DataReaderListenerImpl::on_subscription_lost(
    DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::SubscriptionLostStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_lost\n")));
}

void DataReaderListenerImpl::on_budget_exceeded(
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::BudgetExceededStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_budget_exceeded\n")));
}

int DataReaderListenerImpl::wait_matched(long count, const ACE_Time_Value *abstime) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, -1);

  int result = 0;
  while (count != matched_ && result == 0) {
    result = matched_condition_.wait(abstime);
  }
  return count == matched_ ? 0 : result;
}

