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
#include "MulticastDataLink.h"

#include "ace/CDR_Base.h"

#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"

namespace {

const CORBA::Long TRANSPORT_INTERFACE_ID(0x4d435354); // MCST

} // namespace

namespace OpenDDS {
namespace DCPS {

DataLink*
MulticastTransport::find_or_create_datalink(
  RepoId local_id,
  const AssociationData* remote_association,
  CORBA::Long /*priority*/,
  bool active)
{
  // N.B. We form reservations between DomainParticipants; this is
  // is a significant departure from traditional reservations formed
  // between individual subscriptions and publications. Currently, a
  // TransportImpl instance may only be attached to entities within
  // the same DomainParticipant (which is in turn tied to a specific
  // domainId). Given this, we may assume that the local_id always
  // references the same participantId; all we need to associate a
  // DataLink to is the remote participantId.
  long remote_participant =
    RepoIdConverter(remote_association->remote_id_).participantId();

  MulticastDataLinkMap::iterator it = this->links_.find(remote_participant);
  if (it != this->links_.end()) return it->second;  // found existing

  // TODO implement

  return 0;
}

int
MulticastTransport::configure_i(TransportConfiguration* config)
{
  this->config_i_ = dynamic_cast<MulticastConfiguration*>(config);
  if (this->config_i_ == 0) return -1;  // invalid configuration

  this->config_i_->_add_ref();

  return 0;
}

void
MulticastTransport::shutdown_i()
{

  for (MulticastDataLinkMap::iterator it = this->links_.begin();
       it != this->links_.end(); ++it) {
    // TODO close and erase each DataLink
  }

  this->config_i_->_remove_ref();
  this->config_i_ = 0;
}

int
MulticastTransport::connection_info_i(TransportInterfaceInfo& local_info) const
{
  NetworkAddress group_address(this->config_i_->group_address_);

  ACE_OutputCDR cdr;
  cdr << group_address;

  size_t len = cdr.total_length();
  char* buffer = const_cast<char*>(cdr.buffer()); // safe

  // N.B. Provide connection information for active endpoints; active
  // endpoints will select the group address based on this value.
  local_info.transport_id = TRANSPORT_INTERFACE_ID;
  local_info.data = TransportInterfaceBLOB(len, len,
    reinterpret_cast<CORBA::Octet*>(buffer));

  return 0;
}

void
MulticastTransport::release_datalink_i(DataLink* link, bool /*release_pending*/)
{
  for (MulticastDataLinkMap::iterator it = this->links_.begin();
       it != this->links_.end(); ++it) {
    // N.B. We are guaranteed to have exactly one matching DataLink
    // in the map; release any resources held and return.
    if (it->second == link) {
      this->links_.erase(it);
      link->_remove_ref();
      return;
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS
