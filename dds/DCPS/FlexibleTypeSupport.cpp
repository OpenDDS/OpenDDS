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
  return XTypes::TypeIdentifier::None;
}

const XTypes::TypeMap& FlexibleTypeSupport::getMinimalTypeMap() const
{
  return XTypes::TypeMapBuilder::EmptyMap;
}

const XTypes::TypeIdentifier& FlexibleTypeSupport::getCompleteTypeIdentifier() const
{
  return XTypes::TypeIdentifier::None;
}

const XTypes::TypeMap& FlexibleTypeSupport::getCompleteTypeMap() const
{
  return XTypes::TypeMapBuilder::EmptyMap;
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
