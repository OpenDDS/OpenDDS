/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_SHMEMDATALINK_RCH_H
#define OPENDDS_SHMEMDATALINK_RCH_H

#include "dds/DCPS/RcHandle_T.h"

namespace OpenDDS {
namespace DCPS {

class ShmemDataLink;

typedef RcHandle<ShmemDataLink> ShmemDataLink_rch;

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_SHMEMDATALINK_RCH_H */
