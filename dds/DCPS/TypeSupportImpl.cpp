/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

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
    (!type_name || !type_name[0]) ? name() : type_name;
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
  CORBA::String_var type = name();
  return type._retn();
}

namespace {
  void log_ti_not_found(const char* method_name, const char* name, const XTypes::TypeIdentifier& ti)
  {
    if (log_level >= LogLevel::Error) {
      const XTypes::EquivalenceKind ek = ti.kind();
      String str;
      if (ek == XTypes::EK_COMPLETE || ek == XTypes::EK_MINIMAL) {
        str = ek == XTypes::EK_MINIMAL ? "minimal " : "complete ";
        str += XTypes::equivalence_hash_to_string(ti.equivalence_hash());
      } else {
        str = "not an equivalence hash";
      }
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TypeSupportImpl::%C: "
                 "TypeIdentifier \"%C\" of topic type \"%C\" not found in local type map.\n",
                 method_name, str.c_str(), name));
    }
  }
}

void TypeSupportImpl::to_type_info_i(XTypes::TypeIdentifierWithDependencies& ti_with_deps,
                                     const XTypes::TypeIdentifier& ti,
                                     const XTypes::TypeMap& type_map) const
{
  const XTypes::TypeMap::const_iterator pos = type_map.find(ti);

  if (pos == type_map.end()) {
    log_ti_not_found("to_type_info_i", name(), ti);
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

void TypeSupportImpl::add_types(const XTypes::TypeLookupService_rch& tls) const
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

void TypeSupportImpl::populate_dependencies_i(const XTypes::TypeLookupService_rch& tls,
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
      log_ti_not_found("populate_dependencies_i", name(), *it);
    }
  }
  tls->add_type_dependencies(type_id, deps_with_size);
}

void TypeSupportImpl::populate_dependencies(const XTypes::TypeLookupService_rch& tls) const
{
  populate_dependencies_i(tls, XTypes::EK_MINIMAL);
  populate_dependencies_i(tls, XTypes::EK_COMPLETE);
}

#ifndef OPENDDS_SAFETY_PROFILE
DDS::DynamicType_ptr TypeSupportImpl::get_type_from_type_lookup_service()
{
  if (!type_lookup_service_) {
    type_lookup_service_ = make_rch<XTypes::TypeLookupService>();
    add_types(type_lookup_service_);
    populate_dependencies(type_lookup_service_);
  }

  const XTypes::TypeIdentifier& cti = getCompleteTypeIdentifier();
  const XTypes::TypeMap& ctm = getCompleteTypeMap();
  const XTypes::TypeIdentifier& mti = getMinimalTypeIdentifier();
  const XTypes::TypeMap& mtm = getMinimalTypeMap();
  XTypes::DynamicTypeImpl* dt = dynamic_cast<XTypes::DynamicTypeImpl*>(
    type_lookup_service_->type_identifier_to_dynamic(cti, GUID_UNKNOWN));
  if (dt) {
    dt->set_complete_type_identifier(cti);
    dt->set_complete_type_map(ctm);
    dt->set_minimal_type_identifier(mti);
    dt->set_minimal_type_map(mtm);
  }
  return dt;
}
#endif

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
