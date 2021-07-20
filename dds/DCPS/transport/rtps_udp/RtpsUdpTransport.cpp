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
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DiscoveryBase.h"
#include <dds/DCPS/LogAddr.h>

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
  assign(local_prefix_, GUIDPREFIX_UNKNOWN);
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
  return (config().use_ice()) ? &ice_endpoint_ : 0;
#else
  return 0;
#endif
}

void
RtpsUdpTransport::rtps_relay_only_now(bool flag)
{
  ACE_UNUSED_ARG(flag);

#ifdef OPENDDS_SECURITY
  if (flag) {
    relay_stun_task_->enable(false, ICE::Configuration::instance()->server_reflexive_address_period());
  } else {
    if (!config().use_rtps_relay()) {
      disable_relay_stun_task();
    }
  }
#endif
}

void
RtpsUdpTransport::use_rtps_relay_now(bool flag)
{
  ACE_UNUSED_ARG(flag);

#ifdef OPENDDS_SECURITY
  if (flag) {
    relay_stun_task_->enable(false, ICE::Configuration::instance()->server_reflexive_address_period());
  } else {
    if (!config().rtps_relay_only()) {
      disable_relay_stun_task();
    }
  }
#endif
}

void
RtpsUdpTransport::use_ice_now(bool after)
{
  ACE_UNUSED_ARG(after);

#ifdef OPENDDS_SECURITY
  const bool before = config().use_ice();
  config().use_ice(after);

  if (before && !after) {
    stop_ice();
  } else if (!before && after) {
    start_ice();
  }
#endif
}

RtpsUdpDataLink_rch
RtpsUdpTransport::make_datalink(const GuidPrefix_t& local_prefix)
{
  OPENDDS_ASSERT(!equal_guid_prefixes(local_prefix, GUIDPREFIX_UNKNOWN));

  if (equal_guid_prefixes(local_prefix_, GUIDPREFIX_UNKNOWN)) {
    assign(local_prefix_, local_prefix);
#ifdef OPENDDS_SECURITY
    relay_stun_task(DCPS::MonotonicTimePoint::now());
#endif
  }

  RtpsUdpDataLink_rch link = make_rch<RtpsUdpDataLink>(ref(*this), local_prefix, config(), reactor_task());

#if defined(OPENDDS_SECURITY)
  link->local_crypto_handle(local_crypto_handle_);
#endif

  if (config().use_ice()) {
    if (reactor()->remove_handler(unicast_socket_.get_handle(), ACE_Event_Handler::READ_MASK) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("RtpsUdpTransport::make_datalink: ")
                        ACE_TEXT("failed to unregister handler for unicast ")
                        ACE_TEXT("socket %d\n"),
                        unicast_socket_.get_handle()),
                       RtpsUdpDataLink_rch());
    }
#ifdef ACE_HAS_IPV6
    if (reactor()->remove_handler(ipv6_unicast_socket_.get_handle(), ACE_Event_Handler::READ_MASK) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("RtpsUdpTransport::make_datalink: ")
                        ACE_TEXT("failed to unregister handler for ipv6 unicast ")
                        ACE_TEXT("socket %d\n"),
                        ipv6_unicast_socket_.get_handle()),
                       RtpsUdpDataLink_rch());
    }
#endif
  }

  if (!link->open(unicast_socket_
#ifdef ACE_HAS_IPV6
                  , ipv6_unicast_socket_
#endif
                  )) {
#ifdef ACE_HAS_IPV6
    const ACE_HANDLE v6handle = ipv6_unicast_socket_.get_handle();
#else
    const ACE_HANDLE v6handle = ACE_INVALID_HANDLE;
#endif
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("RtpsUdpTransport::make_datalink: ")
               ACE_TEXT("failed to open DataLink for sockets %d %d\n"),
               unicast_socket_.get_handle(), v6handle
               ));
    return RtpsUdpDataLink_rch();
  }

  // RtpsUdpDataLink now owns the socket
  unicast_socket_.set_handle(ACE_INVALID_HANDLE);
