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
  return reactor_;
}

ACE_INLINE const ACE_Reactor*
OpenDDS::DCPS::ReactorTask::get_reactor() const
{
  return reactor_;
}

ACE_INLINE ACE_thread_t
OpenDDS::DCPS::ReactorTask::get_reactor_owner() const
{
  return reactor_owner_;
}

ACE_INLINE ACE_Proactor*
OpenDDS::DCPS::ReactorTask::get_proactor()
{
  return proactor_;
}

ACE_INLINE const ACE_Proactor*
OpenDDS::DCPS::ReactorTask::get_proactor() const
{
  return proactor_;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
