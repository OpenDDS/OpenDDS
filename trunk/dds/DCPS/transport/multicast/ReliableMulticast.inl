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
ReliableMulticast::reliability_lost()
{
  // Notify transport that reliability has been compromised:
  this->transport_->reliability_lost(this);
}

} // namespace DCPS
} // namespace OpenDDS
