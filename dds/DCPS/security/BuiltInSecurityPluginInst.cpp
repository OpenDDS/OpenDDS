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

OpenDDS::DCPS::Authentication_rch BuiltInSecurityPluginInst::create_authentication()
{
	return OpenDDS::DCPS::rchandle_from(new AuthenticationBuiltInImpl());
}

OpenDDS::DCPS::AccessControl_rch BuiltInSecurityPluginInst::create_access_control()
{
	return OpenDDS::DCPS::rchandle_from(new AccessControlBuiltInImpl());
}
OpenDDS::DCPS::CryptoKeyExchange_rch BuiltInSecurityPluginInst::create_crypto_key_exchange()
{
	return OpenDDS::DCPS::rchandle_from(new CryptoKeyExchangeBuiltInImpl());
}
OpenDDS::DCPS::CryptoKeyFactory_rch BuiltInSecurityPluginInst::create_crypto_key_factory()
{
	return OpenDDS::DCPS::rchandle_from(new CryptoKeyFactoryBuiltInImpl());
}
OpenDDS::DCPS::CryptoTransform_rch BuiltInSecurityPluginInst::create_crypto_transform()
{
	return OpenDDS::DCPS::rchandle_from(new CryptoTransformBuiltInImpl());
}

void BuiltInSecurityPluginInst::shutdown()
{
	// No actions
}

} // namespace Security
} // namespace OpenDDS
