/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TypeSupportImpl.h"

#include "Registered_Data_Types.h"
#include "Service_Participant.h"

#include "XTypes/TypeLookupService.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const ACE_CDR::Long TypeSupportImpl::TYPE_INFO_DEPENDENT_COUNT_NOT_PROVIDED = -1;

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

void TypeSupportImpl::to_type_info_i(XTypes::TypeIdentifierWithDependencies& ti_with_deps,
                                     const XTypes::TypeIdentifier& ti,
                                     const XTypes::TypeMap& type_map) const
{
  const XTypes::TypeMap::const_iterator pos = type_map.find(ti);

  if (pos == type_map.end()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: TypeSupportImpl::to_type_info_i, ")
               ACE_TEXT("%C TypeIdentifier (%C) of topic type not found in local type map.\n"),
               kind_to_string(ti.kind()), XTypes::equivalence_hash_to_string(ti.equivalence_hash()).c_str()));
    ti_with_deps.typeid_with_size.type_id = XTypes::TypeIdentifier();
    ti_with_deps.typeid_with_size.typeobject_serialized_size = 0;
  } else if (TheServiceParticipant->type_object_encoding() == Service_Participant::Encoding_WriteOldFormat) {
    Encoding encoding = XTypes::get_typeobject_encoding();
    encoding.skip_sequence_dheader(true);
    const XTypes::TypeObject& to = pos->second;
    ti_with_deps.typeid_with_size.type_id = makeTypeIdentifier(to, &encoding);
    const size_t sz = serialized_size(encoding, to);
    ti_with_deps.typeid_with_size.typeobject_serialized_size = static_cast<unsigned>(sz);
  } else {
    ti_with_deps.typeid_with_size.type_id = ti;
    const XTypes::TypeObject& to = pos->second;
    const size_t sz = serialized_size(XTypes::get_typeobject_encoding(), to);
    ti_with_deps.typeid_with_size.typeobject_serialized_size = static_cast<unsigned>(sz);
  }

  ti_with_deps.dependent_typeid_count = TYPE_INFO_DEPENDENT_COUNT_NOT_PROVIDED;
}

void TypeSupportImpl::to_type_info(XTypes::TypeInformation& type_info) const
{
  to_type_info_i(type_info.minimal, getMinimalTypeIdentifier(), getMinimalTypeMap());

  // Properly populate the complete member if complete TypeObjects are generated.
  const XTypes::TypeIdentifier& complete_ti = getCompleteTypeIdentifier();
  if (complete_ti.kind() != XTypes::TK_NONE) {
    to_type_info_i(type_info.complete, complete_ti, getCompleteTypeMap());
  } else {
    type_info.complete = XTypes::TypeIdentifierWithDependencies();
  }
}

void TypeSupportImpl::add_types(const RcHandle<XTypes::TypeLookupService>& tls) const
{
  using namespace XTypes;
  const TypeMap& minTypeMap = getMinimalTypeMap();
  tls->add(minTypeMap.begin(), minTypeMap.end());
  const TypeMap& comTypeMap = getCompleteTypeMap();
  tls->add(comTypeMap.begin(), comTypeMap.end());

  if (TheServiceParticipant->type_object_encoding() != Service_Participant::Encoding_Normal) {
    // In this mode we need to be able to recognize TypeIdentifiers received over the network
    // by peers that may have encoded them incorrectly.  Populate the TypeLookupService with
    // additional entries that map the alternate (wrong) TypeIdentifiers to the same TypeObjects.
    Encoding encoding = get_typeobject_encoding();
    encoding.skip_sequence_dheader(true);
    TypeMap altMinMap;
    for (TypeMap::const_iterator iter = minTypeMap.begin(); iter != minTypeMap.end(); ++iter) {
      const TypeObject& minTypeObject = iter->second;
      const TypeIdentifier typeId = makeTypeIdentifier(minTypeObject, &encoding);
      altMinMap[typeId] = minTypeObject;
    }
    tls->add(altMinMap.begin(), altMinMap.end());

    TypeMap altComMap;
    for (TypeMap::const_iterator iter = comTypeMap.begin(); iter != comTypeMap.end(); ++iter) {
      const TypeObject& comTypeObject = iter->second;
      const TypeIdentifier typeId = makeTypeIdentifier(comTypeObject, &encoding);
      altComMap[typeId] = comTypeObject;
    }
    tls->add(altComMap.begin(), altComMap.end());
  }
}

void TypeSupportImpl::populate_dependencies_i(const RcHandle<XTypes::TypeLookupService>& tls,
                                              XTypes::EquivalenceKind ek) const
{
  if (ek != XTypes::EK_MINIMAL && ek != XTypes::EK_COMPLETE) {
    return;
  }

  OPENDDS_SET(XTypes::TypeIdentifier) dependencies;
  const XTypes::TypeIdentifier& type_id = ek == XTypes::EK_MINIMAL ?
    getMinimalTypeIdentifier() : getCompleteTypeIdentifier();
  const XTypes::TypeMap& type_map = ek == XTypes::EK_MINIMAL ?
    getMinimalTypeMap() : getCompleteTypeMap();

  XTypes::compute_dependencies(type_map, type_id, dependencies);

  XTypes::TypeIdentifierWithSizeSeq deps_with_size;
  OPENDDS_SET(XTypes::TypeIdentifier)::const_iterator it = dependencies.begin();
  for (; it != dependencies.end(); ++it) {
    XTypes::TypeMap::const_iterator iter = type_map.find(*it);
    if (iter != type_map.end()) {
      const size_t tobj_size = serialized_size(XTypes::get_typeobject_encoding(), iter->second);
      XTypes::TypeIdentifierWithSize ti_with_size(*it, static_cast<ACE_CDR::ULong>(tobj_size));
      deps_with_size.append(ti_with_size);
    } else if (XTypes::has_type_object(*it)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: TypeSupportImpl::populate_dependencies, ")
                 ACE_TEXT("local %C TypeIdentifier (%C) not found in local type map.\n"),
                 kind_to_string(ek), XTypes::equivalence_hash_to_string(it->equivalence_hash()).c_str()));
    }
  }
  tls->add_type_dependencies(type_id, deps_with_size);
}

void TypeSupportImpl::populate_dependencies(const RcHandle<XTypes::TypeLookupService>& tls) const
{
  populate_dependencies_i(tls, XTypes::EK_MINIMAL);
  populate_dependencies_i(tls, XTypes::EK_COMPLETE);
}

const char* kind_to_string(const XTypes::EquivalenceKind ek)
{
  return ek == XTypes::EK_MINIMAL ? "minimal" : "complete";
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
