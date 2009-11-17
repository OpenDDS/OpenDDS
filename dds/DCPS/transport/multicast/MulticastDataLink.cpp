/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastDataLink.h"
#include "MulticastSendReliable.h"
#include "MulticastSendUnreliable.h"
#include "MulticastReceiveReliable.h"
#include "MulticastReceiveUnreliable.h"

#include "ace/Log_Msg.h"

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
  if (this->config_ != 0) {
    this->config_->_add_ref();

    // N.B. This transport supports two primary modes of operation:
    // reliable and unreliable. Eventually the selection of this mode
    // will be autonegotiated; unfortunately the ETF currently
    // multiplexes reliable and non-reliable samples over the same
    // DataLink.
    //if (this->config_->reliable_) {
    //  ACE_NEW_NORETURN(this->send_strategy_, MulticastSendReliable(this));
    //  ACE_NEW_NORETURN(this->recv_strategy_, MulticastReceiveReliable(this));
//
  //  } else {  // unreliable
      ACE_NEW_NORETURN(this->send_strategy_, MulticastSendUnreliable(this));
      ACE_NEW_NORETURN(this->recv_strategy_, MulticastReceiveUnreliable(this));
    //}
  }
}

MulticastDataLink::~MulticastDataLink()
{
  delete this->recv_strategy_;
  delete this->send_strategy_;

  if (this->config_ != 0) {
    this->config_->_remove_ref();
  }
}

bool
MulticastDataLink::join(const ACE_INET_Addr& group_address, bool active)
{
  if (this->socket_.join(group_address) != 0) return false;

  if (start(this->send_strategy_, this->recv_strategy_) != 0) {
    this->socket_.close();

    ACE_ERROR_RETURN((LM_ERROR,
		      ACE_TEXT("(%P|%t) ERROR: ")
		      ACE_TEXT("MulticastDataLink::join: ")
		      ACE_TEXT("failed to start send/receive strategies!\n")),
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
