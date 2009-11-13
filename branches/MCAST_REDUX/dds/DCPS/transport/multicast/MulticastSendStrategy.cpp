/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSendStrategy.h"

namespace OpenDDS {
namespace DCPS {

MulticastSendStrategy::MulticastSendStrategy(TransportConfiguration* config,
                                             CORBA::Long priority)
  : TransportSendStrategy(config, 0, priority)
{
}

void
MulticastSendStrategy::stop_i()
{
  // TODO implement
}

ACE_HANDLE
MulticastSendStrategy::get_handle()
{
  return 0; // TODO implement
}

ssize_t
MulticastSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  return 0; // TODO implement
}

} // namespace DCPS
} // namespace OpenDDS
