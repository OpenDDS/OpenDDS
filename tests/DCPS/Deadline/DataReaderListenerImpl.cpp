#include "DataReaderListenerImpl.h"

#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/TimeTypes.h>

using namespace Messenger;

DataReaderListenerImpl::DataReaderListenerImpl()
  : mutex_()
  , matched_condition_(mutex_)
  , matched_(0)
  , num_arrived_(0)
  , requested_deadline_total_count_ (0)
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
  if ((requested_deadline_total_count_ + status.total_count_change) != status.total_count) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed:")
      ACE_TEXT("Received incorrect total_count_change, previous total count %d ")
      ACE_TEXT("new total_count=%d total_count_change=%d last_instance_handle=%d\n"),
      requested_deadline_total_count_, status.total_count, status.total_count_change,
      status.last_instance_handle));
  } else {
    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed:")
              ACE_TEXT("total_count=%d total_count_change=%d last_instance_handle=%d\n"),
      status.total_count, status.total_count_change, status.last_instance_handle));
  }
  requested_deadline_total_count_ += status.total_count_change;
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
  matched_condition_.notify_all();
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

bool DataReaderListenerImpl::wait_matched(
  long count, const OpenDDS::DCPS::TimeDuration& max_wait) const
{
  using namespace OpenDDS::DCPS;

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, false);

  const MonotonicTimePoint deadline = MonotonicTimePoint::now() + max_wait;
  while (count != matched_) {
    switch (matched_condition_.wait_until(deadline)) {
    case CvStatus_NoTimeout:
      break;

    case CvStatus_Timeout:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DataReaderListenerImpl::wait_matched: Timeout\n"));
      return false;

    case CvStatus_Error:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DataReaderListenerImpl::wait_matched: "
        "Error in wait_until\n"));
      return false;
    }
  }

  return true;
}

CORBA::Long DataReaderListenerImpl::requested_deadline_total_count (void) const
{
  return requested_deadline_total_count_;
}
