/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTCONFIG_RCH_H
#define OPENDDS_DCPS_TRANSPORTCONFIG_RCH_H

#include <ace/config.h>
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif

#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/transport/framework/TransportConfig.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TransportConfig;
typedef RcHandle<TransportConfig> TransportConfig_rch;

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORTCONFIG_RCH_H */
