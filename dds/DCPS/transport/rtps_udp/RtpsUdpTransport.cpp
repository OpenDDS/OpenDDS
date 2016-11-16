/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpTransport.h"
#include "RtpsUdpInst.h"
#include "RtpsUdpInst_rch.h"
#include "RtpsUdpSendStrategy.h"
#include "RtpsUdpReceiveStrategy.h"

#include "dds/DCPS/AssociationData.h"

#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"

#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"

#include "ace/CDR_Base.h"
#include "ace/Log_Msg.h"
#include "ace/Sock_Connect.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RtpsUdpTransport::RtpsUdpTransport(const TransportInst_rch& inst)
  : default_listener_(0)
{
  if (!inst.is_nil()) {
    if (!configure(inst.in())) {
      throw Transport::UnableToCreate();
    }
  }
}

RtpsUdpDataLink*
RtpsUdpTransport::make_datalink(const GuidPrefix_t& local_prefix)
{
  TransportReactorTask_rch rt = reactor_task();
  ACE_NEW_RETURN(link_,
                 RtpsUdpDataLink(this, local_prefix, config_i_.in(), rt.in()),
                 0);

  RtpsUdpSendStrategy* send_strategy;
  ACE_NEW_RETURN(send_strategy, RtpsUdpSendStrategy(link_.in()), 0);
  link_->send_strategy(send_strategy);

  RtpsUdpReceiveStrategy* recv_strategy;
  ACE_NEW_RETURN(recv_strategy, RtpsUdpReceiveStrategy(link_.in()), 0);
  link_->receive_strategy(recv_strategy);

  if (!link_->open(unicast_socket_)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::make_datalink: ")
                      ACE_TEXT("failed to open DataLink for socket %d\n"),
                      unicast_socket_.get_handle()),
                     0);
  }

  // RtpsUdpDataLink now owns the socket
  unicast_socket_.set_handle(ACE_INVALID_HANDLE);

  return RtpsUdpDataLink_rch(link_)._retn();
}

TransportImpl::AcceptConnectResult
RtpsUdpTransport::connect_datalink(const RemoteTransport& remote,
                                   const ConnectionAttribs& attribs,
                                   TransportClient* client )
{
  GuardThreadType guard_links(this->links_lock_);
  RtpsUdpDataLink_rch link = link_;
  if (link_.is_nil()) {
    link = make_datalink(attribs.local_id_.guidPrefix);
    if (link.is_nil()) {
      return AcceptConnectResult();
    }
  }

  use_datalink(attribs.local_id_, remote.repo_id_, remote.blob_,
               attribs.local_reliable_, remote.reliable_,
               attribs.local_durable_, remote.durable_);

  if (0 == std::memcmp(attribs.local_id_.guidPrefix, remote.repo_id_.guidPrefix,
                       sizeof(GuidPrefix_t))) {
    return AcceptConnectResult(link._retn()); // "loopback" connection return link right away
  }

  if (link->check_handshake_complete(attribs.local_id_, remote.repo_id_)){
    return AcceptConnectResult(link._retn());
  }

  if (!link->add_on_start_callback(client, remote.repo_id_)) {
     // link was started by the reactor thread before we could add a callback
     VDBG_LVL((LM_DEBUG, "(%P|%t) RtpsUdpTransport::connect_datalink got link.\n"), 2);
     return AcceptConnectResult(link._retn());
  }

  GuardType guard(connections_lock_);
  add_pending_connection(client, link.in());
  VDBG_LVL((LM_DEBUG, "(%P|%t) RtpsUdpTransport::connect_datalink pending.\n"), 2);
  return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);
}

TransportImpl::AcceptConnectResult
RtpsUdpTransport::accept_datalink(const RemoteTransport& remote,
                                  const ConnectionAttribs& attribs,
                                  TransportClient* )
{
  GuardThreadType guard_links(this->links_lock_);
  RtpsUdpDataLink_rch link = link_;
  if (link_.is_nil()) {
    link = make_datalink(attribs.local_id_.guidPrefix);
    if (link.is_nil()) {
      return AcceptConnectResult();
    }
  }
  use_datalink(attribs.local_id_, remote.repo_id_, remote.blob_,
               attribs.local_reliable_, remote.reliable_,
               attribs.local_durable_, remote.durable_);
  return AcceptConnectResult(link._retn());
}


void
RtpsUdpTransport::stop_accepting_or_connecting(TransportClient* client,
                                               const RepoId& remote_id)
{
  GuardType guard(connections_lock_);
  typedef OPENDDS_MULTIMAP(TransportClient*, DataLink_rch)::iterator iter_t;
  const std::pair<iter_t, iter_t> range =
        pending_connections_.equal_range(client);
  for (iter_t iter = range.first; iter != range.second; ++iter) {
     iter->second->remove_on_start_callback(client, remote_id);
  }
  pending_connections_.erase(range.first, range.second);
}

void
RtpsUdpTransport::use_datalink(const RepoId& local_id,
                               const RepoId& remote_id,
                               const TransportBLOB& remote_data,
                               bool local_reliable, bool remote_reliable,
                               bool local_durable, bool remote_durable)
{
  bool requires_inline_qos;
  ACE_INET_Addr addr = get_connection_addr(remote_data, requires_inline_qos);
  link_->add_locator(remote_id, addr, requires_inline_qos);
  link_->associated(local_id, remote_id, local_reliable, remote_reliable,
                    local_durable, remote_durable);
}

