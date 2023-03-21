/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_BUILTINSECURITYPLUGININST_H
#define OPENDDS_DCPS_SECURITY_BUILTINSECURITYPLUGININST_H

#include "OpenDDS_Security_Export.h"
#include "framework/SecurityPluginInst.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

/**
 * @class BuiltInSecurityPluginInst
 *
 * @brief Factory object to create interfaces for the BuiltIn plugin.
 */
class OpenDDS_Security_Export BuiltInSecurityPluginInst : public SecurityPluginInst {
public:

  BuiltInSecurityPluginInst();
  ~BuiltInSecurityPluginInst();

#ifdef OPENDDS_SECURITY
  virtual Authentication_var create_authentication();
  virtual AccessControl_var create_access_control();
  virtual CryptoKeyFactory_var create_crypto_key_factory();
  virtual CryptoKeyExchange_var create_crypto_key_exchange();
  virtual CryptoTransform_var create_crypto_transform();
  virtual DCPS::RcHandle<Utility> create_utility();
#endif

  virtual void shutdown();

private:
#ifdef OPENDDS_SECURITY
  Authentication_var authentication_;
  AccessControl_var access_control_;
  CryptoKeyFactory_var key_factory_;
  CryptoKeyExchange_var key_exchange_;
  CryptoTransform_var transform_;
  DCPS::RcHandle<Utility> utility_;
#endif

  BuiltInSecurityPluginInst(const BuiltInSecurityPluginInst&);
  BuiltInSecurityPluginInst& operator=(const BuiltInSecurityPluginInst&);
};


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
