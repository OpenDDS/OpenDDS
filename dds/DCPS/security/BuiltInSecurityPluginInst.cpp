/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BuiltInSecurityPluginInst.h"

#include "AccessControlBuiltInImpl.h"
#include "AuthenticationBuiltInImpl.h"
#include "CryptoBuiltInImpl.h"
#include "UtilityImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

using DDS::Security::CryptoKeyExchange;
using DDS::Security::CryptoTransform;

BuiltInSecurityPluginInst::BuiltInSecurityPluginInst()
#ifdef OPENDDS_SECURITY
  : authentication_(new AuthenticationBuiltInImpl)
  , access_control_(new AccessControlBuiltInImpl)
  , key_factory_(new CryptoBuiltInImpl)
  , key_exchange_(CryptoKeyExchange::_narrow(key_factory_))
  , transform_(CryptoTransform::_narrow(key_factory_))
  , utility_(DCPS::make_rch<UtilityImpl>())
#endif
{
}

BuiltInSecurityPluginInst::~BuiltInSecurityPluginInst()
{
}

#ifdef OPENDDS_SECURITY
Authentication_var BuiltInSecurityPluginInst::create_authentication()
{
  return authentication_;
}

AccessControl_var BuiltInSecurityPluginInst::create_access_control()
{
  return access_control_;
}

CryptoKeyFactory_var BuiltInSecurityPluginInst::create_crypto_key_factory()
{
  return key_factory_;
}

CryptoKeyExchange_var BuiltInSecurityPluginInst::create_crypto_key_exchange()
{
  return key_exchange_;
}

CryptoTransform_var BuiltInSecurityPluginInst::create_crypto_transform()
{
  return transform_;
}

DCPS::RcHandle<Utility> BuiltInSecurityPluginInst::create_utility()
{
  return utility_;
}

#endif

void BuiltInSecurityPluginInst::shutdown()
{
  // No actions
}

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
