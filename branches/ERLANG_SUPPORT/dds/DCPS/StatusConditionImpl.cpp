#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "StatusConditionImpl.h"
#include "EntityImpl.h"

namespace OpenDDS { namespace DCPS {

CORBA::Boolean StatusConditionImpl::get_trigger_value()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_, false);
  return (parent_->get_status_changes() & mask_) > 0;
}

DDS::StatusKindMask StatusConditionImpl::get_enabled_statuses()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_, 0);
  return mask_;
}

DDS::ReturnCode_t
StatusConditionImpl::set_enabled_statuses(DDS::StatusKindMask mask)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_,
                   DDS::RETCODE_OUT_OF_RESOURCES);
  mask_ = mask;
  signal_all();
  return DDS::RETCODE_OK;
}

DDS::Entity_ptr StatusConditionImpl::get_entity()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  return DDS::Entity::_duplicate(parent_);
}

} }
