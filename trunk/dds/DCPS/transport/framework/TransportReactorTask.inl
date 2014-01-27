/*
 * $Id$
 *
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

ACE_INLINE ACE_Proactor*
OpenDDS::DCPS::TransportReactorTask::get_proactor()
{
  DBG_ENTRY_LVL("TransportReactorTask","get_proactor",6);
  return this->proactor_;
}

ACE_INLINE const ACE_Proactor*
OpenDDS::DCPS::TransportReactorTask::get_proactor() const
{
  DBG_ENTRY_LVL("TransportReactorTask","get_proactor",6);
  return this->proactor_;
}
