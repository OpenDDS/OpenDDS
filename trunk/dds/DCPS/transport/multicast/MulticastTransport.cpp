/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastTransport.h"
#include "ReliableMulticast.h"
#include "BestEffortMulticast.h"
#include "MulticastSendStrategy.h"
#include "MulticastReceiveStrategy.h"

#include "ace/CDR_Base.h"
#include "ace/Log_Msg.h"

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
  // This transport forms reservations between DomainParticipants.
  // Given that TransportImpl instances may only be attached either
  // Subscribers or Publishers within the same DomainParticipant,
  // it may be assumed that the local_id always references the same
  // participant. The remote_id may match one or more publications
  // or subscriptions belonging to the same remote participant.
  MulticastPeer remote_peer =
    RepoIdConverter(remote_association->remote_id_).participantId();

  MulticastDataLinkMap::iterator it(this->links_.find(remote_peer));
  if (it != this->links_.end()) return it->second.in();  // found

  // At this point we may assume that we are creating a new DataLink
  // between a logical pair of peers identified by a participantId:
  MulticastPeer local_peer = RepoIdConverter(local_id).participantId();

  // This transport supports two modes of operation: reliable and
  // best-effort; mode selection is based on transport configuration:
  MulticastDataLink_rch link;
  if (this->config_i_->reliable_) {
    link = new ReliableMulticast(this,
                                 local_peer,
                                 remote_peer,
                                 active);
  } else {
    link = new BestEffortMulticast(this,
                                   local_peer,
                                   remote_peer,
                                   active);
  }
  if (link.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::find_or_create_datalink: ")
                      ACE_TEXT("failed to create DataLink for remote peer: 0x%x!\n"),
                      remote_peer),
                     0);
  }

  // Configure link with transport configuration and reactor task:
  link->configure(this->config_i_.in(), reactor_task());

  // Assign send/receive strategies:
  link->send_strategy(new MulticastSendStrategy(link.in()));
  link->receive_strategy(new MulticastReceiveStrategy(link.in()));

  ACE_INET_Addr group_address;
  if (active) {
    // Active peers obtain the group address via the
    // TransportInterfaceBLOB in the TranpsortInterfaceInfo:
    group_address = connection_info_i(remote_association->remote_data_);

  } else {
    // Passive peers obtain the group address via the transport
    // configuration:
    group_address = this->config_i_->group_address_;
  }

  if (!link->join(group_address)) {
    ACE_TCHAR group_address_s[64];
    group_address.addr_to_string(group_address_s, sizeof(group_address_s));
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::find_or_create_datalink: ")
                      ACE_TEXT("failed to join multicast group: %C!\n"),
                      group_address_s),
                     0);
  }

  // Insert new link into the links map; this allows DataLinks to be
  // shared by additional publications or subscriptions belonging to
  // the same participant:
  this->links_.insert(MulticastDataLinkMap::value_type(remote_peer, link));

  return link._retn();
}

int
MulticastTransport::configure_i(TransportConfiguration* config)
{
  this->config_i_ = dynamic_cast<MulticastConfiguration*>(config);
  if (this->config_i_.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     -1);
  }
  this->config_i_->_add_ref();  // take ownership

  return 0;
}

void
MulticastTransport::shutdown_i()
{
  // Shutdown reserved datalinks and release configuration:
  for (MulticastDataLinkMap::iterator it(this->links_.begin());
       it != this->links_.end(); ++it) {
    it->second->transport_shutdown();
  }
  this->links_.clear();

  this->config_i_ = 0;  // release ownership
}

int
MulticastTransport::connection_info_i(TransportInterfaceInfo& info) const
{
  NetworkAddress network_address(this->config_i_->group_address_);

  ACE_OutputCDR cdr;
  cdr << network_address;

  size_t len = cdr.total_length();
  char *buffer = const_cast<char*>(cdr.buffer()); // safe

  // Provide connection information for active peers; active
  // peers will select the group address based on this value.
  info.transport_id = TRANSPORT_INTERFACE_ID;
  info.data = TransportInterfaceBLOB(len, len,
    reinterpret_cast<CORBA::Octet*>(buffer));

  return 0;
}

ACE_INET_Addr
MulticastTransport::connection_info_i(const TransportInterfaceInfo& info) const
{
  if (info.transport_id != TRANSPORT_INTERFACE_ID) {
    ACE_ERROR((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: ")
               ACE_TEXT("MulticastTransport::get_connection_info: ")
               ACE_TEXT("transport interface ID does not match: 0x%x\n"),
               info.transport_id));
  }

  ACE_INET_Addr group_address;
  NetworkAddress network_address;

  size_t len = info.data.length();
  const char* buffer = reinterpret_cast<const char*>(info.data.get_buffer());

  ACE_InputCDR cdr(buffer, len);
  cdr >> network_address;

  network_address.to_addr(group_address);

  return group_address;
}

bool
MulticastTransport::acked(RepoId /*local_id*/, RepoId remote_id)
{
  MulticastPeer remote_peer =
    RepoIdConverter(remote_id).participantId();

  MulticastDataLinkMap::iterator it(this->links_.find(remote_peer));
  if (it == this->links_.end()) return false;  // not found

  return it->second->acked();
}

void
MulticastTransport::remove_ack(RepoId /*local_id*/, RepoId /*remote_id*/)
{
  // Association acks are managed by each individual DataLink; there
  // is no state that needs to be removed by this TransportImpl.
}

void
MulticastTransport::release_datalink_i(DataLink* link, bool /*release_pending*/)
{
  for (MulticastDataLinkMap::iterator it(this->links_.begin());
       it != this->links_.end(); ++it) {
    // We are guaranteed to have exactly one matching DataLink
    // in the map; release any resources held and return.
    if (link == static_cast<DataLink*>(it->second.in())) {
      this->links_.erase(it);
      return;
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS
