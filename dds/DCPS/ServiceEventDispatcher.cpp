/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ServiceEventDispatcher.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

ServiceEventDispatcher::ServiceEventDispatcher(size_t count)
 : dispatcher_(make_rch<DispatchService>(count))
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
      EventBase* ptr = static_cast<EventBase*>(it->second);
      if (ptr) {
        ptr->handle_cancel();
        ptr->_remove_ref();
      }
    }
  }
}

bool ServiceEventDispatcher::dispatch(EventBase_rch event)
{
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
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!dispatcher_) {
    return 0;
  }
  void* arg = 0;
  const size_t result = dispatcher_->cancel(id, &arg);
  if (result) {
    EventBase* ptr = static_cast<EventBase*>(arg);
    if (ptr) {
      ptr->handle_cancel();
      ptr->_remove_ref();
    }
  }
  return result;
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
