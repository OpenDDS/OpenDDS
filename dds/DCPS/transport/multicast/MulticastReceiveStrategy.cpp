/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastReceiveStrategy.h"

namespace OpenDDS {
namespace DCPS {

int
MulticastReceiveStrategy::start_i()
{
  return 0; // TODO implement
}

void
MulticastReceiveStrategy::stop_i()
{
  // TODO implement
}

ssize_t
MulticastReceiveStrategy::receive_bytes(iovec iov[],
                                        int n,
                                        ACE_INET_Addr& remote_address)
{
  return 0; // TODO implement
}

} // namespace DCPS
} // namespace OpenDDS
