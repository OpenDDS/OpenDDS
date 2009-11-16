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

ACE_INLINE TransportConfiguration*
MulticastDataLink::get_configuration()
{
  return this->config_;
}

ACE_INLINE long
MulticastDataLink::get_local_peer() const
{
  return this->local_peer_;
}

ACE_INLINE long
MulticastDataLink::get_remote_peer() const
{
  return this->remote_peer_;
}

ACE_INLINE ACE_SOCK_Dgram_Mcast&
MulticastDataLink::get_socket()
{
  return this->socket_;
}

} // namespace DCPS
} // namespace OpenDDS
