/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ProactorEventDispatcher.h"
#ifdef ACE_HAS_AIO_CALLS
#  include <ace/POSIX_CB_Proactor.h>
#endif
#include "Service_Participant.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ProactorEventDispatcher::ProactorEventDispatcher(size_t count)
 : cv_(mutex_)
 , active_threads_(0)
#ifdef ACE_HAS_AIO_CALLS
 , proactor_(new ACE_Proactor(new ACE_POSIX_AIOCB_Proactor()))
#else
 , proactor_(new ACE_Proactor())
#endif
{
  proactor_->number_of_threads(count);
  pool_ = make_rch<ThreadPool>(count, run, this);
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  while (active_threads_ != count) {
    cv_.wait(TheServiceParticipant->get_thread_status_manager());
  }
}

ProactorEventDispatcher::~ProactorEventDispatcher()
{
  shutdown();
}

void ProactorEventDispatcher::shutdown(bool)
{
  Proactor_rap local;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    local = proactor_;
    proactor_.reset();
  }
  if (local) {
    local->proactor_end_event_loop();
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    while (active_threads_ != 0) {
      cv_.wait(TheServiceParticipant->get_thread_status_manager());
    }
  }
}

bool ProactorEventDispatcher::dispatch(EventBase_rch event)
{
  return schedule(event, MonotonicTimePoint::zero_value) >= 0;
}

long ProactorEventDispatcher::schedule(EventBase_rch event, const MonotonicTimePoint& expiration)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!proactor_) {
    return -1;
  }
  event->_add_ref();
  const TimeDuration delta = expiration == MonotonicTimePoint::zero_value ? TimeDuration::zero_value : expiration - MonotonicTimePoint::now();
  const DDS::Duration_t dds_delta = delta.to_dds_duration();
  const long result = proactor_->schedule_timer(*this, event.get(), ACE_Time_Value(dds_delta.sec, dds_delta.nanosec / 1000));
  if (result < 0) {
    event->_remove_ref();
  }
  return result;
}

size_t ProactorEventDispatcher::cancel(long id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!proactor_) {
    return 0;
  }
  const void* arg = 0;
  const size_t result = proactor_->cancel_timer(id, &arg);
  if (result) {
    EventBase* ptr = static_cast<EventBase*>(const_cast<void*>(arg));
    if (ptr) {
      ptr->handle_cancel();
      ptr->_remove_ref();
    }
  }
  return result;
}

void ProactorEventDispatcher::handle_time_out(const ACE_Time_Value&, const void *act)
{
  EventBase* ptr = static_cast<EventBase*>(const_cast<void*>(act));
  if (ptr) {
    (*ptr)();
  }
}

ACE_THR_FUNC_RETURN ProactorEventDispatcher::run(void* arg)
{
  ProactorEventDispatcher& dispatcher = *static_cast<ProactorEventDispatcher*>(arg);
  Proactor_rap proactor = dispatcher.proactor_;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(dispatcher.mutex_);
    ++dispatcher.active_threads_;
    dispatcher.cv_.notify_all();
  }
  if (proactor) {
    proactor->proactor_run_event_loop();
  }
  {
    ACE_Guard<ACE_Thread_Mutex> guard(dispatcher.mutex_);
    --dispatcher.active_threads_;
    dispatcher.cv_.notify_all();
  }
  return 0;
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
