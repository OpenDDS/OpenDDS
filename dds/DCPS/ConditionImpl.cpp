/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ConditionImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void ConditionImpl::signal_all()
{
  if (DCPS_debug_level > 9) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ConditionImpl::signal_all()\n")));
  }

  if (!get_trigger_value()) return;

  WaitSetSet local_ws;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, g, lock_);
    local_ws = waitsets_;
  }

  if (DCPS_debug_level > 9) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ConditionImpl::signal_all() - ")
               ACE_TEXT("number of sets: %B\n"),
               local_ws.size()));
  }

  for (WaitSetSet::iterator it = local_ws.begin(), end = local_ws.end();
       it != end; ++it) {
    RcHandle<DDS::WaitSet> ws ((*it).lock());
    if (ws)
      ws->signal(this);
  }
}

DDS::ReturnCode_t ConditionImpl::attach_to_ws(DDS::WaitSet_ptr ws)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_,
                   DDS::RETCODE_OUT_OF_RESOURCES);
  return waitsets_.insert(OpenDDS::DCPS::WeakRcHandle<DDS::WaitSet>(*ws)).second
         ? DDS::RETCODE_OK : DDS::RETCODE_PRECONDITION_NOT_MET;
}

DDS::ReturnCode_t ConditionImpl::detach_from_ws(DDS::WaitSet_ptr ws)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_,
                   DDS::RETCODE_OUT_OF_RESOURCES);
  return waitsets_.erase(OpenDDS::DCPS::WeakRcHandle<DDS::WaitSet>(*ws))
         ? DDS::RETCODE_OK : DDS::RETCODE_PRECONDITION_NOT_MET;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
