/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_GUARDCONDITION_H
#define OPENDDS_DCPS_GUARDCONDITION_H

#include "dds/DdsDcpsInfrastructureC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/LocalObject.h"
#include "dds/DCPS/ConditionImpl.h"

namespace DDS {

class GuardCondition;
typedef GuardCondition* GuardCondition_ptr;

class OpenDDS_Dcps_Export GuardCondition
  : public virtual OpenDDS::DCPS::LocalObject<DDS::GuardConditionInterf>
  , public virtual OpenDDS::DCPS::ConditionImpl {
public:
  GuardCondition()
    : trigger_value_(false)
  {}

  virtual ~GuardCondition() {}

  CORBA::Boolean get_trigger_value()
  ACE_THROW_SPEC((CORBA::SystemException));

  ReturnCode_t set_trigger_value(CORBA::Boolean value)
  ACE_THROW_SPEC((CORBA::SystemException));

  static GuardCondition_ptr _duplicate(GuardCondition_ptr obj);

private:
  CORBA::Boolean trigger_value_;
};

} // namespace DDS

namespace TAO {

template<>
struct OpenDDS_Dcps_Export Objref_Traits<DDS::GuardCondition> {
  static DDS::GuardCondition_ptr duplicate(DDS::GuardCondition_ptr p);
  static void release(DDS::GuardCondition_ptr p);
  static DDS::GuardCondition_ptr nil();
  static CORBA::Boolean marshal(const DDS::GuardCondition_ptr p,
                                TAO_OutputCDR & cdr);
};

} // namespace TAO

namespace DDS {

typedef TAO_Objref_Var_T<GuardCondition> GuardCondition_var;

} // namespace DDS

#endif
