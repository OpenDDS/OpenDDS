/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSendReliable.h"

namespace OpenDDS {
namespace DCPS {

MulticastSendReliable::MulticastSendReliable(MulticastDataLink* link)
  : MulticastSendStrategy(link)
{
}

void
MulticastSendReliable::stop_i()
{
  // TODO implement
}

ssize_t
MulticastSendReliable::send_bytes_i(const iovec iov[], int n)
{
  return 0; // TODO implement
}

} // namespace DCPS
} // namespace OpenDDS
