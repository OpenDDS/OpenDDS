/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_STATUSCONDITIONIMPL_H
#define OPENDDS_DCPS_STATUSCONDITIONIMPL_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/ConditionImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class EntityImpl;

class StatusConditionImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::StatusCondition>
  , public virtual ConditionImpl {
public:
  explicit StatusConditionImpl(EntityImpl* e)
    : parent_(e)
    , mask_(0xFFFFFFFF)
  {}

  virtual ~StatusConditionImpl() {}

  CORBA::Boolean get_trigger_value()
  ACE_THROW_SPEC((CORBA::SystemException));

  DDS::StatusMask get_enabled_statuses()
  ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t set_enabled_statuses(DDS::StatusMask mask)
  ACE_THROW_SPEC((CORBA::SystemException));

  DDS::Entity_ptr get_entity()
  ACE_THROW_SPEC((CORBA::SystemException));

private:
  //deliberately not a _var, don't hold a reference to the parent since
  //it is guaranteed to outlive us and we don't want a cyclical reference
  EntityImpl* parent_;
  DDS::StatusMask mask_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif
