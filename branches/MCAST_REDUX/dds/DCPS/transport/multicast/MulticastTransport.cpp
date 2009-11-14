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

#include "dds/DCPS/RepoIdConverter.h"

namespace {

const CORBA::Long TRANSPORT_INTERFACE_ID(0x4d435354); // MCST

} // namespace

namespace OpenDDS {
namespace DCPS {

MulticastTransport::~MulticastTransport()
{
  if (this->config_i_ != 0) {
    this->config_i_->_remove_ref();
  }
}

DataLink*
MulticastTransport::find_or_create_datalink(
  RepoId local_id,
  const AssociationData* remote_association,
  CORBA::Long /*priority*/,
  bool active)
{
  // N.B. We form reservations between DomainParticipants; this is
  // is a significant departure from traditional reservations formed
  // formed between Subscriptions and Publications. Currently, a
  // TransportImpl instance may only be attached to entities within
  // the same DomainParticipant (which is in turn is tied to a
  // specific domainId). Given this, we may assume that the local_id
  // always has the same participantId.
  long remote_id =
    RepoIdConverter(remote_association->remote_id_).participantId();

  datalink_map::iterator it = this->datalinks_.find(remote_id);
  if (it != this->datalinks_.end()) return it->second;  // found existing

  // At this point we can assume we are creating a new connection
  // between participants; TODO implement

  return 0;
}

int
MulticastTransport::configure_i(TransportConfiguration* config)
{
  this->config_i_ = dynamic_cast<MulticastConfiguration*>(config);
  if (this->config_i_ == 0) return -1;  // invalid configuration

  // The ETF will only call configure_i once during
  // our lifetime; increment ref count.
  this->config_i_->_add_ref();

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
  // Both endpoints already know the group address by merit of
  // having a configured TransportImpl; no additional information
  // needs to be exchanged using the TransportInterfaceBLOB.
  local_info.transport_id = TRANSPORT_INTERFACE_ID;
  return 0;
}

void
MulticastTransport::release_datalink_i(DataLink* link, bool /*release_pending*/)
{
  for (datalink_map::iterator it = this->datalinks_.begin();
       it != this->datalinks_.end(); ++it) {
    // We are guaranteed to have only one matching DataLink
    // in the map; release any resources held and return.
    if (it->second == link) {
      this->datalinks_.erase(it);
      link->_remove_ref();
      return;
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS
