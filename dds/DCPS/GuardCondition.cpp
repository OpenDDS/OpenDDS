/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "GuardCondition.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {

CORBA::Boolean GuardCondition::get_trigger_value()
{
  return trigger_value_;
}

ReturnCode_t GuardCondition::set_trigger_value(CORBA::Boolean value)
{
  trigger_value_ = value;
  signal_all();
  return RETCODE_OK;
}

GuardCondition_ptr GuardCondition::_duplicate(GuardCondition_ptr obj)
{
  if (!CORBA::is_nil(obj)) obj->_add_ref();

  return obj;
}

GuardCondition_ptr GuardCondition::_narrow(CORBA::Object_ptr obj)
{
  GuardConditionInterf_var gci = GuardConditionInterf::_narrow(obj);
  const GuardCondition_ptr gc = dynamic_cast<GuardCondition_ptr>(gci.in());
  if (gc) {
    gci._retn();
    return gc;
  }
  return 0;
}

} // namespace DDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

DDS::GuardCondition_ptr
TAO::Objref_Traits<DDS::GuardCondition>::duplicate(DDS::GuardCondition_ptr p)
{
  return DDS::GuardCondition::_duplicate(p);
}

void
TAO::Objref_Traits<DDS::GuardCondition>::release(DDS::GuardCondition_ptr p)
{
  CORBA::release(p);
}

DDS::GuardCondition_ptr
TAO::Objref_Traits<DDS::GuardCondition>::nil()
{
  return static_cast<DDS::GuardCondition_ptr>(0);
}

CORBA::Boolean
TAO::Objref_Traits<DDS::GuardCondition>::marshal(
  const DDS::GuardCondition_ptr p,
  TAO_OutputCDR& cdr)
{
  return CORBA::Object::marshal(p, cdr);
}

