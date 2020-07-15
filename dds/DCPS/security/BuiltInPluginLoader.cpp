/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BuiltInPluginLoader.h"

#include "BuiltInSecurityPluginInst.h"
#include "framework/SecurityConfig.h"
#include "framework/SecurityRegistry.h"

#include <dds/DCPS/RcHandle_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

static const std::string PLUGIN_NAME("BuiltIn");

int
BuiltInPluginLoader::init(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  SecurityPluginInst_rch plugin = TheSecurityRegistry->get_plugin_inst(
    PLUGIN_NAME, false /* don't attempt to load the plugin */);
  if (!plugin) {
    plugin = DCPS::make_rch<BuiltInSecurityPluginInst>();
    TheSecurityRegistry->register_plugin(PLUGIN_NAME, plugin);
  }

  if (TheSecurityRegistry->has_no_configs()) {
    SecurityConfig_rch default_config =
      TheSecurityRegistry->create_config(SecurityRegistry::DEFAULT_CONFIG_NAME,
                                         plugin);
    TheSecurityRegistry->default_config(default_config);
  }

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
