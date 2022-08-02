/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "SecurityConfig.h"

#include "Properties.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

SecurityConfig::SecurityConfig(const OPENDDS_STRING& name,
#ifdef OPENDDS_SECURITY
                               Authentication_var authentication_plugin,
                               AccessControl_var access_ctrl_plugin,
                               CryptoKeyExchange_var key_exchange_plugin,
                               CryptoKeyFactory_var key_factory_plugin,
                               CryptoTransform_var transform_plugin,
                               DCPS::RcHandle<Utility> utility_plugin,
#endif
                               const ConfigPropertyList& properties)
  : name_(name)
#ifdef OPENDDS_SECURITY
  , authentication_plugin_(authentication_plugin)
  , access_control_plugin_(access_ctrl_plugin)
  , key_exchange_plugin_(key_exchange_plugin)
  , key_factory_plugin_(key_factory_plugin)
  , transform_plugin_(transform_plugin)
  , utility_plugin_(utility_plugin)
#endif
  , properties_(properties)
{}

SecurityConfig::~SecurityConfig()
{
#ifdef OPENDDS_SECURITY
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("SecurityConfig::~SecurityConfig handle_registry_map_ %B\n"),
               handle_registry_map_.size()));
  }
#endif
}

void SecurityConfig::get_properties(DDS::PropertyQosPolicy& out_properties) const
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

bool SecurityConfig::qos_implies_security(const DDS::DomainParticipantQos& qos) const {
  const DDS::PropertySeq& properties = qos.property.value;
  for (unsigned int idx = 0; idx != properties.length(); ++idx) {
    const char* name = properties[idx].name.in();
    if (std::strcmp(DDS::Security::Properties::AuthIdentityCA, name) == 0 ||
        std::strcmp(DDS::Security::Properties::AuthIdentityCertificate, name) == 0 ||
        std::strcmp(DDS::Security::Properties::AuthPrivateKey, name) == 0 ||
        std::strcmp(DDS::Security::Properties::AccessPermissionsCA, name) == 0 ||
        std::strcmp(DDS::Security::Properties::AccessGovernance, name) == 0 ||
        std::strcmp(DDS::Security::Properties::AccessPermissions, name) == 0) {
      return true;
    }
  }
  return false;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
