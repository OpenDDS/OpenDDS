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
                                 const RepoId& local_id,
                                 bool active)
  : DataLink(transport,
             0, // priority
             false, // is_loopback,
             active),// is_active
    active_(active),
    config_(0),
    reactor_task_(0),
    local_id_(local_id)
{
}

bool
RtpsUdpDataLink::open()
{
  if (this->socket_.open(this->config_->local_address_) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpDataLink::open: socket open: %m\n")),
                     false);
  }

  if (start(static_rchandle_cast<TransportSendStrategy>(this->send_strategy_),
            static_rchandle_cast<TransportStrategy>(this->recv_strategy_))
      != 0) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpDataLink::open: start failed!\n")),
                     false);
  }

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
