/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastDataLink.h"
#include "MulticastTransport.h"
#include "MulticastSendReliable.h"
#include "MulticastSendUnreliable.h"
#include "MulticastReceiveReliable.h"
#include "MulticastReceiveUnreliable.h"

#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"

#ifndef __ACE_INLINE__
# include "MulticastDataLink.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

MulticastDataLink::MulticastDataLink(MulticastTransport* impl,
				     CORBA::Long priority,
                                     long local_peer,
                                     long remote_peer)
  : DataLink(impl, priority),
    config_(impl->get_configuration()),
    local_peer_(local_peer),
    remote_peer_(remote_peer)
{
  if (!this->config_.is_nil()) {
    // N.B. This transport supports two primary modes of operation:
    // reliable and unreliable. Eventually the selection of this mode
    // will be autonegotiated; unfortunately the ETF currently
    // multiplexes reliable and non-reliable samples over the same
    // DataLink.
    if (this->config_->reliable_) {
      this->send_strategy_ = new MulticastSendReliable(this);
      this->recv_strategy_ = new MulticastReceiveReliable(this);
    
    } else {  // unreliable
      this->send_strategy_ = new MulticastSendUnreliable(this);
      this->recv_strategy_ = new MulticastReceiveUnreliable(this);
    }
  }
}

bool
MulticastDataLink::join(const ACE_INET_Addr& group_address, bool active)
{
  int error;

  if ((error = this->socket_.join(group_address)) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastDataLink::join: ")
                      ACE_TEXT("join failed: %C\n"),
                      ACE_OS::strerror(error)),
                     false);
  }

  if ((error = start(this->send_strategy_.in(),
                     this->recv_strategy_.in())) != 0) {
    stop_i();

    ACE_ERROR_RETURN((LM_ERROR,
		      ACE_TEXT("(%P|%t) ERROR: ")
		      ACE_TEXT("MulticastDataLink::join: ")
		      ACE_TEXT("start failed: %d\n"),
                      error),
                     false);
  }

  // Reliable links should handshake before returning control
  // back to the ETF; this ensures both peers are initialized
  // prior to sending data.
  if (active && this->config_->reliable_) {
    // TODO handshake
  }

  return true;
}

void
MulticastDataLink::stop_i()
{
  this->socket_.close();
}

} // namespace DCPS
} // namespace OpenDDS
