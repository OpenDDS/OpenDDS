/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTTYPE_RCH_H
#define OPENDDS_DCPS_TRANSPORTTYPE_RCH_H

#include <ace/config.h>
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif

#include "dds/DCPS/RcHandle_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TransportType;
typedef RcHandle<TransportType> TransportType_rch;

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORTTYPE_RCH_H */