ACE_INET_Addr
RtpsUdpTransport::get_connection_addr(const TransportBLOB& remote,
                                      bool& requires_inline_qos) const
{
  using namespace OpenDDS::RTPS;
  LocatorSeq locators;
  DDS::ReturnCode_t result =
    blob_to_locators(remote, locators, requires_inline_qos);
  if (result != DDS::RETCODE_OK) {
    return ACE_INET_Addr();
  }

  for (CORBA::ULong i = 0; i < locators.length(); ++i) {
    ACE_INET_Addr addr;
    // If conversion was successful
    if (locator_to_address(addr, locators[i], map_ipv4_to_ipv6()) == 0) {
      // if this is a unicast address, or if we are allowing multicast
      if (!addr.is_multicast() || config_i_->use_multicast_) {
        return addr;
      }
    }
  }

  // Return default address
  return ACE_INET_Addr();
}

bool
RtpsUdpTransport::connection_info_i(TransportLocator& info) const
{
  this->config_i_->populate_locator(info);
  return true;
}

void
RtpsUdpTransport::register_for_reader(const RepoId& participant,
                                      const RepoId& writerid,
                                      const RepoId& readerid,
                                      const TransportLocatorSeq& locators,
                                      OpenDDS::DCPS::DiscoveryListener* listener)
{
  const TransportBLOB* blob = this->config_i_->get_blob(locators);
  if (!blob)
    return;
  if (link_ == 0) {
    make_datalink(participant.guidPrefix);
  }
  bool requires_inline_qos;
  link_->register_for_reader(writerid, readerid, get_connection_addr(*blob, requires_inline_qos), listener);
}

void
RtpsUdpTransport::unregister_for_reader(const RepoId& /*participant*/,
                                        const RepoId& writerid,
                                        const RepoId& readerid)
{
  if (link_ != 0) {
    link_->unregister_for_reader(writerid, readerid);
  }
}

void
RtpsUdpTransport::register_for_writer(const RepoId& participant,
                                      const RepoId& readerid,
                                      const RepoId& writerid,
                                      const TransportLocatorSeq& locators,
                                      DiscoveryListener* listener)
{
  const TransportBLOB* blob = this->config_i_->get_blob(locators);
  if (!blob)
    return;
  if (link_ == 0) {
    make_datalink(participant.guidPrefix);
  }
  bool requires_inline_qos;
  link_->register_for_writer(readerid, writerid, get_connection_addr(*blob, requires_inline_qos), listener);
}

void
RtpsUdpTransport::unregister_for_writer(const RepoId& /*participant*/,
                                        const RepoId& readerid,
                                        const RepoId& writerid)
{
  if (link_ != 0) {
    link_->unregister_for_writer(readerid, writerid);
  }
}

bool
RtpsUdpTransport::configure_i(TransportInst* config)
{
  config_i_ = RtpsUdpInst_rch(dynamic_cast<RtpsUdpInst*>(config), false);

  if (config_i_.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     false);
  }

  // Override with DCPSDefaultAddress.
  if (this->config_i_->local_address() == ACE_INET_Addr () &&
      !TheServiceParticipant->default_address ().empty ()) {
    this->config_i_->local_address(0, TheServiceParticipant->default_address ().c_str ());
  }

  // Open the socket here so that any addresses/ports left
  // unspecified in the RtpsUdpInst are known by the time we get to
  // connection_info_i().  Opening the sockets here also allows us to
  // detect and report errors during DataReader/Writer setup instead
  // of during association.

  if (!open_appropriate_socket_type(unicast_socket_, config_i_->local_address())) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::configure_i: open_appropriate_socket_type:")
                      ACE_TEXT("%m\n")),
                      false);
  }

  if (config_i_->local_address().get_port_number() == 0) {

    ACE_INET_Addr address;
    if (unicast_socket_.get_local_addr(address) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: RtpsUdpDataLink::configure_i - %p\n"),
        ACE_TEXT("cannot get local addr")), false);
    }
    config_i_->local_address_set_port(address.get_port_number());
  }

  create_reactor_task();

  if (config_i_->opendds_discovery_default_listener_) {
    RtpsUdpDataLink_rch link =
      make_datalink(config_i_->opendds_discovery_guid_.guidPrefix);
    link->default_listener(config_i_->opendds_discovery_default_listener_);
    default_listener_ =
      dynamic_cast<TransportClient*>(config_i_->opendds_discovery_default_listener_);
  }

  return true;
}

void
RtpsUdpTransport::shutdown_i()
{
  if (!link_.is_nil()) {
    link_->transport_shutdown();
  }
  link_ = 0;
  config_i_ = 0;
}

void
RtpsUdpTransport::release_datalink(DataLink* /*link*/)
{
  // No-op for rtps_udp: keep the link_ around until the transport is shut down.
}

void
RtpsUdpTransport::pre_detach(TransportClient* c)
{
  if (default_listener_ && !link_.is_nil() && c == default_listener_) {
    link_->default_listener(0);
    default_listener_ = 0;
  }
}

bool
RtpsUdpTransport::map_ipv4_to_ipv6() const
{
  bool map = false;
  ACE_INET_Addr tmp;
  link_->unicast_socket().get_local_addr(tmp);
  if (tmp.get_type() != AF_INET) {
    map = true;
  }
  return map;
}
} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
