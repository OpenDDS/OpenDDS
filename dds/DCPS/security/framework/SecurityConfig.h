/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURTIY_CONFIG_H
#define OPENDDS_DCPS_SECURTIY_CONFIG_H

#include <ace/config.h>
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif

#include "dds/DCPS/dcps_export.h"

#ifdef OPENDDS_SECURITY
#include "dds/DdsSecurityCoreC.h"
#include "dds/DCPS/security/Utility.h"
#endif

#include "dds/DdsDcpsCoreC.h"
#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/security/framework/SecurityConfigPropertyList.h"

#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

#ifdef OPENDDS_SECURITY
using DDS::Security::Authentication_var;
using DDS::Security::AccessControl_var;
using DDS::Security::CryptoKeyExchange_var;
using DDS::Security::CryptoKeyFactory_var;
using DDS::Security::CryptoTransform_var;
#endif

class OpenDDS_Dcps_Export SecurityConfig : public DCPS::RcObject {
 public:

  OPENDDS_STRING name() const
  {
    return name_;
  }

#ifdef OPENDDS_SECURITY
  // Get the plugins associated with this configuration
  Authentication_var get_authentication() const
  {
    return authentication_plugin_;
  }

  AccessControl_var get_access_control() const
  {
    return access_control_plugin_;
  }

  CryptoKeyExchange_var get_crypto_key_exchange() const
  {
    return key_exchange_plugin_;
  }

  CryptoKeyFactory_var get_crypto_key_factory() const
  {
    return key_factory_plugin_;
  }

  CryptoTransform_var get_crypto_transform() const
  {
    return transform_plugin_;
  }

  Utility* get_utility() const
  {
    return utility_plugin_;
  }
#endif

  void get_properties(DDS::PropertyQosPolicy& properties) const;

  bool qos_implies_security(const DDS::DomainParticipantQos& qos) const;

  SecurityConfig(const OPENDDS_STRING& name,
#ifdef OPENDDS_SECURITY
                 Authentication_var authentication_plugin,
                 AccessControl_var access_ctrl_plugin,
                 CryptoKeyExchange_var key_exchange_plugin,
                 CryptoKeyFactory_var key_factory_plugin,
                 CryptoTransform_var transform_plugin,
                 Utility* utility_plugin,
#endif
                 const ConfigPropertyList& properties);

 private:
  friend class SecurityRegistry;

  template <typename T, typename U>
  friend DCPS::RcHandle<T> DCPS::make_rch(U const&);

  ~SecurityConfig();

  const OPENDDS_STRING name_;

#ifdef OPENDDS_SECURITY
  Authentication_var authentication_plugin_;
  AccessControl_var access_control_plugin_;
  CryptoKeyExchange_var key_exchange_plugin_;
  CryptoKeyFactory_var key_factory_plugin_;
  CryptoTransform_var transform_plugin_;
  Utility* utility_plugin_;
#endif

  ConfigPropertyList properties_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_SECURTIY_CONFIG_H */
