/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTSESSION_RCH_H
#define DCPS_MULTICASTSESSION_RCH_H

#include "dds/DCPS/RcHandle_T.h"

namespace OpenDDS {
namespace DCPS {

class MulticastSession;

typedef RcHandle<MulticastSession> MulticastSession_rch;

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTSESSION_RCH_H */
