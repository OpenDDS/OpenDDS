/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

namespace OpenDDS {
namespace DCPS {

ACE_INLINE MulticastTransport*
MulticastDataLink::transport()
{
  return this->transport_;
}

ACE_INLINE MulticastPeer
MulticastDataLink::local_peer() const
{
  return this->local_peer_;
}

ACE_INLINE MulticastSendStrategy*
MulticastDataLink::send_strategy()
{
  return this->send_strategy_.in();
}

ACE_INLINE MulticastReceiveStrategy*
MulticastDataLink::receive_strategy()
{
  return this->recv_strategy_.in();
}

ACE_INLINE TransportSendBuffer*
MulticastDataLink::send_buffer()
{
  return this->send_buffer_.in();
}

ACE_INLINE MulticastConfiguration*
MulticastDataLink::config()
{
  return this->config_;
}

ACE_INLINE TransportReactorTask*
MulticastDataLink::reactor_task()
{
  return this->reactor_task_;
}

ACE_INLINE ACE_Reactor*
MulticastDataLink::get_reactor()
{
  if (this->reactor_task_ == 0) return 0;
  return this->reactor_task_->get_reactor();
}

ACE_INLINE ACE_SOCK_Dgram_Mcast&
MulticastDataLink::socket()
{
  return this->socket_;
}

} // namespace DCPS
} // namespace OpenDDS
