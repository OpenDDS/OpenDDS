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

RtpsUdpTransport::RtpsUdpTransport(RtpsUdpInst& inst)
  : TransportImpl(inst)
#if defined(OPENDDS_SECURITY)
  , local_crypto_handle_(DDS::HANDLE_NIL)
#endif
#ifdef OPENDDS_SECURITY
  , ice_endpoint_(*this)
#endif
{
  if (! (configure_i(inst) && open())) {
    throw Transport::UnableToCreate();
  }
}

RtpsUdpInst&
RtpsUdpTransport::config() const
{
  return static_cast<RtpsUdpInst&>(TransportImpl::config());
}

ICE::Endpoint*
RtpsUdpTransport::get_ice_endpoint()
{
#ifdef OPENDDS_SECURITY
  return (config().use_ice_) ? &ice_endpoint_ : 0;
#else
  return 0;
#endif
}

RtpsUdpDataLink_rch
RtpsUdpTransport::make_datalink(const GuidPrefix_t& local_prefix)
{

  RtpsUdpDataLink_rch link = make_rch<RtpsUdpDataLink>(ref(*this), local_prefix, config(), reactor_task());

#if defined(OPENDDS_SECURITY)
  link->local_crypto_handle(local_crypto_handle_);
#endif

  if (config().use_ice_) {
    if (reactor()->remove_handler(unicast_socket_.get_handle(), ACE_Event_Handler::READ_MASK) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("RtpsUdpReceiveStrategy::start_i: ")
                        ACE_TEXT("failed to unregister handler for unicast ")
                        ACE_TEXT("socket %d\n"),
                        unicast_socket_.get_handle()),
                       RtpsUdpDataLink_rch());
    }
  }
  if (!link->open(unicast_socket_)) {
    ACE_ERROR((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::make_datalink: ")
                      ACE_TEXT("failed to open DataLink for socket %d\n"),
                      unicast_socket_.get_handle()));
    return RtpsUdpDataLink_rch();
  }

  // RtpsUdpDataLink now owns the socket
  unicast_socket_.set_handle(ACE_INVALID_HANDLE);

  return link;
}

TransportImpl::AcceptConnectResult
RtpsUdpTransport::connect_datalink(const RemoteTransport& remote,
                                   const ConnectionAttribs& attribs,
                                   const TransportClient_rch& client)
{
  if (is_shut_down_) {
    return AcceptConnectResult();
  }

  GuardThreadType guard_links(links_lock_);

  if (!link_) {
    link_ = make_datalink(attribs.local_id_.guidPrefix);
    if (!link_) {
      return AcceptConnectResult();
    }
  }

  RtpsUdpDataLink_rch link = link_;


  use_datalink(attribs.local_id_, remote.repo_id_, remote.blob_,
               attribs.local_reliable_, remote.reliable_,
               attribs.local_durable_, remote.durable_);

  if (0 == std::memcmp(attribs.local_id_.guidPrefix, remote.repo_id_.guidPrefix,
                       sizeof(GuidPrefix_t))) {
    return AcceptConnectResult(link); // "loopback" connection return link right away
  }

  if (link->check_handshake_complete(attribs.local_id_, remote.repo_id_)){
    return AcceptConnectResult(link);
  }

  link->add_on_start_callback(client, remote.repo_id_);

  GuardType guard(connections_lock_);
  add_pending_connection(client, link);
  VDBG_LVL((LM_DEBUG, "(%P|%t) RtpsUdpTransport::connect_datalink pending.\n"), 2);
  return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);
}

