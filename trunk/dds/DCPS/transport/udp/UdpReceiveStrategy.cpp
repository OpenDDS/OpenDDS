/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpReceiveStrategy.h"
#include "UdpDataLink.h"

#include "ace/Reactor.h"

namespace OpenDDS {
namespace DCPS {

UdpReceiveStrategy::UdpReceiveStrategy(UdpDataLink* link)
  : link_(link)
{
}

ACE_HANDLE
UdpReceiveStrategy::get_handle() const
{
  ACE_SOCK_Dgram& socket = this->link_->socket();
  return socket.get_handle();
}

int
UdpReceiveStrategy::handle_input(ACE_HANDLE /*fd*/)
{
  return TransportReceiveStrategy::handle_input();  // delegate to parent
}

ssize_t
UdpReceiveStrategy::receive_bytes(iovec iov[],
                                  int n,
                                  ACE_INET_Addr& remote_address)
{
  ACE_SOCK_Dgram& socket = this->link_->socket();
  return socket.recv(iov, n, remote_address);
}

void
UdpReceiveStrategy::deliver_sample(ReceivedDataSample& sample,
                                   const ACE_INET_Addr& /*remote_address*/)
{
  switch(sample.header_.message_id_) {
  case SAMPLE_ACK:
    this->link_->ack_received(sample);
    break;

  default:
    this->link_->data_received(sample);
  }
}

int
UdpReceiveStrategy::start_i()
{
  ACE_Reactor* reactor = this->link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpReceiveStrategy::start_i: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     -1);
  }

  if (reactor->register_handler(this, ACE_Event_Handler::READ_MASK) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpReceiveStrategy::start_i: ")
                      ACE_TEXT("failed to register handler for DataLink!\n")),
                     -1);
  }

  return 0;
}

void
UdpReceiveStrategy::stop_i()
{
  ACE_Reactor* reactor = this->link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("UdpReceiveStrategy::stop_i: ")
               ACE_TEXT("NULL reactor reference!\n")));
    return;
  }

  reactor->remove_handler(this, ACE_Event_Handler::READ_MASK);
}

} // namespace DCPS
} // namespace OpenDDS
