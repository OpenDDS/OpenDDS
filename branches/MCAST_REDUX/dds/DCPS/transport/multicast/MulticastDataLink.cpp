/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastDataLink.h"
#include "MulticastTransport.h"
#include "MulticastConfiguration.h"

namespace OpenDDS {
namespace DCPS {

MulticastDataLink::MulticastDataLink(MulticastTransport* impl,
                                     long local_id,
				     CORBA::Long priority,
                                     bool active)
  : DataLink(impl, priority),
    local_id_(local_id),
    active_(active)
{
}

bool
MulticastDataLink::open(long remote_id)
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
