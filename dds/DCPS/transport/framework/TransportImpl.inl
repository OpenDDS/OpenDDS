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
  return reservation_lock_.acquire();
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
  // released this lock.  There are three possibilities
  // for what state we can be in:
  // 1) We took the lock and need to release it
  // 2) We took the lock, released it, and nobody owns
  //    it.
  // 3) We took the lock, released it, and somebody else
  //    owns it.
  //
  // Given the existing mutex APIs and portability
  // concerns, the best approach is to just call release
  // and swallow some of the return values.

  int rv = reservation_lock_.release();

  if (DCPS::DCPS_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TransportImpl::release() - ")
               ACE_TEXT("attempting release, rv=%d\n"), rv));
  }

  switch (rv) {
  case 0:      // Case 1
  case EAGAIN: // Case 2?
  case EPERM:  // Case 3
    return 0;
  default:
    return rv;
  }
}

ACE_INLINE int
OpenDDS::DCPS::TransportImpl::remove()
{
  return reservation_lock_.remove();
}
