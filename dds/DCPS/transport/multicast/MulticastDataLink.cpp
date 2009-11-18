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
#include "MulticastSendStrategy.h"
#include "MulticastReceiveStrategy.h"

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
    local_peer_(local_peer),
    remote_peer_(remote_peer)
{
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

  // Reliable links handshake before returning control; this ensures
  // the remote (passive) peer is ready to receive data reliably:
  if (active && this->config_->reliable_) {
    // TODO implement
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
