/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_PLUGIN_API_RCH_H
#define OPENDDS_DCPS_SECURITY_PLUGIN_API_RCH_H

#include "dds/DCPS/RcHandle_T.h"
#include "dds/DdsSecurityCoreC.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {


typedef RcHandle<DDS::Security::Authentication> Authentication_rch;
typedef RcHandle<DDS::Security::AccessControl> AccessControl_rch;
typedef RcHandle<DDS::Security::CryptoKeyExchange> CryptoKeyExchange_rch;
typedef RcHandle<DDS::Security::CryptoKeyFactory> CryptoKeyFactory_rch;
typedef RcHandle<DDS::Security::CryptoTransform> CryptoTransform_rch;


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
