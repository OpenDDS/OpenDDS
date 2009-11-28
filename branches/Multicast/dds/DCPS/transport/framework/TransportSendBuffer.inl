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
TransportSendBuffer::bind(TransportSendStrategy* strategy) {
  this->strategy_ = strategy;
}

} // namespace DCPS
} // namespace OpenDDS