#ifdef ACE_HAS_IPV6
  ipv6_unicast_socket_.set_handle(ACE_INVALID_HANDLE);
#endif

  return link;
}

TransportImpl::AcceptConnectResult
RtpsUdpTransport::connect_datalink(const RemoteTransport& remote,
                                   const ConnectionAttribs& attribs,
                                   const TransportClient_rch& client)
{
  bit_sub_ = client->get_builtin_subscriber();

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

  if (use_datalink(attribs.local_id_, remote.repo_id_, remote.blob_, remote.participant_discovered_at_,
                   remote.context_,
                   attribs.local_reliable_, remote.reliable_,
                   attribs.local_durable_, remote.durable_, attribs.max_sn_, client)) {
    return AcceptConnectResult(link);
  }

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
  bit_sub_ = client->get_builtin_subscriber();

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

  if (use_datalink(attribs.local_id_, remote.repo_id_, remote.blob_, remote.participant_discovered_at_,
                   remote.context_,
                   attribs.local_reliable_, remote.reliable_,
                   attribs.local_durable_, remote.durable_, attribs.max_sn_, client)) {
    return AcceptConnectResult(link);
  }

  GuardType guard(connections_lock_);
  add_pending_connection(client, link);
  VDBG_LVL((LM_DEBUG, "(%P|%t) RtpsUdpTransport::accept_datalink pending.\n"), 2);
  return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);
}


void
RtpsUdpTransport::stop_accepting_or_connecting(const TransportClient_wrch& client,
                                               const RepoId& remote_id,
                                               bool disassociate,
                                               bool association_failed)
{
  if (disassociate || association_failed) {
    GuardThreadType guard_links(links_lock_);
    if (link_) {
      TransportClient_rch c = client.lock();
      if (c) {
        link_->disassociated(c->get_repo_id(), remote_id);
      }
    }
  }

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
}

bool
RtpsUdpTransport::use_datalink(const RepoId& local_id,
                               const RepoId& remote_id,
                               const TransportBLOB& remote_data,
                               const MonotonicTime_t& participant_discovered_at,
                               ACE_CDR::ULong participant_flags,
                               bool local_reliable, bool remote_reliable,
                               bool local_durable, bool remote_durable,
                               SequenceNumber max_sn,
                               const TransportClient_rch& client)
{
  bool requires_inline_qos;
  unsigned int blob_bytes_read;
  std::pair<AddrSet, AddrSet> addrs =
    get_connection_addrs(remote_data, &requires_inline_qos, &blob_bytes_read);

  if (link_) {
    return link_->associated(local_id, remote_id, local_reliable, remote_reliable,
                             local_durable, remote_durable,
                             participant_discovered_at, participant_flags, max_sn, client,
                             addrs.first, addrs.second, requires_inline_qos);
  }

  return true;
}

