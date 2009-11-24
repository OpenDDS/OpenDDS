/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

namespace OpenDDS {
namespace DCPS {

ACE_INLINE void
MulticastDataLink::configure(MulticastConfiguration* config,
                             TransportReactorTask* reactor_task)
{
  this->config_ = config;
  this->reactor_task_ = reactor_task;
}

ACE_INLINE void
MulticastDataLink::send_strategy(MulticastSendStrategy* send_strategy)
{
  this->send_strategy_ = send_strategy;
}

ACE_INLINE void
MulticastDataLink::receive_strategy(MulticastReceiveStrategy* recv_strategy)
{
  this->recv_strategy_ = recv_strategy;
}

ACE_INLINE MulticastPeer
MulticastDataLink::local_peer() const
{
  return this->local_peer_;
}

ACE_INLINE MulticastPeer
MulticastDataLink::remote_peer() const
{
  return this->remote_peer_;
}

ACE_INLINE MulticastConfiguration*
MulticastDataLink::config()
{
  return this->config_.in();
}

ACE_INLINE TransportReactorTask*
MulticastDataLink::reactor_task()
{
  return this->reactor_task_.in();
}

ACE_INLINE ACE_Reactor*
MulticastDataLink::get_reactor()
{
  if (this->reactor_task_.is_nil()) return 0;
  return this->reactor_task_->get_reactor();
}

ACE_INLINE ACE_SOCK_Dgram_Mcast&
MulticastDataLink::socket()
{
  return this->socket_;
}

} // namespace DCPS
} // namespace OpenDDS
