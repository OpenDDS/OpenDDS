/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/Reactor.h"
#include "EntryExit.h"

ACE_INLINE ACE_Reactor*
OpenDDS::DCPS::TransportReactorTask::get_reactor()
{
  DBG_ENTRY_LVL("TransportReactorTask","get_reactor",6);
  return this->reactor_;
}

ACE_INLINE const ACE_Reactor*
OpenDDS::DCPS::TransportReactorTask::get_reactor() const
{
  DBG_ENTRY_LVL("TransportReactorTask","get_reactor",6);
  return this->reactor_;
}
