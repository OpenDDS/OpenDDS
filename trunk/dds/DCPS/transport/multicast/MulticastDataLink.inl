/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
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

ACE_INLINE bool
MulticastDataLink::active() const
{
  return this->active_;
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
