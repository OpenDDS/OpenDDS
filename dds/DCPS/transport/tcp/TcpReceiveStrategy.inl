/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_INLINE ACE_Reactor*
OpenDDS::DCPS::TcpReceiveStrategy::get_reactor()
{
  DBG_ENTRY_LVL("TcpReceiveStrategy","get_reactor",6);
  return this->reactor_task_->get_reactor();
}

ACE_INLINE bool
OpenDDS::DCPS::TcpReceiveStrategy::gracefully_disconnected()
{
  return this->gracefully_disconnected_;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
