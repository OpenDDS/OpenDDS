/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "StatusConditionImpl.h"
#include "EntityImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

CORBA::Boolean StatusConditionImpl::get_trigger_value()
{
  if (DCPS_debug_level > 9) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) StatusConditionImpl::get_trigger_value() - ")
               ACE_TEXT("mask==0x%x, changes==0x%x.\n"),
               this->mask_,
               this->parent_->get_status_changes()));
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_, false);
  return (parent_->get_status_changes() & mask_) > 0;
}

DDS::StatusMask StatusConditionImpl::get_enabled_statuses()
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_, 0);
  return mask_;
}

DDS::ReturnCode_t
StatusConditionImpl::set_enabled_statuses(DDS::StatusMask mask)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, g, lock_,
                   DDS::RETCODE_OUT_OF_RESOURCES);
  mask_ = mask;
  signal_all();
  return DDS::RETCODE_OK;
}

DDS::Entity_ptr StatusConditionImpl::get_entity()
{
  return DDS::Entity::_duplicate(parent_);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
