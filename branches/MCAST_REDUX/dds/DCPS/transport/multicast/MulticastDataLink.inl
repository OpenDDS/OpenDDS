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

ACE_INLINE MulticastConfiguration*
MulticastDataLink::config()
{
  return this->config_.in();
}

ACE_INLINE long
MulticastDataLink::local_peer() const
{
  return this->local_peer_;
}

ACE_INLINE long
MulticastDataLink::remote_peer() const
{
  return this->remote_peer_;
}

ACE_INLINE ACE_SOCK_Dgram_Mcast&
MulticastDataLink::socket()
{
  return this->socket_;
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

} // namespace DCPS
} // namespace OpenDDS
