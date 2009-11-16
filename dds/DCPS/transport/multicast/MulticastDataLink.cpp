/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastDataLink.h"
#include "MulticastConfiguration.h"

namespace OpenDDS {
namespace DCPS {

MulticastDataLink::MulticastDataLink(MulticastTransport* impl,
				     CORBA::Long priority,
                                     long local_peer,
                                     long remote_peer)
  : DataLink(impl, priority),
    impl_i_(impl),
    local_peer_(local_peer),
    remote_peer_(remote_peer)
{
}

bool
MulticastDataLink::open(const ACE_INET_Addr& group_address, bool active)
{
  return false; // TODO implement
}

void
MulticastDataLink::close()
{
  // TODO implement
}

void
MulticastDataLink::stop_i()
{
  // TODO implement
}

} // namespace DCPS
} // namespace OpenDDS