TransportImpl::AcceptConnectResult
RtpsUdpTransport::accept_datalink(const RemoteTransport& remote,
                                  const ConnectionAttribs& attribs,
                                  const TransportClient_rch& client)
{
  GuardThreadType guard_links(links_lock_);

  if (is_shut_down_) {
    return AcceptConnectResult();
  }

  if (!link_) {
    link_ = make_datalink(attribs.local_id_.guidPrefix);
    if (!link_) {
      return AcceptConnectResult();
    }
  }
  RtpsUdpDataLink_rch link = link_;

  use_datalink(attribs.local_id_, remote.repo_id_, remote.blob_,
               attribs.local_reliable_, remote.reliable_,
               attribs.local_durable_, remote.durable_);

  if (0 == std::memcmp(attribs.local_id_.guidPrefix, remote.repo_id_.guidPrefix,
                       sizeof(GuidPrefix_t))) {
    return AcceptConnectResult(link); // "loopback" connection return link right away
  }

  if (link->check_handshake_complete(attribs.local_id_, remote.repo_id_)){
    return AcceptConnectResult(link);
  }

  link->add_on_start_callback(client, remote.repo_id_);

  GuardType guard(connections_lock_);
  add_pending_connection(client, link);
  VDBG_LVL((LM_DEBUG, "(%P|%t) RtpsUdpTransport::accept_datalink pending.\n"), 2);
  return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);
}


void
RtpsUdpTransport::stop_accepting_or_connecting(const TransportClient_wrch& client,
                                               const RepoId& remote_id)
{
  GuardType guard(pending_connections_lock_);
  typedef PendConnMap::iterator iter_t;
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
  unsigned int blob_bytes_read;
  ACE_INET_Addr addr = get_connection_addr(remote_data, &requires_inline_qos,
                                           &blob_bytes_read);

  if (link_) {
    link_->add_locator(remote_id, addr, requires_inline_qos);

#if defined(OPENDDS_SECURITY)
    if (remote_data.length() > blob_bytes_read) {
      link_->populate_security_handles(local_id, remote_id,
                                       remote_data.get_buffer() + blob_bytes_read,
                                       remote_data.length() - blob_bytes_read);
    }
#endif

    link_->associated(local_id, remote_id, local_reliable, remote_reliable,
                      local_durable, remote_durable);
  }
}

ACE_INET_Addr
RtpsUdpTransport::get_connection_addr(const TransportBLOB& remote,
                                      bool* requires_inline_qos,
                                      unsigned int* blob_bytes_read) const
{
  using namespace OpenDDS::RTPS;
  LocatorSeq locators;
  DDS::ReturnCode_t result =
    blob_to_locators(remote, locators, requires_inline_qos, blob_bytes_read);
  if (result != DDS::RETCODE_OK) {
    return ACE_INET_Addr();
  }

  for (CORBA::ULong i = 0; i < locators.length(); ++i) {
    ACE_INET_Addr addr;
    // If conversion was successful
    if (locator_to_address(addr, locators[i], map_ipv4_to_ipv6()) == 0) {
      // if this is a unicast address, or if we are allowing multicast
      if (!addr.is_multicast() || config().use_multicast_) {
        return addr;
      }
    }
  }

  // Return default address
  return ACE_INET_Addr();
}

bool
RtpsUdpTransport::connection_info_i(TransportLocator& info, ConnectionInfoFlags flags) const
{
  config().populate_locator(info, flags);
  return true;
}

void
RtpsUdpTransport::register_for_reader(const RepoId& participant,
                                      const RepoId& writerid,
                                      const RepoId& readerid,
                                      const TransportLocatorSeq& locators,
                                      OpenDDS::DCPS::DiscoveryListener* listener)
{
  const TransportBLOB* blob = config().get_blob(locators);
  if (!blob || is_shut_down_) {
    return;
  }

  GuardThreadType guard_links(links_lock_);

  if (!link_) {
    link_ = make_datalink(participant.guidPrefix);
  }

  link_->register_for_reader(writerid, readerid, get_connection_addr(*blob),
                             listener);
}