std::pair<AddrSet, AddrSet>
RtpsUdpTransport::get_connection_addrs(const TransportBLOB& remote,
                                       bool* requires_inline_qos,
                                       unsigned int* blob_bytes_read) const
{
  using namespace OpenDDS::RTPS;
  LocatorSeq locators;
  DDS::ReturnCode_t result =
    blob_to_locators(remote, locators, requires_inline_qos, blob_bytes_read);
  if (result != DDS::RETCODE_OK) {
    return std::make_pair(AddrSet(), AddrSet());
  }

  AddrSet uc_addrs;
  AddrSet mc_addrs;
  for (CORBA::ULong i = 0; i < locators.length(); ++i) {
    ACE_INET_Addr addr;
    // If conversion was successful
    if (locator_to_address(addr, locators[i], false) == 0) {
      if (addr.is_multicast()) {
        if (config().use_multicast_) {
          mc_addrs.insert(addr);
        }
      } else {
        uc_addrs.insert(addr);
      }
    }
  }

  return std::make_pair(uc_addrs, mc_addrs);
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

  link_->register_for_reader(writerid, readerid, get_connection_addrs(*blob).first,
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

  link_->register_for_writer(readerid, writerid, get_connection_addrs(*blob).first,
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
    std::pair<AddrSet, AddrSet> addrs =
      get_connection_addrs(*blob, &requires_inline_qos, &blob_bytes_read);
    link_->update_locators(remote, addrs.first, addrs.second, requires_inline_qos, false);
  }
}

bool
RtpsUdpTransport::configure_i(RtpsUdpInst& config)
{
  // Override with DCPSDefaultAddress.
  if (config.local_address() == ACE_INET_Addr() &&
      TheServiceParticipant->default_address() != ACE_INET_Addr()) {
    config.local_address(TheServiceParticipant->default_address());
  }
  if (config.multicast_interface_.empty() &&
    TheServiceParticipant->default_address() != ACE_INET_Addr()) {
    config.multicast_interface_ = DCPS::LogAddr::ip(TheServiceParticipant->default_address());
  }

  // Open the socket here so that any addresses/ports left
  // unspecified in the RtpsUdpInst are known by the time we get to
  // connection_info_i().  Opening the sockets here also allows us to
  // detect and report errors during DataReader/Writer setup instead
  // of during association.

  ACE_INET_Addr address = config.local_address();

  if (unicast_socket_.open(address, PF_INET) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::configure_i: open:")
                      ACE_TEXT("%m\n")),
                     false);
  }

#ifdef ACE_WIN32
  // By default Winsock will cause reads to fail with "connection reset"
  // when UDP sends result in ICMP "port unreachable" messages.
  // The transport framework is not set up for this since returning <= 0
  // from our receive_bytes causes the framework to close down the datalink
  // which in this case is used to receive from multiple peers.
  {
    BOOL recv_udp_connreset = FALSE;
    unicast_socket_.control(SIO_UDP_CONNRESET, &recv_udp_connreset);
  }
#endif

  if (unicast_socket_.get_local_addr(address) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::configure_i: get_local_addr:")
                      ACE_TEXT("%m\n")),
                     false);
  }

  config.local_address(address);

#ifdef ACE_RECVPKTINFO
  int sockopt = 1;
  if (unicast_socket_.set_option(IPPROTO_IP, ACE_RECVPKTINFO, &sockopt, sizeof sockopt) == -1) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsUdpTransport::configure_i: set_option: %m\n")), false);
  }
#endif

#ifdef ACE_HAS_IPV6
  address = config.ipv6_local_address();

  if (ipv6_unicast_socket_.open(address, PF_INET6) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::configure_i: open:")
                      ACE_TEXT("%m\n")),
                     false);
  }

#ifdef ACE_WIN32
  // By default Winsock will cause reads to fail with "connection reset"
  // when UDP sends result in ICMP "port unreachable" messages.
  // The transport framework is not set up for this since returning <= 0
  // from our receive_bytes causes the framework to close down the datalink
  // which in this case is used to receive from multiple peers.
  {
    BOOL recv_udp_connreset = FALSE;
    ipv6_unicast_socket_.control(SIO_UDP_CONNRESET, &recv_udp_connreset);
  }
#endif

  if (ipv6_unicast_socket_.get_local_addr(address) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::configure_i: get_local_addr:")
                      ACE_TEXT("%m\n")),
                     false);
  }

  config.ipv6_local_address(address);

#ifdef ACE_RECVPKTINFO6
  if (ipv6_unicast_socket_.set_option(IPPROTO_IPV6, ACE_RECVPKTINFO6, &sockopt, sizeof sockopt) == -1) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsUdpTransport::configure_i: set_option: %m\n")), false);
  }
#endif
#endif

  create_reactor_task(false, "RtpsUdpTransport" + config.name());

  ACE_Reactor* reactor = reactor_task_->get_reactor();
  job_queue_ = DCPS::make_rch<DCPS::JobQueue>(reactor);

