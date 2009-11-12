/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastTransport.h"
#include "MulticastConfiguration.h"

#include "ace/CDR_Base.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"

namespace OpenDDS {
namespace DCPS {

const CORBA::Long MulticastTransport::TRANSPORT_ID(0x4d435354); // MCST

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
  this->config_i_ = dynamic_cast<MulticastConfiguration*>(config);
  if (this->config_i_ == 0) return -1;  // invalid configuration
  return 0;
}

void
MulticastTransport::shutdown_i()
{
  // TODO implement
}

int
MulticastTransport::connection_info_i(TransportInterfaceInfo& local_info) const
{
  NetworkAddress group_address(this->config_i_->group_address_);

  ACE_OutputCDR cdr;
  cdr << group_address;

  size_t len = cdr.total_length();
  char* buffer = const_cast<char*>(cdr.buffer()); // safe

  local_info.transport_id = TRANSPORT_ID;
  local_info.data = TransportInterfaceBLOB(len, len,
    reinterpret_cast<CORBA::Octet*>(buffer));

  return 0;
}

void
MulticastTransport::release_datalink_i(DataLink* link, bool release_pending)
{
  // TODO implement
}

} // namespace DCPS
} // namespace OpenDDS