void
RtpsUdpTransport::unregister_for_reader(const RepoId& /*participant*/,
                                        const RepoId& writerid,
                                        const RepoId& readerid)
{
  if (link_) {
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
  const TransportBLOB* blob = config().get_blob(locators);
  if (!blob || is_shut_down_) {
    return;
  }

  GuardThreadType guard_links(links_lock_);

  if (!link_) {
    link_ = make_datalink(participant.guidPrefix);
  }

  link_->register_for_writer(readerid, writerid, get_connection_addr(*blob),
                             listener);
}

void
RtpsUdpTransport::unregister_for_writer(const RepoId& /*participant*/,
                                        const RepoId& readerid,
                                        const RepoId& writerid)
{
  if (link_) {
    link_->unregister_for_writer(readerid, writerid);
  }
}

void
RtpsUdpTransport::update_locators(const RepoId& remote,
                                  const TransportLocatorSeq& locators)
{
  const TransportBLOB* blob = config().get_blob(locators);
  if (!blob || is_shut_down_) {
    return;
  }

  GuardThreadType guard_links(links_lock_);

  if (link_) {
    bool requires_inline_qos;
    unsigned int blob_bytes_read;
    ACE_INET_Addr addr = get_connection_addr(*blob, &requires_inline_qos,
                                             &blob_bytes_read);
    link_->add_locator(remote, addr, requires_inline_qos);
  }
}

bool
RtpsUdpTransport::configure_i(RtpsUdpInst& config)
{
  // Override with DCPSDefaultAddress.
  if (config.local_address() == ACE_INET_Addr () &&
      !TheServiceParticipant->default_address ().empty ()) {
    config.local_address(0, TheServiceParticipant->default_address().c_str());
  }

  // Open the socket here so that any addresses/ports left
  // unspecified in the RtpsUdpInst are known by the time we get to
  // connection_info_i().  Opening the sockets here also allows us to
  // detect and report errors during DataReader/Writer setup instead
  // of during association.

  int protocol_family = PF_UNSPEC;
  if (!open_appropriate_socket_type(unicast_socket_, config.local_address(), &protocol_family)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::configure_i: open_appropriate_socket_type:")
                      ACE_TEXT("%m\n")),
                      false);
  }

#ifdef ACE_RECVPKTINFO
  if (protocol_family == PF_INET) {
    int sockopt = 1;
    if (unicast_socket_.set_option(IPPROTO_IP, ACE_RECVPKTINFO, &sockopt, sizeof sockopt) == -1) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsUdpTransport::configure_i: set_option: %m\n")), false);
    }
  }
#endif

  if (config.local_address().get_port_number() == 0) {

    ACE_INET_Addr address;
    if (unicast_socket_.get_local_addr(address) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: RtpsUdpDataLink::configure_i - %p\n"),
        ACE_TEXT("cannot get local addr")), false);
    }
    config.local_address_set_port(address.get_port_number());
  }

  create_reactor_task();

#ifdef OPENDDS_SECURITY
  if (config.use_ice_) {
    ICE::Agent::instance()->add_endpoint(&ice_endpoint_);
    if (reactor()->register_handler(unicast_socket_.get_handle(), &ice_endpoint_,
                                    ACE_Event_Handler::READ_MASK) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("RtpsUdpReceiveStrategy::start_i: ")
                        ACE_TEXT("failed to register handler for unicast ")
                        ACE_TEXT("socket %d\n"),
                        unicast_socket_.get_handle()),
                       false);
    }
  }
#endif

  if (config.opendds_discovery_default_listener_) {
    link_= make_datalink(config.opendds_discovery_guid_.guidPrefix);
    link_->default_listener(*config.opendds_discovery_default_listener_);
  }

  return true;
}

void
RtpsUdpTransport::shutdown_i()
{
  GuardThreadType guard_links(links_lock_);
  if (link_) {
    link_->transport_shutdown();
  }
  link_.reset();

#ifdef OPENDDS_SECURITY
  if(config().use_ice_) {
    ICE::Agent::instance()->remove_endpoint(&ice_endpoint_);
  }
#endif
}

