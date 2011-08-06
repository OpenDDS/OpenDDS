/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTCONFIG_RCH_H
#define OPENDDS_DCPS_TRANSPORTCONFIG_RCH_H

#include <ace/config.h>
#ifndef ACE_LACKS_PRAGMA_ONCE
#pragma once
#endif

#include "dds/DCPS/RcHandle_T.h"

namespace OpenDDS {
namespace DCPS {

class TransportConfig;
typedef RcHandle<TransportConfig> TransportConfig_rch;

}
}

#endif /* OPENDDS_DCPS_TRANSPORTCONFIG_RCH_H */
