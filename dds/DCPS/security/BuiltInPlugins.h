/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_SECURITY_BUILTIN_PLUGINS_INITIALIZER_H
#define DCPS_SECURITY_BUILTIN_PLUGINS_INITIALIZER_H

#include "DdsSecurity_Export.h"
#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class DdsSecurity_Export BuiltinPluginsInitializer {
public:
  BuiltinPluginsInitializer();
};

static BuiltinPluginsInitializer builtin_plugins_init;

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
