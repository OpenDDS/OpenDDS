/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/security/BuiltInPluginLoader.h"
#include "dds/DCPS/security/BuiltInSecurityPluginInst.h"
#include "dds/DCPS/security/framework/SecurityConfig.h"
#include "dds/DCPS/security/framework/SecurityRegistry.h"

#include "dds/DCPS/RcHandle_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

static const std::string PLUGIN_NAME("BuiltIn");

int
BuiltInPluginLoader::init(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  if (initialized) return 0;  // already initialized

  SecurityPluginInst_rch plugin = DCPS::make_rch<BuiltInSecurityPluginInst>();
  TheSecurityRegistry->register_plugin(PLUGIN_NAME, plugin);

  SecurityConfig_rch default_config =
    TheSecurityRegistry->create_config(SecurityRegistry::DEFAULT_CONFIG_NAME,
                                       plugin);
  TheSecurityRegistry->default_config(default_config);

  initialized = true;

  return 0;
}

ACE_FACTORY_DEFINE(DdsSecurity, BuiltInPluginLoader);
ACE_STATIC_SVC_DEFINE(
  BuiltInPluginLoader,
  ACE_TEXT("OpenDDS_Security"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME(BuiltInPluginLoader),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0)

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
