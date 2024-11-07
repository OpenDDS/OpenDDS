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

DDS::ReturnCode_t FlexibleTypeSupport::add(const RcHandle<TypeSupportImpl>& alternativeTypeSupport)
{
  map_[alternativeTypeSupport->name()] = alternativeTypeSupport;
  return DDS::RETCODE_OK;
}

void FlexibleTypeSupport::to_type_info(TypeInformation& type_info) const
{
  type_info.flags_ = TypeInformation::Flags_FlexibleTypeSupport;
}

namespace {
  const XTypes::TypeIdentifier tiEmpty;
  const XTypes::TypeMap tmEmpty;
}

const XTypes::TypeIdentifier& FlexibleTypeSupport::getMinimalTypeIdentifier() const
{
  return tiEmpty;
}

const XTypes::TypeMap& FlexibleTypeSupport::getMinimalTypeMap() const
{
  return tmEmpty;
}

const XTypes::TypeIdentifier& FlexibleTypeSupport::getCompleteTypeIdentifier() const
{
  return tiEmpty;
}

const XTypes::TypeMap& FlexibleTypeSupport::getCompleteTypeMap() const
{
  return tmEmpty;
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
