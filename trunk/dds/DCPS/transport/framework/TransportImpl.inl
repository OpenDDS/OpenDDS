/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransportInst.h"
#include "TransportReactorTask.h"
#include "DataLink_rch.h"
#include "DataLink.h"
#include "EntryExit.h"

#include "ace/Reactor.h"

ACE_INLINE
OpenDDS::DCPS::TransportInst*
OpenDDS::DCPS::TransportImpl::config() const
{
  return this->config_.in();
}

/// NOTE: Should only be called if this->lock_ has already been acquired.
//MJM: I am not convinced that this needs to be guarded by the caller.
//MJM: He gets a current snapshot of the value.  If it changes, his copy
//MJM: is stale.  Or do you mean that his stale copy may be stopped if
//MJM: his _use_ of the reactor task is not guarded?
ACE_INLINE OpenDDS::DCPS::TransportReactorTask*
OpenDDS::DCPS::TransportImpl::reactor_task()
{
  DBG_ENTRY_LVL("TransportImpl","reactor_task",6);
  TransportReactorTask_rch task = this->reactor_task_;
  return task._retn();
}

ACE_INLINE ACE_Reactor_Timer_Interface*
OpenDDS::DCPS::TransportImpl::timer() const
{
  TransportReactorTask_rch task = this->reactor_task_;
  return task.is_nil() ? 0 : task->get_reactor();
}

ACE_INLINE bool
OpenDDS::DCPS::TransportImpl::connection_info
  (TransportLocator& local_info) const
{
  return this->connection_info_i(local_info);
}


ACE_INLINE void
OpenDDS::DCPS::TransportImpl::pre_shutdown_i()
{
  //noop
}

ACE_INLINE int
OpenDDS::DCPS::TransportImpl::acquire()
{
  int rv = reservation_lock_.acquire();
  if (rv == 0) {
    rlock_thread_id_ = ACE_OS::thr_self();
  }
  return rv;
}

ACE_INLINE int
OpenDDS::DCPS::TransportImpl::tryacquire()
{
  return reservation_lock_.tryacquire();
}

ACE_INLINE int
OpenDDS::DCPS::TransportImpl::release()
{
  // Because of the current design, we may have already
  // released this lock.  Looking at the rlock_thread_id_
  // data member should tell us whether we own the lock.

  if (rlock_thread_id_ == ACE_OS::thr_self()) {
    // we own it, clear the thread id and release
    rlock_thread_id_ = 0;
    return reservation_lock_.release();
  }
  // we don't own it, do nothing
  return 0;
}

ACE_INLINE int
OpenDDS::DCPS::TransportImpl::remove()
{
  return reservation_lock_.remove();
}