#ifdef OPENDDS_SECURITY
  if (config.use_ice()) {
    start_ice();
  }

  relay_stun_task_= make_rch<Periodic>(reactor_task()->interceptor(), ref(*this), &RtpsUdpTransport::relay_stun_task);

  if (config.use_rtps_relay() || config.rtps_relay_only()) {
    relay_stun_task_->enable(false, ICE::Configuration::instance()->server_reflexive_address_period());
  }
#endif

  if (config.opendds_discovery_default_listener_) {
    link_ = make_datalink(config.opendds_discovery_guid_.guidPrefix);
    link_->default_listener(*config.opendds_discovery_default_listener_);
  }

  return true;
}

void RtpsUdpTransport::client_stop(const RepoId& localId)
{
  GuardThreadType guard_links(links_lock_);
  const RtpsUdpDataLink_rch link = link_;
  guard_links.release();
  if (link) {
    link->client_stop(localId);
  }
}

void
RtpsUdpTransport::shutdown_i()
{
#ifdef OPENDDS_SECURITY
  if(config().use_ice()) {
    stop_ice();
  }

  relay_stun_task_->disable_and_wait();
#endif

  job_queue_.reset();

  GuardThreadType guard_links(links_lock_);
  if (link_) {
    link_->transport_shutdown();
  }
  link_.reset();
}

void
RtpsUdpTransport::release_datalink(DataLink* /*link*/)
{
  // No-op for rtps_udp: keep the link_ around until the transport is shut down.
}



#ifdef OPENDDS_SECURITY

const ACE_SOCK_Dgram&
RtpsUdpTransport::IceEndpoint::choose_recv_socket(ACE_HANDLE fd) const
{
#ifdef ACE_HAS_IPV6
  if (fd == transport.ipv6_unicast_socket_.get_handle()) {
    return transport.ipv6_unicast_socket_;
  }
#endif
  ACE_UNUSED_ARG(fd);
  return transport.unicast_socket_;
}

int
RtpsUdpTransport::IceEndpoint::handle_input(ACE_HANDLE fd)
{
  struct iovec iov[1];
  char buffer[0x10000];
  iov[0].iov_base = buffer;
  iov[0].iov_len = sizeof buffer;
  ACE_INET_Addr remote_address;

  bool stop;
  RtpsUdpReceiveStrategy::receive_bytes_helper(iov, 1, choose_recv_socket(fd), remote_address, transport.get_ice_endpoint(), transport, stop);

  return 0;
}

namespace {
  bool shouldWarn(int code) {
    return code == EPERM || code == EACCES || code == EINTR || code == ENOBUFS || code == ENOMEM
      || code == EADDRNOTAVAIL || code == ENETUNREACH;
  }

