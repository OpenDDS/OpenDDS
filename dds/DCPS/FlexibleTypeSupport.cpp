/*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "FlexibleTypeSupport.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

char* FlexibleTypeSupport::get_type_name()
{
  return CORBA::string_dup(name_.c_str());
}

DDS::ReturnCode_t FlexibleTypeSupport::add(const String& typeKey,
                                           const XTypes::TypeIdentifier& minimalTypeIdentifier,
                                           const XTypes::TypeMap& minimalTypeMap,
                                           const XTypes::TypeIdentifier& completeTypeIdentifier,
                                           const XTypes::TypeMap& completeTypeMap)
{
  map_[typeKey] = Alternative(minimalTypeIdentifier, minimalTypeMap, completeTypeIdentifier, completeTypeMap);
  return DDS::RETCODE_OK;
}

void FlexibleTypeSupport::to_type_info(TypeInformation& type_info) const
{
  type_info.flags_ = TypeInformation::Flags_FlexibleTypeSupport;
}

const XTypes::TypeIdentifier& FlexibleTypeSupport::getMinimalTypeIdentifier() const
{
  return base_->getMinimalTypeIdentifier();
}

const XTypes::TypeMap& FlexibleTypeSupport::getMinimalTypeMap() const
{
  return base_->getMinimalTypeMap();
}

const XTypes::TypeIdentifier& FlexibleTypeSupport::getCompleteTypeIdentifier() const
{
  return base_->getCompleteTypeIdentifier();
}

const XTypes::TypeMap& FlexibleTypeSupport::getCompleteTypeMap() const
{
  return base_->getCompleteTypeMap();
}

void FlexibleTypeSupport::get_flexible_types(const char* key, XTypes::TypeInformation& type_info)
{
  const OPENDDS_MAP(String, Alternative)::const_iterator iter = map_.find(key);
  if (iter == map_.end()) {
    TypeInformation dcps_type_info;
    TypeSupportImpl::to_type_info(dcps_type_info);
    type_info = dcps_type_info.xtypes_type_info_;
    return;
  }
  const Alternative& alt = iter->second;
  to_type_info_i(type_info.minimal, alt.minimalId_, alt.minimalMap_);
  if (alt.completeId_.kind() != XTypes::TK_NONE) {
    to_type_info_i(type_info.complete, alt.completeId_, alt.completeMap_);
  }
}

void FlexibleTypeSupport::add_types(const RcHandle<XTypes::TypeLookupService>& tls) const
{
  for (MapType::const_iterator pos = map_.begin(), limit = map_.end(); pos != limit; ++pos) {
    {
      XTypes::TypeMap type_map = getMinimalTypeMap();
      type_map.insert(pos->second.minimalMap_.begin(), pos->second.minimalMap_.end());
      populate_dependencies_i(tls, pos->second.minimalId_, type_map);
    }
    {
      XTypes::TypeMap type_map = getCompleteTypeMap();
      type_map.insert(pos->second.completeMap_.begin(), pos->second.completeMap_.end());
      populate_dependencies_i(tls, pos->second.completeId_, type_map);
    }
  }
  base_->add_types(tls);
}

void FlexibleTypeSupport::populate_dependencies_i(const XTypes::TypeLookupService_rch& tls,
                                                  const XTypes::TypeIdentifier& type_id,
                                                  const XTypes::TypeMap& type_map) const
{
  OPENDDS_SET(XTypes::TypeIdentifier) dependencies;
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: FlexibleTypeSupport::populate_dependencies_io, ")
                 ACE_TEXT("local TypeIdentifier (%C) not found in local type map.\n"),
                 XTypes::equivalence_hash_to_string(it->equivalence_hash()).c_str()));
    }
  }
  tls->add_type_dependencies(type_id, deps_with_size);
}

FlexibleTypeSupport::Alternative::Alternative(const XTypes::TypeIdentifier& minimalTypeIdentifier,
                                              const XTypes::TypeMap& minimalTypeMap,
                                              const XTypes::TypeIdentifier& completeTypeIdentifier,
                                              const XTypes::TypeMap& completeTypeMap)
  : minimalId_(minimalTypeIdentifier)
  , completeId_(completeTypeIdentifier)
  , minimalMap_(minimalTypeMap)
  , completeMap_(completeTypeMap)
{
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
