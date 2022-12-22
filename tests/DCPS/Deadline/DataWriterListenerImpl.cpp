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
  , offered_deadline_total_count_(0)
{
}

DataWriterListenerImpl::~DataWriterListenerImpl()
{
}

bool DataWriterListenerImpl::wait_matched(long count, const OpenDDS::DCPS::TimeDuration& max_wait) const
{
  using namespace OpenDDS::DCPS;
  Lock lock(mutex_);
  if (!lock.locked()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DataWriterListenerImpl::wait_matched: failed to lock\n")));
    return false;
  }
  const MonotonicTimePoint deadline = MonotonicTimePoint::now() + max_wait;
  ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  while (count != matched_) {
    switch (matched_condition_.wait_until(deadline, thread_status_manager)) {
    case CvStatus_NoTimeout:
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) DataWriterListenerImpl::wait_matched: %d\n"), matched_));
      break;

    case CvStatus_Timeout:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DataWriterListenerImpl::wait_matched: Timeout\n"));
      return false;

    case CvStatus_Error:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DataWriterListenerImpl::wait_matched: wait_until returned error\n"));
      return false;
    }
  }
  return true;
}

CORBA::Long DataWriterListenerImpl::offered_deadline_total_count() const
{
  return offered_deadline_total_count_;
}

void DataWriterListenerImpl::on_offered_deadline_missed(DDS::DataWriter_ptr , const DDS::OfferedDeadlineMissedStatus& status)
{
  if ((offered_deadline_total_count_ + status.total_count_change) != status.total_count) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_offered_deadline_missed: ")
      ACE_TEXT("Received incorrect change, previous count %d new count=%d change=%d instance=%d\n"),
      offered_deadline_total_count_,
      status.total_count, status.total_count_change, status.last_instance_handle));
  } else {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_offered_deadline_missed: count=%d change=%d instance=%d\n"),
      status.total_count, status.total_count_change, status.last_instance_handle));
  }
  offered_deadline_total_count_ += status.total_count_change;
}

void DataWriterListenerImpl::on_publication_matched(DDS::DataWriter_ptr, const DDS::PublicationMatchedStatus& status)
{
  Lock lock(mutex_);
  if (!lock.locked()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DataWriterListenerImpl::on_publication_matched: failed to lock\n")));
    return;
  }
  matched_ = status.current_count;
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_publication_matched %d\n"), matched_));
  matched_condition_.notify_all();
}

void DataWriterListenerImpl::on_offered_incompatible_qos(DDS::DataWriter_ptr, const DDS::OfferedIncompatibleQosStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_offered_incompatible_qos\n")));
}

void DataWriterListenerImpl::on_liveliness_lost(DDS::DataWriter_ptr, const DDS::LivelinessLostStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_liveliness_lost\n")));
}

void DataWriterListenerImpl::on_publication_disconnected(DDS::DataWriter_ptr, const ::OpenDDS::DCPS::PublicationDisconnectedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_publication_disconnected\n")));
}

void DataWriterListenerImpl::on_publication_reconnected(DDS::DataWriter_ptr, const ::OpenDDS::DCPS::PublicationReconnectedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_publication_reconnected\n")));
}

void DataWriterListenerImpl::on_publication_lost(DDS::DataWriter_ptr, const ::OpenDDS::DCPS::PublicationLostStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_publication_lost\n")));
}
