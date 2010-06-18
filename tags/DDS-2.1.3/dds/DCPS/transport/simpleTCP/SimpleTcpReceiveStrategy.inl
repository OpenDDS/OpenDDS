/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE ACE_Reactor*
OpenDDS::DCPS::SimpleTcpReceiveStrategy::get_reactor()
{
  DBG_ENTRY_LVL("SimpleTcpReceiveStrategy","get_reactor",6);
  return this->reactor_task_->get_reactor();
}

ACE_INLINE bool
OpenDDS::DCPS::SimpleTcpReceiveStrategy::gracefully_disconnected()
{
  return this->gracefully_disconnected_;
}
