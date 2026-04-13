/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "debug.h"
#include "ServiceEventDispatcher.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

namespace {

void handle_cancel_and_release(EventBase* ptr)
{
  if (!ptr) {
    return;
  }

  try {
    ptr->handle_cancel();
  } catch (...) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: ServiceEventDispatcher: handle_cancel threw an exception\n"));
    }
  }

  ptr->_remove_ref();
}

}

ServiceEventDispatcher::ServiceEventDispatcher(size_t count)
 : dispatcher_(make_rch<DispatchService>(count ? count : 1))
{
}

ServiceEventDispatcher::~ServiceEventDispatcher()
{
  shutdown();
}

void ServiceEventDispatcher::shutdown(bool immediate)
{
  DispatchService_rch local;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    local.swap(dispatcher_);
  }
  if (local) {
    DispatchService::EventQueue remaining;
    local->shutdown(immediate, &remaining);
    for (DispatchService::EventQueue::iterator it = remaining.begin(), limit = remaining.end(); it != limit; ++it) {
      handle_cancel_and_release(static_cast<EventBase*>(it->second));
    }
  }
}

bool ServiceEventDispatcher::dispatch(EventBase_rch event)
{
  if (!event) {
    return false;
  }

  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!dispatcher_) {
    return false;
  }
  event->_add_ref();
  const bool result = dispatcher_->dispatch(*event);
  if (!result) {
    event->_remove_ref();
  }
  return result;
}

long ServiceEventDispatcher::schedule(EventBase_rch event, const MonotonicTimePoint& expiration)
{
  if (!event) {
    return -1;
  }

  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!dispatcher_) {
    return -1;
  }
  event->_add_ref();
  const long result = dispatcher_->schedule(*event, expiration);
  if (result < 0) {
    event->_remove_ref();
  }
  return result;
}

size_t ServiceEventDispatcher::cancel(long id)
{
  EventBase* ptr = 0;
  size_t result = 0;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (!dispatcher_) {
      return 0;
    }
    void* arg = 0;
    result = dispatcher_->cancel(id, &arg);
    if (result) {
      ptr = static_cast<EventBase*>(arg);
    }
  }
  if (result) {
    handle_cancel_and_release(ptr);
  }
  return result;
}

size_t ServiceEventDispatcher::queue_size() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return dispatcher_ ? dispatcher_->queue_size() : 0;
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
