#include "DataWriterListenerImpl.h"
#include "dds/DdsDcpsPublicationC.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/TimeTypes.h"

#include <iostream>

using namespace std;


DataWriterListenerImpl::DataWriterListenerImpl()
  : mutex_()
  , matched_condition_(mutex_)
  , matched_(0)
  , offered_deadline_total_count_ (0)
{
}

DataWriterListenerImpl::~DataWriterListenerImpl()
{
}

void
DataWriterListenerImpl::on_offered_deadline_missed(
    ::DDS::DataWriter_ptr /* writer */,
    const ::DDS::OfferedDeadlineMissedStatus& status)
{
  if ((offered_deadline_total_count_ + status.total_count_change) != status.total_count)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_offered_deadline_missed:")
        ACE_TEXT("Received incorrect total_count_change, previous total count %d ")
        ACE_TEXT("new total_count=%d total_count_change=%d last_instance_handle=%d\n"),
        offered_deadline_total_count_, status.total_count, status.total_count_change,
        status.last_instance_handle));
    }
  else
    {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_offered_deadline_missed:")
        ACE_TEXT("total_count=%d total_count_change=%d last_instance_handle=%d\n"),
        status.total_count, status.total_count_change, status.last_instance_handle));
    }
  offered_deadline_total_count_ += status.total_count_change;
}

void
DataWriterListenerImpl::on_offered_incompatible_qos(
    ::DDS::DataWriter_ptr /* writer */,
    const ::DDS::OfferedIncompatibleQosStatus& /* status */)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_offered_incompatible_qos\n")));
}

void
DataWriterListenerImpl::on_liveliness_lost(
    ::DDS::DataWriter_ptr /* writer */,
    const ::DDS::LivelinessLostStatus& /* status */)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_liveliness_lost\n")));
}

void
DataWriterListenerImpl::on_publication_matched(
    ::DDS::DataWriter_ptr /* writer */,
    const ::DDS::PublicationMatchedStatus& status)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  matched_ = status.current_count;
  matched_condition_.notify_all();
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_publication_matched\n")));
}

void
DataWriterListenerImpl::on_publication_disconnected(
    ::DDS::DataWriter_ptr /* writer */,
    const ::OpenDDS::DCPS::PublicationDisconnectedStatus& /* status */)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_publication_disconnected\n")));
}

void
DataWriterListenerImpl::on_publication_reconnected(
    ::DDS::DataWriter_ptr /* writer */,
    const ::OpenDDS::DCPS::PublicationReconnectedStatus& /* status */)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_publication_reconnected\n")));
}

void
DataWriterListenerImpl::on_publication_lost(
    ::DDS::DataWriter_ptr /* writer */,
    const ::OpenDDS::DCPS::PublicationLostStatus& /* status */)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_publication_lost\n")));
}

bool DataWriterListenerImpl::wait_matched(
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
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DataWriterListenerImpl::wait_matched: Timeout\n"));
      return false;

    case CvStatus_Error:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DataWriterListenerImpl::wait_matched: "
        "Error in wait_until\n"));
      return false;
    }
  }

  return true;
}

CORBA::Long DataWriterListenerImpl::offered_deadline_total_count (void) const
{
  return offered_deadline_total_count_;
}