void
RtpsUdpTransport::release_datalink(DataLink* /*link*/)
{
  // No-op for rtps_udp: keep the link_ around until the transport is shut down.
}



bool
RtpsUdpTransport::map_ipv4_to_ipv6() const
{
  bool map = false;
  ACE_INET_Addr tmp;
  const ACE_SOCK_Dgram& socket = link_ ? link_->unicast_socket() : unicast_socket_;
  socket.get_local_addr(tmp);
  if (tmp.get_type() != AF_INET) {
    map = true;
  }
  return map;
}

#ifdef OPENDDS_SECURITY
int
RtpsUdpTransport::IceEndpoint::handle_input(ACE_HANDLE /*fd*/)
{
  struct iovec iov[1];
  char buffer[0x10000];
  iov[0].iov_base = buffer;
  iov[0].iov_len = sizeof buffer;
  ACE_INET_Addr remote_address;

  bool stop;
  RtpsUdpReceiveStrategy::receive_bytes_helper(iov, 1, transport.unicast_socket_, remote_address, transport.get_ice_endpoint(), stop);

  return 0;
}

namespace {
  bool shouldWarn(int code) {
    return code == EPERM || code == EACCES || code == EINTR || code == ENOBUFS || code == ENOMEM;
  }

  ssize_t
  send_single_i(ACE_SOCK_Dgram& socket, const iovec iov[], int n, const ACE_INET_Addr& addr)
  {
#ifdef ACE_LACKS_SENDMSG
    char buffer[UDP_MAX_MESSAGE_SIZE];
    char *iter = buffer;
    for (int i = 0; i < n; ++i) {
      if (size_t(iter - buffer + iov[i].iov_len) > UDP_MAX_MESSAGE_SIZE) {
        ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpSendStrategy::send_single_i() - "
                   "message too large at index %d size %d\n", i, iov[i].iov_len));
        return -1;
      }
      std::memcpy(iter, iov[i].iov_base, iov[i].iov_len);
      iter += iov[i].iov_len;
    }
    const ssize_t result = socket.send(buffer, iter - buffer, addr);
#else
    const ssize_t result = socket.send(iov, n, addr);
#endif
    if (result < 0) {
      ACE_TCHAR addr_buff[256] = {};
      int err = errno;
      addr.addr_to_string(addr_buff, 256);
      errno = err;
      const ACE_Log_Priority prio = shouldWarn(errno) ? LM_WARNING : LM_ERROR;
      ACE_ERROR((prio, "(%P|%t) RtpsUdpSendStrategy::send_single_i() - "
                 "destination %s failed %p\n", addr_buff, ACE_TEXT("send")));
    }
    return result;
  }
}

ICE::AddressListType
RtpsUdpTransport::IceEndpoint::host_addresses() const {
  return transport.config().host_addresses();
}

void
RtpsUdpTransport::IceEndpoint::send(const ACE_INET_Addr& destination, const STUN::Message& message)
{
  ACE_SOCK_Dgram& socket = transport.link_ ? transport.link_->unicast_socket() : transport.unicast_socket_;

  ACE_Message_Block block(20 + message.length());
  DCPS::Serializer serializer(&block, DCPS::Serializer::SWAP_BE);
  const_cast<STUN::Message&>(message).block = &block;
  serializer << message;

  iovec iov[MAX_SEND_BLOCKS];
  const int num_blocks = RtpsUdpSendStrategy::mb_to_iov(block, iov);
  const ssize_t result = send_single_i(socket, iov, num_blocks, destination);
  if (result < 0) {
    const ACE_Log_Priority prio = shouldWarn(errno) ? LM_WARNING : LM_ERROR;
    ACE_ERROR((prio, "(%P|%t) RtpsUdpTransport::send() - "
               "failed to send STUN message\n"));
  }
}

ACE_INET_Addr
RtpsUdpTransport::IceEndpoint::stun_server_address() const {
  return transport.config().stun_server_address();
}
#endif

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
