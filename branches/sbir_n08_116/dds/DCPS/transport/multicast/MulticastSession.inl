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

ACE_INLINE MulticastDataLink*
MulticastSession::link()
{
  return this->link_;
}

ACE_INLINE MulticastPeer
MulticastSession::remote_peer() const
{
  return this->remote_peer_;
}

} // namespace DCPS
} // namespace OpenDDS
