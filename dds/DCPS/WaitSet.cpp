/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "WaitSet.h"
#include "ConditionImpl.h"
#include "Qos_Helper.h"

#include "ace/OS_NS_sys_time.h"

namespace {

void copyInto(DDS::ConditionSeq& target,
              const DDS::WaitSet::ConditionSet& source)
{
  size_t size = source.size();
  target.length(static_cast<CORBA::ULong>(size));
  CORBA::ULong index = 0;

  for (DDS::WaitSet::ConditionSet::const_iterator iter = source.begin(),
       end = source.end(); iter != end; ++iter, ++index) {
    target[index] = *iter;
  }
}

} // namespace

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {

DDS::ReturnCode_t WaitSet::attach_condition(Condition_ptr cond)
{
  using OpenDDS::DCPS::ConditionImpl;
  Condition_var condv(Condition::_duplicate(cond));

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_,
                   RETCODE_OUT_OF_RESOURCES);
  ConditionImpl* ci = dynamic_cast<ConditionImpl*>(cond);

  if (!ci) return RETCODE_BAD_PARAMETER;

  ReturnCode_t ret = ci->attach_to_ws(this);

  if (ret == RETCODE_OK) {
    attached_conditions_.insert(condv);

    if (condv->get_trigger_value()) signal(condv.in());

    return RETCODE_OK;

  } else if (ret == RETCODE_PRECONDITION_NOT_MET) {
    // RETCODE_PRECONDITION_NOT_MET means it was already in the set
    return RETCODE_OK;
  }

  return ret;
}

ReturnCode_t WaitSet::detach_condition(Condition_ptr cond)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_,
                   RETCODE_OUT_OF_RESOURCES);
  return detach_i(cond);
}

ReturnCode_t WaitSet::detach_conditions(const ConditionSeq& conds)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_,
                   RETCODE_OUT_OF_RESOURCES);

  for (CORBA::ULong i = 0; i < conds.length(); ++i) {
    ReturnCode_t ret = detach_i(conds[ i]);

    if (ret != RETCODE_OK) {
      return ret;
    }
  }

  return RETCODE_OK;
}

ReturnCode_t WaitSet::detach_i(const Condition_ptr cond)
{
  using OpenDDS::DCPS::ConditionImpl;
  Condition_var condv(Condition::_duplicate(cond));

  ConditionImpl* ci = dynamic_cast<ConditionImpl*>(cond);

  if (!ci) return RETCODE_BAD_PARAMETER;

  ReturnCode_t ret = ci->detach_from_ws(this);
  attached_conditions_.erase(condv);
  signaled_conditions_.erase(condv);
  return ret;
}

ReturnCode_t WaitSet::get_conditions(ConditionSeq& conds)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_,
                   RETCODE_OUT_OF_RESOURCES);
  copyInto(conds, attached_conditions_);
  return RETCODE_OK;
}

ReturnCode_t WaitSet::wait(ConditionSeq& active_conditions,
                           const Duration_t& timeout)
{
  if (waiting_.value()) return RETCODE_PRECONDITION_NOT_MET;

  if (!OpenDDS::DCPS::non_negative_duration(timeout))
    return DDS::RETCODE_BAD_PARAMETER;

  ACE_Time_Value deadline;
  ACE_Time_Value* p_deadline = 0;

  if (timeout.sec != DURATION_INFINITE_SEC ||
      timeout.nanosec != DURATION_INFINITE_NSEC) {
    deadline = OpenDDS::DCPS::duration_to_absolute_time_value(timeout);
    p_deadline = &deadline;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_,
                   RETCODE_OUT_OF_RESOURCES);
  waiting_ = 1;
  signaled_conditions_.clear();

  for (ConditionSet::const_iterator iter = attached_conditions_.begin(),
       end = attached_conditions_.end(); iter != end; ++iter) {
    if ((*iter)->get_trigger_value()) {
      signaled_conditions_.insert(*iter);
    }
  }

  int error = 0;

  while ((attached_conditions_.empty() || signaled_conditions_.empty())
         && !error) {
    if (cond_.wait(p_deadline) == -1) error = errno;
  }

  copyInto(active_conditions, signaled_conditions_);
  signaled_conditions_.clear();
  waiting_ = 0;

  switch (error) {
  case 0:
    return RETCODE_OK;
  case ETIME:
    return RETCODE_TIMEOUT;
  default:
    return RETCODE_ERROR;
  }
}

void WaitSet::signal(Condition_ptr condition)
{
  Condition_var condv(Condition::_duplicate(condition));
  ACE_GUARD(ACE_Recursive_Thread_Mutex, g, lock_);

  if (attached_conditions_.find(condv) != attached_conditions_.end()) {
    signaled_conditions_.insert(condv);
    cond_.signal();
  }
}

WaitSet_ptr WaitSet::_duplicate(WaitSet_ptr obj)
{
  if (!CORBA::is_nil(obj)) obj->_add_ref();

  return obj;
}

} // namespace DDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

DDS::WaitSet_ptr
TAO::Objref_Traits<DDS::WaitSet>::duplicate(DDS::WaitSet_ptr p)
{
  return DDS::WaitSet::_duplicate(p);
}

void
TAO::Objref_Traits<DDS::WaitSet>::release(DDS::WaitSet_ptr p)
{
  CORBA::release(p);
}

DDS::WaitSet_ptr
TAO::Objref_Traits<DDS::WaitSet>::nil()
{
  return static_cast<DDS::WaitSet_ptr>(0);
}

CORBA::Boolean
TAO::Objref_Traits<DDS::WaitSet>::marshal(const DDS::WaitSet_ptr p,
                                          TAO_OutputCDR& cdr)
{
  return CORBA::Object::marshal(p, cdr);
}
