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

#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"

#ifndef __ACE_INLINE__
# include "MulticastDataLink.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

MulticastDataLink::MulticastDataLink(MulticastTransport* transport,
                                     MulticastPeer local_peer,
                                     MulticastPeer remote_peer,
                                     bool active)
  : DataLink(transport,
             0), // priority
    transport_(transport),
    local_peer_(local_peer),
    remote_peer_(remote_peer),
    active_(active)
{
}

MulticastDataLink::~MulticastDataLink()
{
}

void
MulticastDataLink::send_strategy(MulticastSendStrategy* send_strategy)
{
  this->send_strategy_ = send_strategy;
}

void
MulticastDataLink::receive_strategy(MulticastReceiveStrategy* recv_strategy)
{
  this->recv_strategy_ = recv_strategy;
}

bool
MulticastDataLink::join(const ACE_INET_Addr& group_address)
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

  return true;
}

void
MulticastDataLink::stop_i()
{
  this->socket_.close();
}

} // namespace DCPS
} // namespace OpenDDS
