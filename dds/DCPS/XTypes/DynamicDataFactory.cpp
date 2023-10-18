/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicDataFactory.h"

#  include "DynamicDataImpl.h"

#include <ace/Singleton.h>

using namespace OpenDDS::DCPS;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace DDS {

DynamicDataFactory_ptr DynamicDataFactory::get_instance()
{
  return ACE_Singleton<DynamicDataFactory, ACE_SYNCH_MUTEX>::instance();
}

ReturnCode_t DynamicDataFactory::delete_instance()
{
  return RETCODE_OK;
}

DynamicData_ptr DynamicDataFactory::create_data(DynamicType_ptr type)
{
  return new OpenDDS::XTypes::DynamicDataImpl(type);
}

DynamicDataFactory_ptr DynamicDataFactory::_duplicate(DynamicDataFactory_ptr obj)
{
  if (obj) {
    obj->_add_ref();
  }
  return obj;
}

} // namespace DDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

TAO_BEGIN_VERSIONED_NAMESPACE_DECL
namespace TAO {

DDS::DynamicDataFactory_ptr Objref_Traits<DDS::DynamicDataFactory>::duplicate(DDS::DynamicDataFactory_ptr p)
{
  return DDS::DynamicDataFactory::_duplicate(p);
}

void Objref_Traits<DDS::DynamicDataFactory>::release(DDS::DynamicDataFactory_ptr p)
{
  CORBA::release(p);
}

DDS::DynamicDataFactory_ptr Objref_Traits<DDS::DynamicDataFactory>::nil()
{
  return static_cast<DDS::DynamicDataFactory_ptr>(0);
}

CORBA::Boolean Objref_Traits<DDS::DynamicDataFactory>::marshal(
  const DDS::DynamicDataFactory_ptr, TAO_OutputCDR&)
{
  return false;
}

} // namespace TAO
TAO_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