  ssize_t
  send_single_i(ACE_SOCK_Dgram& socket, const iovec iov[], int n, const ACE_INET_Addr& addr, bool& network_is_unreachable)
  {
    OPENDDS_ASSERT(addr != ACE_INET_Addr());
#ifdef ACE_LACKS_SENDMSG
    char buffer[UDP_MAX_MESSAGE_SIZE];
    char *iter = buffer;
    for (int i = 0; i < n; ++i) {
      if (size_t(iter - buffer + iov[i].iov_len) > UDP_MAX_MESSAGE_SIZE) {
        ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpTransport.cpp send_single_i() - "
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
      const int err = errno;
      if (err != ENETUNREACH || !network_is_unreachable) {
        errno = err;
        const ACE_Log_Priority prio = shouldWarn(errno) ? LM_WARNING : LM_ERROR;
        ACE_ERROR((prio, "(%P|%t) RtpsUdpTransport.cpp send_single_i() - "
                   "destination %C failed %p\n", DCPS::LogAddr(addr).c_str(), ACE_TEXT("send")));
      }
      if (err == ENETUNREACH) {
        network_is_unreachable = true;
      }
      // Reset errno since the rest of framework expects it.
      errno = err;
    } else {
      network_is_unreachable = false;
    }
    return result;
  }
}

ICE::AddressListType
RtpsUdpTransport::IceEndpoint::host_addresses() const {
  ICE::AddressListType addresses;

  ACE_INET_Addr addr = transport.config().local_address();
  if (addr != ACE_INET_Addr()) {
    if (addr.is_any()) {
      ICE::AddressListType addrs;
      DCPS::get_interface_addrs(addrs);
      for (ICE::AddressListType::iterator pos = addrs.begin(), limit = addrs.end(); pos != limit; ++pos) {
        if (pos->get_type() == AF_INET) {
          pos->set_port_number(addr.get_port_number());
          addresses.push_back(*pos);
        }
      }
    } else {
      addresses.push_back(addr);
    }
  }

#ifdef ACE_HAS_IPV6
  addr = transport.config().ipv6_local_address();
  if (addr != ACE_INET_Addr()) {
    if (addr.is_any()) {
      ICE::AddressListType addrs;
      DCPS::get_interface_addrs(addrs);
      for (ICE::AddressListType::iterator pos = addrs.begin(), limit = addrs.end(); pos != limit; ++pos) {
        if (pos->get_type() == AF_INET6) {
          pos->set_port_number(addr.get_port_number());
          addresses.push_back(*pos);
        }
      }
    } else {
      addresses.push_back(addr);
    }
  }
#endif

  return addresses;
}

ACE_SOCK_Dgram&
RtpsUdpTransport::IceEndpoint::choose_send_socket(const ACE_INET_Addr& destination) const
{
#ifdef ACE_HAS_IPV6
  if (destination.get_type() == AF_INET6) {
    return transport.link_ ? transport.link_->ipv6_unicast_socket() : transport.ipv6_unicast_socket_;
  }
#endif

  ACE_UNUSED_ARG(destination);
  return transport.link_ ? transport.link_->unicast_socket() : transport.unicast_socket_;
}

void
RtpsUdpTransport::IceEndpoint::send(const ACE_INET_Addr& destination, const STUN::Message& message)
{
  ACE_SOCK_Dgram& socket = choose_send_socket(destination);

  ACE_Message_Block block(20 + message.length());
  DCPS::Serializer serializer(&block, STUN::encoding);
  const_cast<STUN::Message&>(message).block = &block;
  serializer << message;

  iovec iov[MAX_SEND_BLOCKS];
  const int num_blocks = RtpsUdpSendStrategy::mb_to_iov(block, iov);
  const ssize_t result = send_single_i(socket, iov, num_blocks, destination, network_is_unreachable_);
  if (result < 0 && !network_is_unreachable_) {
    const ACE_Log_Priority prio = shouldWarn(errno) ? LM_WARNING : LM_ERROR;
    ACE_ERROR((prio, "(%P|%t) RtpsUdpTransport::send() - "
               "failed to send STUN message\n"));
  }
}

ACE_INET_Addr
RtpsUdpTransport::IceEndpoint::stun_server_address() const {
  return transport.config().stun_server_address();
}

void
RtpsUdpTransport::start_ice()
{
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, "(%P|%t) RtpsUdpTransport::start_ice\n"));
  }

  ICE::Agent::instance()->add_endpoint(&ice_endpoint_);

  if (!link_) {
    if (reactor()->register_handler(unicast_socket_.get_handle(), &ice_endpoint_,
                                    ACE_Event_Handler::READ_MASK) != 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("RtpsUdpTransport::start_ice: ")
                 ACE_TEXT("failed to register handler for unicast ")
                 ACE_TEXT("socket %d\n"),
                 unicast_socket_.get_handle()));
    }
#ifdef ACE_HAS_IPV6
    if (reactor()->register_handler(ipv6_unicast_socket_.get_handle(), &ice_endpoint_,
                                    ACE_Event_Handler::READ_MASK) != 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("RtpsUdpTransport::start_ice: ")
                 ACE_TEXT("failed to register handler for ipv6 unicast ")
                 ACE_TEXT("socket %d\n"),
                 ipv6_unicast_socket_.get_handle()));
    }
#endif
  }
}

