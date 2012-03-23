/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSendStrategy.h"
#include "MulticastDataLink.h"
#include "dds/DCPS/transport/framework/NullSynchStrategy.h"
#include "ace/Proactor.h"

namespace OpenDDS {
namespace DCPS {
 
MulticastSendStrategy::MulticastSendStrategy(MulticastDataLink* link)
  : TransportSendStrategy(TransportInst_rch(link->config(), false),
                          0,  // synch_resource
                          link->transport_priority(),
                          new NullSynchStrategy),
    link_(link)
{
  // Multicast will send a SYN (TRANSPORT_CONTROL) before any reservations
  // are made on the DataLink, if the link is "release" it will be dropped.
  this->link_released(false);
}

void
MulticastSendStrategy::prepare_header_i()
{
  // Tag outgoing packets with our peer ID:
  this->header_.source_ = this->link_->local_peer();
}

ssize_t
MulticastSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  return (this->link_->config()->async_send() ? async_send(iov, n) : sync_send(iov, n));
}

ssize_t
MulticastSendStrategy::sync_send(const iovec iov[], int n)
{
  ACE_SOCK_Dgram_Mcast& socket = this->link_->socket();

  return socket.send(iov, n);
}

ssize_t
MulticastSendStrategy::async_send(const iovec iov[], int n)
{
  ACE_SOCK_Dgram_Mcast& socket = this->link_->socket();
  ACE_Asynch_Write_Dgram wd;

  if (-1 == wd.open(*this, socket.get_handle(), 0 /*completion_key*/, this->link_->get_proactor())) {
    return -1;
  }

  ACE_Message_Block* mb = 0;

  for (int i = n - 1; i >=0 ; --i) {
    ACE_Message_Block* next = new ACE_Message_Block(iov[i].iov_base, iov[i].iov_len);
    next->wr_ptr(iov[i].iov_len);
    next->cont(mb);
    mb = next;
  }

  size_t bytes_sent = 0;
  ssize_t result = wd.send(mb, bytes_sent, 0 /*flags*/, this->link_->config()->group_address_);

  if (result < 0) {
    mb->release();
    return result;
  }
 
  return bytes_sent;
}

void
MulticastSendStrategy::stop_i()
{
}

void MulticastSendStrategy::handle_write_dgram(const ACE_Asynch_Write_Dgram::Result& res)
{
  if (!res.success()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MulticastSendStrategy::handle_write_dgram: %d\n", res.error()));
  }
  res.message_block()->release();
}

} // namespace DCPS
} // namespace OpenDDS
