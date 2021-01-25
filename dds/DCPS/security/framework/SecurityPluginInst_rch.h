/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_FRAMEWORK_SECURITYPLUGININST_RCH_H
#define OPENDDS_DCPS_SECURITY_FRAMEWORK_SECURITYPLUGININST_RCH_H

#include "dds/DCPS/RcHandle_T.h"
#include "SecurityPluginInst.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

typedef DCPS::RcHandle<SecurityPluginInst> SecurityPluginInst_rch;

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
