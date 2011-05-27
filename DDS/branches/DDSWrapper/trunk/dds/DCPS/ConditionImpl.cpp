#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ConditionImpl.h"

namespace OpenDDS { namespace DCPS {

void ConditionImpl::signal_all()
{
  if (!get_trigger_value()) return;

  WaitSetSet local_ws;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, g, lock_);
    local_ws = waitsets_;
  }

  for (WaitSetSet::iterator it = local_ws.begin(), end = local_ws.end();
    it != end; ++it)
    {
      (*it)->signal(this);
    }
}

DDS::ReturnCode_t ConditionImpl::attach_to_ws(DDS::WaitSet_ptr ws)
{
  DDS::WaitSet_var wsv(DDS::WaitSet::_duplicate(ws));
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_,
                   DDS::RETCODE_OUT_OF_RESOURCES);
  return waitsets_.insert(wsv).second
    ? DDS::RETCODE_OK : DDS::RETCODE_PRECONDITION_NOT_MET;
}

DDS::ReturnCode_t ConditionImpl::detach_from_ws(DDS::WaitSet_ptr ws)
{
  DDS::WaitSet_var wsv(DDS::WaitSet::_duplicate(ws));
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_,
                   DDS::RETCODE_OUT_OF_RESOURCES);
  return waitsets_.erase(wsv)
    ? DDS::RETCODE_OK : DDS::RETCODE_PRECONDITION_NOT_MET;
}


} }