void
RtpsUdpTransport::stop_ice()
{
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, "(%P|%t) RtpsUdpTransport::stop_ice\n"));
  }

  if (!link_) {
    if (reactor()->remove_handler(unicast_socket_.get_handle(), ACE_Event_Handler::READ_MASK) != 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("RtpsUdpTransport::stop_ice: ")
                 ACE_TEXT("failed to unregister handler for unicast ")
                 ACE_TEXT("socket %d\n"),
                 unicast_socket_.get_handle()));
    }
#ifdef ACE_HAS_IPV6
    if (reactor()->remove_handler(ipv6_unicast_socket_.get_handle(), ACE_Event_Handler::READ_MASK) != 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("RtpsUdpTransport::stop_ice: ")
                 ACE_TEXT("failed to unregister handler for ipv6 unicast ")
                 ACE_TEXT("socket %d\n"),
                 ipv6_unicast_socket_.get_handle()));
    }
#endif
  }

  ICE::Agent::instance()->remove_endpoint(&ice_endpoint_);
}

void
RtpsUdpTransport::relay_stun_task(const DCPS::MonotonicTimePoint& /*now*/)
{
  ACE_GUARD(ACE_Thread_Mutex, g, relay_stun_mutex_);

  if (!(config().use_rtps_relay() ||
        config().rtps_relay_only())) {
    return;
  }

  const ACE_INET_Addr stun_server_address = config().rtps_relay_address();

  process_relay_sra(relay_srsm_.send(stun_server_address, ICE::Configuration::instance()->server_reflexive_indication_count(), local_prefix_));

  if (!equal_guid_prefixes(local_prefix_, GUIDPREFIX_UNKNOWN) && stun_server_address != ACE_INET_Addr()) {
    ice_endpoint_.send(stun_server_address, relay_srsm_.message());
  }
}

void
RtpsUdpTransport::process_relay_sra(ICE::ServerReflexiveStateMachine::StateChange sc)
{
#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::ConnectionRecord connection_record;
  std::memset(connection_record.guid, 0, sizeof(connection_record.guid));
  connection_record.protocol = RTPS_RELAY_STUN_PROTOCOL;

  switch (sc) {
  case ICE::ServerReflexiveStateMachine::SRSM_None:
    break;
  case ICE::ServerReflexiveStateMachine::SRSM_Set:
  case ICE::ServerReflexiveStateMachine::SRSM_Change:
    connection_record.address = DCPS::LogAddr(relay_srsm_.stun_server_address()).c_str();
    deferred_connection_records_.push_back(std::make_pair(true, connection_record));
    break;
  case ICE::ServerReflexiveStateMachine::SRSM_Unset:
    {
      connection_record.address = DCPS::LogAddr(relay_srsm_.unset_stun_server_address()).c_str();
      deferred_connection_records_.push_back(std::make_pair(false, connection_record));
      break;
    }
  }

  if (!bit_sub_) {
    return;
  }

  job_queue_->enqueue(DCPS::make_rch<WriteConnectionRecords>(bit_sub_, deferred_connection_records_));
  deferred_connection_records_.clear();

#else
  ACE_UNUSED_ARG(sc);
#endif
}

void
RtpsUdpTransport::disable_relay_stun_task()
{
#ifndef DDS_HAS_MINIMUM_BIT
  relay_stun_task_->disable();

  DCPS::ConnectionRecord connection_record;
  std::memset(connection_record.guid, 0, sizeof(connection_record.guid));
  connection_record.protocol = RTPS_RELAY_STUN_PROTOCOL;

  if (relay_srsm_.stun_server_address() != ACE_INET_Addr()) {
    connection_record.address = DCPS::LogAddr(relay_srsm_.stun_server_address()).c_str();
    deferred_connection_records_.push_back(std::make_pair(false, connection_record));
  }

  if (!bit_sub_) {
    return;
  }

  job_queue_->enqueue(DCPS::make_rch<WriteConnectionRecords>(bit_sub_, deferred_connection_records_));
  deferred_connection_records_.clear();

  relay_srsm_ = ICE::ServerReflexiveStateMachine();
#endif
}

#endif

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
