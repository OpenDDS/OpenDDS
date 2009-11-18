/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastReceiveUnreliable.h"

namespace OpenDDS {
namespace DCPS {

MulticastReceiveUnreliable::MulticastReceiveUnreliable(MulticastDataLink* link)
  : MulticastReceiveStrategy(link)
{
}

ssize_t
MulticastReceiveUnreliable::receive_bytes(iovec iov[],
                                          int n,
                                          ACE_INET_Addr& remote_address)
{
  ACE_SOCK_Dgram_Mcast& socket = this->link_->socket();
  return socket.recv(iov, n, remote_address);
}

void
MulticastReceiveUnreliable::deliver_sample(ReceivedDataSample& sample,
                                           const ACE_INET_Addr& /*remote_address*/)
{
  this->link_->data_received(sample);
}

int
MulticastReceiveUnreliable::start_i()
{
  ACE_Reactor* reactor = this->link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastReceiveUnreliable::start_i: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     -1);
  }

  if (reactor->register_handler(this, ACE_Event_Handler::READ_MASK) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastReceiveUnreliable::start_i: ")
                      ACE_TEXT("failed to register handler for socket!\n")),
                     -1);
  }

  return 0;
}

void
MulticastReceiveUnreliable::stop_i()
{
  ACE_Reactor* reactor = this->link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("MulticastReceiveUnreliable::stop_i: ")
               ACE_TEXT("NULL reactor reference!\n")));
    return;
  }

  reactor->remove_handler(this, ACE_Event_Handler::READ_MASK);
}

} // namespace DCPS
} // namespace OpenDDS
