/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSendStrategy.h"
#include "MulticastDataLink.h"
#include "dds/DCPS/transport/framework/NullSynchStrategy.h"
#include "ace/Proactor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

MulticastSendStrategy::MulticastSendStrategy(MulticastDataLink* link, const TransportInst_rch& inst)
  : TransportSendStrategy(0, inst,
                          0,  // synch_resource
                          link->transport_priority(),
                          make_rch<NullSynchStrategy>()),
    link_(link)
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
  , async_init_(false)
#endif
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

  const ssize_t result = socket.send(iov, n);

  if (result == -1 && errno == ENOBUFS) {
    // Make the framework think this was a successful send to avoid
    // putting the send strategy in suspended mode.  If reliability
    // is enabled, the data may be resent later in response to a NAK.
    ssize_t b = 0;
    for (int i = 0; i < n; ++i) b += iov[i].iov_len;
    return b;
  }

  return result;
}

ssize_t
MulticastSendStrategy::async_send(const iovec iov[], int n)
{
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
  if (!async_init_) {
    if (-1 == async_writer_.open(*this, link_->socket().get_handle(), 0 /*completion_key*/,
                                 link_->get_proactor())) {
        return -1;
    }
    async_init_ = true;
  }

  ACE_Message_Block* mb = 0;
  size_t total_length = 0;

  for (int i = n - 1; i >= 0; --i) {
    ACE_Message_Block* next =
      new ACE_Message_Block(static_cast<const char*>(iov[i].iov_base),
                            iov[i].iov_len);
    next->wr_ptr(iov[i].iov_len);
    total_length += iov[i].iov_len;
    next->cont(mb);
    mb = next;
  }

  size_t bytes_sent = 0;
  ssize_t result = async_writer_.send(mb, bytes_sent, 0 /*flags*/,
                                      this->link_->config()->group_address_);

  if (result < 0) {
    if (mb) mb->release();
    return result;
  }

  // framework needs to think we sent the entire datagram
  return total_length;
#else
  ACE_UNUSED_ARG(iov);
  ACE_UNUSED_ARG(n);
  return -1;
#endif
}

void
MulticastSendStrategy::stop_i()
{
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
  if (async_init_) {
    async_writer_.cancel();
  }
#endif
}

#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
void
MulticastSendStrategy::handle_write_dgram(const ACE_Asynch_Write_Dgram::Result& res)
{
  if (!res.success()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MulticastSendStrategy::handle_write_dgram: %d\n", res.error()));
  }
  res.message_block()->release();
}
#endif


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
