/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BestEffortSessionFactory.h"
#include "BestEffortSession.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

int
BestEffortSessionFactory::requires_send_buffer() const
{
  return 0;
}

MulticastSession_rch
BestEffortSessionFactory::create(RcHandle<EventDispatcher> event_dispatcher,
                                 MulticastDataLink* link,
                                 MulticastPeer remote_peer)
{
  return make_rch<BestEffortSession>(event_dispatcher, link, remote_peer);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
