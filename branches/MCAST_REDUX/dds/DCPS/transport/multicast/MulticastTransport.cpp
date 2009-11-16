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
#include "ace/Log_Msg.h"

#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"

#ifndef __ACE_INLINE__
# include "MulticastTransport.inl"
#endif  /* __ACE_INLINE__ */

namespace {

const CORBA::Long TRANSPORT_INTERFACE_ID(0x4d435354); // MCST

} // namespace

namespace OpenDDS {
namespace DCPS {

DataLink*
MulticastTransport::find_or_create_datalink(
  RepoId local_id,
  const AssociationData* remote_association,
  CORBA::Long priority,
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
  long remote_peer =
    RepoIdConverter(remote_association->remote_id_).participantId();

  MulticastDataLinkMap::iterator it = this->links_.find(remote_peer);
  if (it != this->links_.end()) return it->second;  // found existing

  // At this point we can assume that we are creating a new DataLink
  // to a pair of DomainParticipants identified by their participantIds.
  long local_peer = RepoIdConverter(local_id).participantId();

  MulticastDataLink* link;
  ACE_NEW_RETURN(link,
                 MulticastDataLink(this,
                                   priority,
                                   local_peer,
                                   remote_peer),
                 0);

  ACE_INET_Addr group_address;
  if (active) {
    // Active peers obtain the group address via the
    // TransportInterfaceBLOB in the TranpsortInterfaceInfo:
    group_address = get_connection_info(remote_association->remote_data_);

  } else {
    // Passive peers obtain the group address via the
    // transport configuration:
    group_address = this->config_i_->group_address_;
  }

  if (!link->open(group_address, active)) {
    link->_remove_ref();
    ACE_ERROR_RETURN((LM_ERROR,
		      ACE_TEXT("(%P|%t) ERROR: ")
		      ACE_TEXT("MulticastTransport::find_or_create_datalink: ")
		      ACE_TEXT("unable to open with remote peer: %d\n"),
		      remote_peer), 0);
  }

  std::pair<MulticastDataLinkMap::iterator, bool> pair =
    this->links_.insert(MulticastDataLinkMap::value_type(remote_peer, link));
  if (!pair.second) {
    link->_remove_ref();
    ACE_ERROR_RETURN((LM_ERROR,
		      ACE_TEXT("(%P|%t) ERROR: ")
		      ACE_TEXT("MulticastTransport::find_or_create_datalink: ")
		      ACE_TEXT("unable to reserve link to remote peer: %d\n"),
		      remote_peer), 0);
  }

  return link;
}

int
MulticastTransport::configure_i(TransportConfiguration* config)
{
  this->config_i_ = dynamic_cast<MulticastConfiguration*>(config);
  if (this->config_i_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     -1);
  }
  this->config_i_->_add_ref();

  return 0;
}

void
MulticastTransport::shutdown_i()
{
  // Close reserved datalinks and configuration; release
  // any resources held.
  for (MulticastDataLinkMap::iterator it = this->links_.begin();
       it != this->links_.end(); ++it) {
    MulticastDataLink* link = it->second;
    link->close();
    link->_remove_ref();
  }
  this->links_.clear();

  this->config_i_->_remove_ref();
  this->config_i_ = 0;
}

int
MulticastTransport::connection_info_i(TransportInterfaceInfo& info) const
{
  NetworkAddress network_address(this->config_i_->group_address_);

  ACE_OutputCDR cdr;
  cdr << network_address;

  size_t len = cdr.total_length();
  char *buffer = const_cast<char*>(cdr.buffer()); // safe

  // Provide connection information for active endpoints; active
  // endpoints will select the group address based on this value.
  info.transport_id = TRANSPORT_INTERFACE_ID;
  info.data = TransportInterfaceBLOB(len, len,
    reinterpret_cast<CORBA::Octet*>(buffer));

  return 0;
}

ACE_INET_Addr
MulticastTransport::get_connection_info(const TransportInterfaceInfo& info) const
{
  if (info.transport_id != TRANSPORT_INTERFACE_ID) {
    ACE_ERROR((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: ")
               ACE_TEXT("MulticastTransport::get_connection_info: ")
               ACE_TEXT("transport interface does not match ours: 0x%x\n"),
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

void
MulticastTransport::release_datalink_i(DataLink* link, bool /*release_pending*/)
{
  for (MulticastDataLinkMap::iterator it = this->links_.begin();
       it != this->links_.end(); ++it) {
    // We are guaranteed to have exactly one matching DataLink
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
