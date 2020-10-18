/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TypeSupportImpl.h"

#include "Registered_Data_Types.h"
#include "XTypes/TypeLookupService.h"

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
  using namespace XTypes;
  const TypeIdentifier& minTypeId = getMinimalTypeIdentifier();
  const TypeMap& minTypeMap = getMinimalTypeMap();
  const TypeMap::const_iterator pos = minTypeMap.find(minTypeId);

  if (pos == minTypeMap.end()) {
    type_info.minimal.typeid_with_size.type_id = TypeIdentifier();
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;

  } else {
    const TypeObject& minTypeObject = pos->second;
    type_info.minimal.typeid_with_size.type_id = minTypeId;
    type_info.minimal.typeid_with_size.typeobject_serialized_size =
      serialized_size(get_typeobject_encoding(), minTypeObject);
  }

  type_info.minimal.dependent_typeid_count = 0;
  type_info.complete.typeid_with_size.typeobject_serialized_size = 0;
  type_info.complete.dependent_typeid_count = 0;
}

void TypeSupportImpl::populate_dependencies(const RcHandle<XTypes::TypeLookupService>& tls) const
{
  OPENDDS_SET(XTypes::TypeIdentifier) dependencies;
  const XTypes::TypeMap& type_map = getMinimalTypeMap();
  XTypes::compute_dependencies(type_map, getMinimalTypeIdentifier(), dependencies);

  XTypes::TypeIdentifierWithSizeSeq deps_with_size;
  OPENDDS_SET(XTypes::TypeIdentifier)::const_iterator it = dependencies.begin();
  for (; it != dependencies.end(); ++it) {
    XTypes::TypeMap::const_iterator iter = type_map.find(*it);
    if (iter != type_map.end()) {
      const size_t tobj_size = serialized_size(XTypes::get_typeobject_encoding(), iter->second);
      XTypes::TypeIdentifierWithSize tmp = {*it, static_cast<ACE_CDR::ULong>(tobj_size)};
      deps_with_size.append(tmp);
    } else if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: TypeSupportImpl::populate_dependencies, ")
                 ACE_TEXT("local TypeIdentifier not found in local type map.\n")));
    }
  }

  tls->add_type_dependencies(getMinimalTypeIdentifier(), deps_with_size);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
