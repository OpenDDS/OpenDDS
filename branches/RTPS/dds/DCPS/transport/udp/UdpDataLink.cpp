/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpDataLink.h"
#include "UdpTransport.h"

#include "ace/Default_Constants.h"
#include "ace/Log_Msg.h"

#ifndef __ACE_INLINE__
# include "UdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

UdpDataLink::UdpDataLink(UdpTransport* transport,
                         bool active)
  : DataLink(transport,
             0, // priority
             false, // is_loopback,
             active),// is_active
    active_(active),
    config_(0),
    reactor_task_(0)
{
}

bool
UdpDataLink::open(const ACE_INET_Addr& remote_address)
{
  this->remote_address_ = remote_address;
  this->is_loopback_ = this->remote_address_ == this->config_->local_address_;

  ACE_INET_Addr local_address;
  if (!this->active_) {
    local_address = this->config_->local_address_;
  }

  if (this->socket_.open(local_address) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpDataLink::open: open failed: %m\n")),
                     false);
  }

#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
  int snd_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
  int rcv_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;

  if (this->socket_.set_option(SOL_SOCKET,
                              SO_SNDBUF,
                              (void *) &snd_size,
                              sizeof(snd_size)) < 0
      && errno != ENOTSUP) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpDataLink::open: failed to set the send buffer size to %d errno %m\n"),
                      snd_size),
                     false);
  }

  if (this->socket_.set_option(SOL_SOCKET,
                              SO_RCVBUF,
                              (void *) &rcv_size,
                              sizeof(int)) < 0
      && errno != ENOTSUP) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpDataLink::open: failed to set the receive buffer size to %d errno %m \n"),
                      rcv_size),
                     false);
  }
#endif /* ACE_DEFAULT_MAX_SOCKET_BUFSIZ */

  if (start(this->send_strategy_.in(), this->recv_strategy_.in()) != 0) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpDataLink::open: start failed!\n")),
                     false);
  }

  return true;
}

void
UdpDataLink::stop_i()
{
  this->socket_.close();
}

} // namespace DCPS
} // namespace OpenDDS
