/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_BUILTINPLUGINLOADER_H
#define OPENDDS_DCPS_SECURITY_BUILTINPLUGINLOADER_H

#include "OpenDDS_Security_Export.h"

#include <dds/Versioned_Namespace.h>

#include <ace/Global_Macros.h>
#include <ace/Service_Config.h>
#include <ace/Service_Object.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class BuiltInSecurityPluginInst;

class OpenDDS_Security_Export BuiltInPluginLoader : public ACE_Service_Object
{
public:
  virtual int init(int argc, ACE_TCHAR* argv[]);
};

ACE_STATIC_SVC_DECLARE_EXPORT(OpenDDS_Security, BuiltInPluginLoader)
ACE_FACTORY_DECLARE(OpenDDS_Security, BuiltInPluginLoader)

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_SEC_BUILTIN_PLUGIN_LOADER_H */
