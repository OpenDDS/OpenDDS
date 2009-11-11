/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastTransport.h"

namespace OpenDDS {
namespace DCPS {

MulticastTransport::~MulticastTransport()
{
}

DataLink*
MulticastTransport::find_or_create_datalink(
  RepoId                  local_id,
  const AssociationData*  remote_association,
  CORBA::Long             priority,
  bool                    active)
{
  return 0; // TODO implement
}

int
MulticastTransport::configure_i(TransportConfiguration* config)
{
  return 0; // TODO implement
}

void
MulticastTransport::shutdown_i()
{
  // TODO implement
}

int
MulticastTransport::connection_info_i(TransportInterfaceInfo& local_info) const
{
  return 0; // TODO implement
}

void
MulticastTransport::release_datalink_i(DataLink* link, bool release_pending)
{
  // TODO implement
}

} // namespace DCPS
} // namespace OpenDDS
