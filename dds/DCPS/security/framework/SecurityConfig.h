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
#include "dds/DdsSecurityCoreC.h"
#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/security/framework/SecurityPluginInterfaces_rch.h"
#include "dds/DCPS/security/framework/SecurityConfigPropertyList.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export SecurityConfig : public RcObject<ACE_SYNCH_MUTEX> {
public:

  OPENDDS_STRING name() const
  {
  	return name_;
  }

  // Get the plugins associated with this configuration
  Authentication_rch get_authentication() const
  {
  	return authentication_plugin_;
  }
  AccessControl_rch get_access_control() const
  {
  	return access_control_plugin_;
  }
  CryptoKeyExchange_rch get_crypto_key_exchange() const
  {
  	return key_exchange_plugin_;
  }
  CryptoKeyFactory_rch get_crypto_key_factory() const
  {
  	return key_factory_plugin_;
  }
  CryptoTransform_rch get_crypto_transform() const
  {
  	return transform_plugin_;
  }

  void get_properties(DDS::Security::PropertyQosPolicy& properties) const;

	SecurityConfig(const OPENDDS_STRING& name,
								Authentication_rch authentication_plugin,
								AccessControl_rch access_ctrl_plugin,
								CryptoKeyExchange_rch key_exchange_plugin,
								CryptoKeyFactory_rch key_factory_plugin,
								CryptoTransform_rch transform_plugin,
								const ConfigPropertyList& properties);

private:
  friend class SecurityRegistry;

  template <typename T, typename U>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U const&);

  ~SecurityConfig();

  const OPENDDS_STRING name_;
  Authentication_rch authentication_plugin_;
  AccessControl_rch access_control_plugin_;
  CryptoKeyExchange_rch key_exchange_plugin_;
  CryptoKeyFactory_rch key_factory_plugin_;
  CryptoTransform_rch transform_plugin_;
  ConfigPropertyList properties_;

};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_SECURTIY_CONFIG_H */
