/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TIMERS_H
#define OPENDDS_DCPS_TIMERS_H

#include "Definitions.h"
#include "RcEventHandler.h"
#include "TimeTypes.h"

#include "dcps_export.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Reactor;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {
namespace Timers {

typedef long TimerId; // from ACE_Reactor
extern OpenDDS_Dcps_Export const TimerId InvalidTimerId;

OpenDDS_Dcps_Export TimerId schedule(ACE_Reactor* reactor,
                                     RcEventHandler& handler,
                                     const void* arg,
                                     const TimeDuration& delay,
                                     const TimeDuration& interval = TimeDuration());

OpenDDS_Dcps_Export void cancel(ACE_Reactor* reactor, TimerId timer);

}
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
