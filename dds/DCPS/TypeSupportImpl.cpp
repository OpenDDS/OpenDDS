/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TypeSupportImpl.h"

#include "Registered_Data_Types.h"
#include "TypeObject.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TypeSupportImpl::~TypeSupportImpl()
{}

DDS::ReturnCode_t
TypeSupportImpl::register_type(DDS::DomainParticipant_ptr participant,
                               const char* type_name)
{
  const char* const type =
    (!type_name || !type_name[0]) ? default_type_name() : type_name;
  return Registered_Data_Types->register_type(participant, type, this);
}

DDS::ReturnCode_t
TypeSupportImpl::unregister_type(DDS::DomainParticipant_ptr participant,
    const char* type_name)
{
  if (type_name == 0 || type_name[0] == '\0') {
    return DDS::RETCODE_BAD_PARAMETER;
  } else {
    return Registered_Data_Types->unregister_type(participant, type_name, this);
  }
}

char*
TypeSupportImpl::get_type_name()
{
  CORBA::String_var type = default_type_name();
  return type._retn();
}

void TypeSupportImpl::to_type_info(XTypes::TypeInformation& type_info) const
{
  const XTypes::TypeObject& minTypeObject = getMinimalTypeObject();
  type_info.minimal.typeid_with_size.type_id = XTypes::makeTypeIdentifier(minTypeObject);
  type_info.minimal.typeid_with_size.typeobject_serialized_size =
    serialized_size(XTypes::get_typeobject_encoding(), minTypeObject);
  type_info.minimal.dependent_typeid_count = 0;
  type_info.complete.typeid_with_size.typeobject_serialized_size = 0;
  type_info.complete.dependent_typeid_count = 0;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
