/*
 *
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

#include "dds/DCPS/ConditionImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {

class GuardCondition;
typedef GuardCondition* GuardCondition_ptr;
typedef TAO_Objref_Var_T<GuardCondition> GuardCondition_var;

class OpenDDS_Dcps_Export GuardCondition
  : public virtual DDS::GuardConditionInterf
  , public virtual OpenDDS::DCPS::ConditionImpl {
public:
  typedef GuardCondition* _ptr_type;
  typedef GuardCondition_var _var_type;

  GuardCondition()
    : trigger_value_(false)
  {}

  virtual ~GuardCondition() {}

  CORBA::Boolean get_trigger_value();

  ReturnCode_t set_trigger_value(CORBA::Boolean value);

  static GuardCondition_ptr _duplicate(GuardCondition_ptr obj);
  static GuardCondition_ptr _narrow(CORBA::Object_ptr obj);

private:
  CORBA::Boolean trigger_value_;
};

} // namespace DDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

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

TAO_END_VERSIONED_NAMESPACE_DECL

#endif
