/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BuiltInPlugins.h"

#include "BuiltInPluginLoader.h"

#include <ace/Service_Config.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

BuiltinPluginsInitializer::BuiltinPluginsInitializer()
{
  ACE_Service_Config::process_directive(ace_svc_desc_BuiltInPluginLoader);

#if (OPENDDS_SECURITY_HAS_DLL == 0)
  ACE_Service_Config::process_directive(ACE_TEXT("static OpenDDS_Security"));
#endif

  OpenDDS::Security::BuiltInPluginLoader().init(0, 0);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
