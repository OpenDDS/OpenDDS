/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "SecurityConfig.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

SecurityConfig::SecurityConfig(const OPENDDS_STRING& name,
                               Authentication_var authentication_plugin,
                               AccessControl_var access_ctrl_plugin,
                               CryptoKeyExchange_var key_exchange_plugin,
                               CryptoKeyFactory_var key_factory_plugin,
                               CryptoTransform_var transform_plugin,
                               const ConfigPropertyList& properties)
  : name_(name)
  , authentication_plugin_(authentication_plugin)
  , access_control_plugin_(access_ctrl_plugin)
  , key_exchange_plugin_(key_exchange_plugin)
  , key_factory_plugin_(key_factory_plugin)
  , transform_plugin_(transform_plugin)
  , properties_(properties)
{}

SecurityConfig::~SecurityConfig()
{}

void SecurityConfig::get_properties(DDS::Security::PropertyQosPolicy& out_properties) const
{
  // The names on the internal attributes of the PropertyQosPolicy are
  // strange, but value is a properties sequence

  // Allocate space and copy over the properties
  // Presumably this will not be truncating the property count
  out_properties.value.length(static_cast<CORBA::ULong>(properties_.size()));
  CORBA::ULong index = 0;
  for (ConfigPropertyList::const_iterator iProp = properties_.begin();
       iProp != properties_.end();
       ++iProp) {
    DDS::Property_t& out_prop = out_properties.value[index++];
    out_prop.name = iProp->first.c_str();
    out_prop.value = iProp->second.c_str();
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
