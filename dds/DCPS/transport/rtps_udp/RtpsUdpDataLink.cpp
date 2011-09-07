/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpDataLink.h"
#include "RtpsUdpTransport.h"
#include "RtpsUdpInst.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"

#include "ace/Default_Constants.h"
#include "ace/Log_Msg.h"

#ifndef __ACE_INLINE__
# include "RtpsUdpDataLink.inl"
#endif  /* __ACE_INLINE__ */


namespace OpenDDS {
namespace DCPS {

RtpsUdpDataLink::RtpsUdpDataLink(RtpsUdpTransport* transport,
                                 bool active)
  : DataLink(transport,
             0, // priority
             false, // is_loopback,
             active),// is_active
    active_(active),
    config_(0),
    reactor_task_(0)
{
}

bool
RtpsUdpDataLink::open(const ACE_INET_Addr& remote_address)
{
  return true;
}

void
RtpsUdpDataLink::control_received(ReceivedDataSample& sample,
                                  const ACE_INET_Addr& remote_address)
{
}

void
RtpsUdpDataLink::stop_i()
{
}

} // namespace DCPS
} // namespace OpenDDS
