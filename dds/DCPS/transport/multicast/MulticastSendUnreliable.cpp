/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSendUnreliable.h"

namespace OpenDDS {
namespace DCPS {

MulticastSendUnreliable::MulticastSendUnreliable(MulticastDataLink* link)
  : MulticastSendStrategy(link)
{
}

void
MulticastSendUnreliable::stop_i()
{
  // TODO implement
}

ACE_HANDLE
MulticastSendUnreliable::get_handle()
{
  return 0; // TODO implement
}

ssize_t
MulticastSendUnreliable::send_bytes_i(const iovec iov[], int n)
{
  return 0; // TODO implement
}

} // namespace DCPS
} // namespace OpenDDS
