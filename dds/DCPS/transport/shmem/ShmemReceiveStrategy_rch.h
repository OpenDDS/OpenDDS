/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_SHMEMRECEIVESTRATEGY_RCH_H
#define OPENDDS_SHMEMRECEIVESTRATEGY_RCH_H

#include "dds/DCPS/RcHandle_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ShmemReceiveStrategy;

typedef RcHandle<ShmemReceiveStrategy> ShmemReceiveStrategy_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_SHMEMRECEIVESTRATEGY_RCH_H */
