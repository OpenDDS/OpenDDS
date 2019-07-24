/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/Reactor.h"
#include "debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_INLINE ACE_Reactor*
OpenDDS::DCPS::ReactorTask::get_reactor()
{
  return this->reactor_;
}

ACE_INLINE const ACE_Reactor*
OpenDDS::DCPS::ReactorTask::get_reactor() const
{
  return this->reactor_;
}

ACE_INLINE ACE_thread_t
OpenDDS::DCPS::ReactorTask::get_reactor_owner() const
{
  return this->reactor_owner_;
}

ACE_INLINE ACE_Proactor*
OpenDDS::DCPS::ReactorTask::get_proactor()
{
  return this->proactor_;
}

ACE_INLINE const ACE_Proactor*
OpenDDS::DCPS::ReactorTask::get_proactor() const
{
  return this->proactor_;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
