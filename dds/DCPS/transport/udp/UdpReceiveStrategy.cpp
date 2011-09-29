/*
 * $Id$
 *
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
  , last_received_()
{
}

ACE_HANDLE
UdpReceiveStrategy::get_handle() const
{
  ACE_SOCK_Dgram& socket = this->link_->socket();
  return socket.get_handle();
}

int
UdpReceiveStrategy::handle_input(ACE_HANDLE fd)
{
  return this->handle_dds_input(fd);
}

ssize_t
UdpReceiveStrategy::receive_bytes(iovec iov[],
                                  int n,
                                  ACE_INET_Addr& remote_address,
                                  ACE_HANDLE /*fd*/)
{
  ACE_SOCK_Dgram& socket = this->link_->socket();
  return socket.recv(iov, n, remote_address);
}

void
UdpReceiveStrategy::deliver_sample(ReceivedDataSample& sample,
                                   const ACE_INET_Addr& remote_address)
{
  switch (sample.header_.message_id_) {
  case SAMPLE_ACK:
    this->link_->ack_received(sample);
    break;

  case TRANSPORT_CONTROL:
    this->link_->control_received(sample, remote_address);
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

  this->enable_reassembly();
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

bool
UdpReceiveStrategy::check_header(const TransportHeader& header)
{
  SequenceNumber expected(this->last_received_);
  ++expected;
  if (header.sequence_ != expected) {
    VDBG_LVL((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: UdpReceiveStrategy::check_header ")
               ACE_TEXT("expected %q received %q\n"), expected.getValue(),
               header.sequence_.getValue()), 2);
    SequenceRange range(expected, header.sequence_.previous());
    this->data_unavailable(range);
  }

  this->last_received_ = header.sequence_;
  return true;
}

} // namespace DCPS
} // namespace OpenDDS
