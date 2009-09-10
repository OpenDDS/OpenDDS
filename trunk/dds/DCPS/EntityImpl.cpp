/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "EntityImpl.h"
#include "StatusConditionImpl.h"

namespace OpenDDS {
namespace DCPS {

// Implementation skeleton constructor
EntityImpl::EntityImpl()
  : enabled_(false),
    entity_deleted_(false),
    status_changes_(0),
    status_condition_(new StatusConditionImpl(this))
{
}

// Implementation skeleton destructor
EntityImpl::~EntityImpl()
{
}

DDS::ReturnCode_t
EntityImpl::set_enabled()
{
  if (enabled_ == false) {
    enabled_ = true;
  }

  return DDS::RETCODE_OK;
}

bool
EntityImpl::is_enabled() const
{
  return this->enabled_.value();
}

DDS::StatusCondition_ptr
EntityImpl::get_statuscondition()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return DDS::StatusCondition::_duplicate(status_condition_);
}

DDS::StatusMask
EntityImpl::get_status_changes()
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, 0);
  return status_changes_;
}

void
EntityImpl::set_status_changed_flag(
  DDS::StatusKind status,
  bool status_changed_flag)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (status_changed_flag) {
    status_changes_ |= status;

  } else {
    status_changes_ &= ~status;
  }
}

void
EntityImpl::set_deleted(bool state)
{
  if (entity_deleted_ != state) {
    entity_deleted_ = state;
  }
}

bool
EntityImpl::get_deleted()
{
  bool deleted_state = true;

  if (entity_deleted_ != true) {
    deleted_state = false;
  }

  return deleted_state;
}

void
EntityImpl::notify_status_condition()
{
  dynamic_cast<StatusConditionImpl*>(status_condition_.in())->signal_all();
}

} // namespace DCPS
} // namespace OpenDDS
