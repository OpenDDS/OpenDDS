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
                                     MulticastPeer remote_peer)
  : DataLink(transport, 0), // priority
    transport_(transport),
    local_peer_(local_peer),
    remote_peer_(remote_peer)
{
}

MulticastDataLink::~MulticastDataLink()
{
}

void
MulticastDataLink::configure(MulticastConfiguration* config,
                             TransportReactorTask* reactor_task)
{
  this->config_ = config;
  this->reactor_task_ = reactor_task;
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
  
  if (!join_i(group_address, active)) {
    leave();
    ACE_ERROR_RETURN((LM_ERROR,
		      ACE_TEXT("(%P|%t) ERROR: ")
		      ACE_TEXT("MulticastDataLink::join: ")
		      ACE_TEXT("join_i failed!\n"),
                      error),
                     false);
  }

  if ((error = start(this->send_strategy_.in(),
                     this->recv_strategy_.in())) != 0) {
    leave();
    ACE_ERROR_RETURN((LM_ERROR,
		      ACE_TEXT("(%P|%t) ERROR: ")
		      ACE_TEXT("MulticastDataLink::join: ")
		      ACE_TEXT("start failed: %d\n"),
                      error),
                     false);
  }

  return true;
}

bool
MulticastDataLink::join_i(const ACE_INET_Addr& /*group_address*/, bool /*active*/)
{
  return true;  // default
}

void
MulticastDataLink::leave()
{
  leave_i();
  this->socket_.close();
}

void
MulticastDataLink::leave_i()
{
  // default
}

void
MulticastDataLink::stop_i()
{
  leave();
}

} // namespace DCPS
} // namespace OpenDDS
