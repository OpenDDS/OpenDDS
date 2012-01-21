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

ACE_INLINE OpenDDS::DCPS::TransportImpl::ReservationLockType&
OpenDDS::DCPS::TransportImpl::reservation_lock()
{
  DBG_ENTRY_LVL("TransportImpl","reservation_lock",6);
  return this->reservation_lock_;
}

ACE_INLINE const OpenDDS::DCPS::TransportImpl::ReservationLockType&
OpenDDS::DCPS::TransportImpl::reservation_lock() const
{
  DBG_ENTRY_LVL("TransportImpl","reservation_lock",6);
  return this->reservation_lock_;
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
