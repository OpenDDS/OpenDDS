/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/security/BuiltInSecurityPluginInst.h"
#include "dds/DCPS/security/AccessControlBuiltInImpl.h"
#include "dds/DCPS/security/AuthenticationBuiltInImpl.h"
#include "dds/DCPS/security/CryptoKeyExchangeBuiltInImpl.h"
#include "dds/DCPS/security/CryptoKeyFactoryBuiltInImpl.h"
#include "dds/DCPS/security/CryptoTransformBuiltInImpl.h"
#include "dds/DCPS/RcHandle_T.h"

namespace OpenDDS {
namespace Security {

BuiltInSecurityPluginInst::BuiltInSecurityPluginInst()
{
}

BuiltInSecurityPluginInst::~BuiltInSecurityPluginInst()
{
}

Authentication_var BuiltInSecurityPluginInst::create_authentication()
{
  return new AuthenticationBuiltInImpl;
}

AccessControl_var BuiltInSecurityPluginInst::create_access_control()
{
  return new AccessControlBuiltInImpl;
}

CryptoKeyExchange_var BuiltInSecurityPluginInst::create_crypto_key_exchange()
{
  return new CryptoKeyExchangeBuiltInImpl;
}

CryptoKeyFactory_var BuiltInSecurityPluginInst::create_crypto_key_factory()
{
  return new CryptoKeyFactoryBuiltInImpl;
}

CryptoTransform_var BuiltInSecurityPluginInst::create_crypto_transform()
{
  return new CryptoTransformBuiltInImpl;
}

void BuiltInSecurityPluginInst::shutdown()
{
  // No actions
}

} // namespace Security
} // namespace OpenDDS
