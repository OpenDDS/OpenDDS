/*
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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_INLINE OpenDDS::DCPS::TransportInst_rch
OpenDDS::DCPS::TransportImpl::config() const
{
  return this->config_;
}

ACE_INLINE OpenDDS::DCPS::TransportReactorTask_rch
OpenDDS::DCPS::TransportImpl::reactor_task()
{
  DBG_ENTRY_LVL("TransportImpl","reactor_task",6);
  return this->reactor_task_;
}

ACE_INLINE ACE_Reactor_Timer_Interface*
OpenDDS::DCPS::TransportImpl::timer() const
{
  return reactor();
}

ACE_INLINE ACE_Reactor*
OpenDDS::DCPS::TransportImpl::reactor() const
{
  TransportReactorTask_rch task = this->reactor_task_;
  return task.is_nil() ? 0 : task->get_reactor();
}

ACE_INLINE ACE_thread_t
OpenDDS::DCPS::TransportImpl::reactor_owner() const
{
  return this->reactor_task_->get_reactor_owner();
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

OPENDDS_END_VERSIONED_NAMESPACE_DECL
