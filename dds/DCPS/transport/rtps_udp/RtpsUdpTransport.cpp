/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpTransport.h"

#include "RtpsUdpDataLink.h"
#include "RtpsUdpInst.h"
#include "RtpsUdpInst_rch.h"
#include "RtpsUdpSendStrategy.h"
#include "RtpsUdpReceiveStrategy.h"

#include <dds/OpenDDSConfigWrapper.h>
#include <dds/OpenddsDcpsExtTypeSupportImpl.h>

#include <dds/DCPS/AssociationData.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/NetworkResource.h>

#include <dds/DCPS/transport/framework/TransportClient.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>

#include <dds/DCPS/RTPS/MessageUtils.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>

#include <ace/CDR_Base.h>
#include <ace/Log_Msg.h>
#include <ace/Sock_Connect.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RtpsUdpCore::RtpsUdpCore(const RtpsUdpInst_rch& inst)
  : send_delay_(inst->send_delay())
  , heartbeat_period_(inst->heartbeat_period())
  , nak_response_delay_(inst->nak_response_delay())
  , receive_address_duration_(inst->receive_address_duration())
  , rtps_relay_only_(inst->rtps_relay_only())
  , use_rtps_relay_(inst->use_rtps_relay())
  , rtps_relay_address_(inst->rtps_relay_address())
  , use_ice_(inst->use_ice())
  , stun_server_address_(inst->stun_server_address())
  , transport_statistics_(inst->name())
  , relay_stun_task_falloff_(TimeDuration::zero_value)
{}

RtpsUdpTransport::RtpsUdpTransport(const RtpsUdpInst_rch& inst,
                                   DDS::DomainId_t domain)
  : TransportImpl(inst, domain)
#if OPENDDS_CONFIG_SECURITY
  , local_crypto_handle_(DDS::HANDLE_NIL)
#endif
#if OPENDDS_CONFIG_SECURITY
  , ice_endpoint_(make_rch<IceEndpoint>(ref(*this)))
  , ice_agent_(ICE::Agent::instance())
#endif
  , core_(inst)
{
  assign(local_prefix_, GUIDPREFIX_UNKNOWN);
  if (!(configure_i(inst) && open())) {
    throw Transport::UnableToCreate();
  }
}

RtpsUdpInst_rch
RtpsUdpTransport::config() const
{
  return dynamic_rchandle_cast<RtpsUdpInst>(TransportImpl::config());
}

#if OPENDDS_CONFIG_SECURITY
DCPS::RcHandle<ICE::Agent>
RtpsUdpTransport::get_ice_agent() const
{
  return ice_agent_;
}
#endif

DCPS::WeakRcHandle<ICE::Endpoint>
RtpsUdpTransport::get_ice_endpoint()
{
#if OPENDDS_CONFIG_SECURITY
  return core_.use_ice() ? static_rchandle_cast<ICE::Endpoint>(ice_endpoint_) : DCPS::WeakRcHandle<ICE::Endpoint>();
#else
  return DCPS::WeakRcHandle<ICE::Endpoint>();
#endif
}

RtpsUdpDataLink_rch
RtpsUdpTransport::make_datalink(const GuidPrefix_t& local_prefix)
{
  OPENDDS_ASSERT(!equal_guid_prefixes(local_prefix, GUIDPREFIX_UNKNOWN));

  RtpsUdpInst_rch cfg = config();
  if (!cfg) {
    return RtpsUdpDataLink_rch();
  }

  if (equal_guid_prefixes(local_prefix_, GUIDPREFIX_UNKNOWN)) {
    assign(local_prefix_, local_prefix);
#if OPENDDS_CONFIG_SECURITY
    core_.reset_relay_stun_task_falloff();
    relay_stun_task_->schedule(TimeDuration::zero_value);
#endif
  }

#if OPENDDS_CONFIG_SECURITY
  {
    if (core_.use_ice()) {
      ReactorTask_rch ri = reactor_task();
      ri->execute_or_enqueue(make_rch<RemoveHandler>(unicast_socket_.get_handle(), static_cast<ACE_Reactor_Mask>(ACE_Event_Handler::READ_MASK)));
#ifdef ACE_HAS_IPV6
      ri->execute_or_enqueue(make_rch<RemoveHandler>(ipv6_unicast_socket_.get_handle(), static_cast<ACE_Reactor_Mask>(ACE_Event_Handler::READ_MASK)));
#endif
    }
  }
#endif

  RtpsUdpDataLink_rch link = make_rch<RtpsUdpDataLink>(rchandle_from(this), local_prefix, config(), reactor_task());

#if OPENDDS_CONFIG_SECURITY
  link->local_crypto_handle(local_crypto_handle_);
#endif

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
  bit_sub_ = client->get_builtin_subscriber_proxy();

  GuardThreadType guard_links(links_lock_);

  if (is_shut_down()) {
    return AcceptConnectResult();
  }

  if (!link_) {
    link_ = make_datalink(attribs.local_id_.guidPrefix);
    if (!link_) {
      return AcceptConnectResult();
    }
  }

  RtpsUdpDataLink_rch link = link_;

  if (use_datalink(attribs.local_id_, remote.repo_id_, remote.blob_, remote.discovery_blob_, remote.participant_discovered_at_,
                   remote.context_,
                   attribs.local_reliable_, remote.reliable_,
                   attribs.local_durable_, remote.durable_, attribs.max_sn_, client)) {
    return AcceptConnectResult(link);
  }

  add_pending_connection(client, link);
  VDBG_LVL((LM_DEBUG, "(%P|%t) RtpsUdpTransport::connect_datalink pending.\n"), 2);
  return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);
}

TransportImpl::AcceptConnectResult
RtpsUdpTransport::accept_datalink(const RemoteTransport& remote,
                                  const ConnectionAttribs& attribs,
                                  const TransportClient_rch& client)
{
  bit_sub_ = client->get_builtin_subscriber_proxy();

  GuardThreadType guard_links(links_lock_);

  if (is_shut_down()) {
    return AcceptConnectResult();
  }

  if (!link_) {
    link_ = make_datalink(attribs.local_id_.guidPrefix);
    if (!link_) {
      return AcceptConnectResult();
    }
  }
  RtpsUdpDataLink_rch link = link_;

  if (use_datalink(attribs.local_id_, remote.repo_id_, remote.blob_, remote.discovery_blob_, remote.participant_discovered_at_,
                   remote.context_,
                   attribs.local_reliable_, remote.reliable_,
                   attribs.local_durable_, remote.durable_, attribs.max_sn_, client)) {
    return AcceptConnectResult(link);
  }

  add_pending_connection(client, link);
  VDBG_LVL((LM_DEBUG, "(%P|%t) RtpsUdpTransport::accept_datalink pending.\n"), 2);
  return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);
}


void
RtpsUdpTransport::stop_accepting_or_connecting(const TransportClient_wrch& client,
                                               const GUID_t& remote_id,
                                               bool disassociate,
                                               bool association_failed)
{
  if (disassociate || association_failed) {
    GuardThreadType guard_links(links_lock_);
    if (link_) {
      TransportClient_rch c = client.lock();
      if (c) {
        link_->release_reservations(remote_id, c->get_guid());
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
RtpsUdpTransport::use_datalink(const GUID_t& local_id,
                               const GUID_t& remote_id,
                               const TransportBLOB& remote_data,
                               const TransportBLOB& discovery_locator,
                               const MonotonicTime_t& participant_discovered_at,
                               ACE_CDR::ULong participant_flags,
                               bool local_reliable, bool remote_reliable,
                               bool local_durable, bool remote_durable,
                               SequenceNumber max_sn,
                               const TransportClient_rch& client)
{
  NetworkAddressSet uc_addrs, mc_addrs;
  bool requires_inline_qos;
  RTPS::VendorId_t vendor_id = {{ 0, 0 }};
  unsigned int blob_bytes_read;
  get_connection_addrs(remote_data, &uc_addrs, &mc_addrs, &requires_inline_qos, &vendor_id, &blob_bytes_read);

  NetworkAddress disco_addr_hint;
  if (discovery_locator.length()) {
    NetworkAddressSet disco_uc_addrs, disco_mc_addrs;
    bool disco_requires_inline_qos;
    unsigned int disco_blob_bytes_read;
    get_connection_addrs(discovery_locator, &disco_uc_addrs, &disco_mc_addrs, &disco_requires_inline_qos, 0, &disco_blob_bytes_read);

    for (NetworkAddressSet::const_iterator it = disco_uc_addrs.begin(), limit = disco_uc_addrs.end(); it != limit; ++it) {
      for (NetworkAddressSet::const_iterator it2 = uc_addrs.begin(), limit2 = uc_addrs.end(); it2 != limit2; ++it2) {
        if (it->addr_bytes_equal(*it2) && DCPS::is_more_local(disco_addr_hint, *it2)) {
          disco_addr_hint = *it2;
        }
      }
    }
  }

  if (link_) {
    return link_->associated(local_id, remote_id, local_reliable, remote_reliable,
                             local_durable, remote_durable,
                             vendor_id,
                             participant_discovered_at, participant_flags, max_sn, client,
                             uc_addrs, mc_addrs, disco_addr_hint, requires_inline_qos);
  }

  return true;
}

#if OPENDDS_CONFIG_SECURITY
void
RtpsUdpTransport::local_crypto_handle(DDS::Security::ParticipantCryptoHandle pch)
{
  RtpsUdpDataLink_rch link;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(links_lock_);
    local_crypto_handle_ = pch;
    link = link_;
  }
  if (link) {
    link->local_crypto_handle(pch);
  }
}
#endif

void
RtpsUdpTransport::get_connection_addrs(const TransportBLOB& remote,
                                       NetworkAddressSet* uc_addrs,
                                       NetworkAddressSet* mc_addrs,
                                       bool* requires_inline_qos,
                                       RTPS::VendorId_t* vendor_id,
                                       unsigned int* blob_bytes_read) const
{
  using namespace OpenDDS::RTPS;
  LocatorSeq locators;
  VendorId_t vid;
  DDS::ReturnCode_t result =
    blob_to_locators(remote, locators, vid, requires_inline_qos, blob_bytes_read);
  if (result != DDS::RETCODE_OK) {
    return;
  }

  if (vendor_id) {
    *vendor_id = vid;
  }

  for (CORBA::ULong i = 0; i < locators.length(); ++i) {
    ACE_INET_Addr addr;
    // If conversion was successful
    if (locator_to_address(addr, locators[i], false) == 0) {
      if (addr.is_multicast()) {
        RtpsUdpInst_rch cfg = config();
        if (cfg && cfg->use_multicast() && mc_addrs) {
          mc_addrs->insert(NetworkAddress(addr));
        }
      } else if (uc_addrs) {
        uc_addrs->insert(NetworkAddress(addr));
      }
    }
  }
}

bool
RtpsUdpTransport::connection_info_i(TransportLocator& info, ConnectionInfoFlags flags) const
{
  RtpsUdpInst_rch cfg = config();
  if (cfg) {
    cfg->populate_locator(info, flags, domain_);
    return true;
  }
  return false;
}

void
RtpsUdpTransport::register_for_reader(const GUID_t& participant,
                                      const GUID_t& writerid,
                                      const GUID_t& readerid,
                                      const TransportLocatorSeq& locators,
                                      OpenDDS::DCPS::DiscoveryListener* listener)
{
  if (is_shut_down()) {
    return;
  }

  RtpsUdpInst_rch cfg = config();
  if (!cfg) {
    return;
  }

  const TransportBLOB* blob = cfg->get_blob(locators);
  if (!blob) {
    return;
  }

  GuardThreadType guard_links(links_lock_);

  if (!link_) {
    link_ = make_datalink(participant.guidPrefix);
  }

  NetworkAddressSet uc_addrs;
  get_connection_addrs(*blob, &uc_addrs);
  link_->register_for_reader(writerid, readerid, uc_addrs, listener);
}

void
RtpsUdpTransport::unregister_for_reader(const GUID_t& /*participant*/,
                                        const GUID_t& writerid,
                                        const GUID_t& readerid)
{
  GuardThreadType guard_links(links_lock_);

  if (link_) {
    link_->unregister_for_reader(writerid, readerid);
  }
}

void
RtpsUdpTransport::register_for_writer(const GUID_t& participant,
                                      const GUID_t& readerid,
                                      const GUID_t& writerid,
                                      const TransportLocatorSeq& locators,
                                      DiscoveryListener* listener)
{
  if (is_shut_down()) {
    return;
  }

  RtpsUdpInst_rch cfg = config();
  if (!cfg) {
    return;
  }

  const TransportBLOB* blob = cfg->get_blob(locators);
  if (!blob) {
    return;
  }

  GuardThreadType guard_links(links_lock_);

  if (!link_) {
    link_ = make_datalink(participant.guidPrefix);
  }

  NetworkAddressSet uc_addrs;
  get_connection_addrs(*blob, &uc_addrs);
  link_->register_for_writer(readerid, writerid, uc_addrs, listener);
}

void
RtpsUdpTransport::unregister_for_writer(const GUID_t& /*participant*/,
                                        const GUID_t& readerid,
                                        const GUID_t& writerid)
{
  GuardThreadType guard_links(links_lock_);

  if (link_) {
    link_->unregister_for_writer(readerid, writerid);
  }
}

void
RtpsUdpTransport::update_locators(const GUID_t& remote,
                                  const TransportLocatorSeq& locators)
{
  if (is_shut_down()) {
    return;
  }

  RtpsUdpInst_rch cfg = config();
  if (!cfg) {
    return;
  }

  const TransportBLOB* blob = cfg->get_blob(locators);
  if (!blob) {
    return;
  }

  GuardThreadType guard_links(links_lock_);

  if (link_) {
    NetworkAddressSet uc_addrs, mc_addrs;
    bool requires_inline_qos;
    unsigned int blob_bytes_read;
    get_connection_addrs(*blob, &uc_addrs, &mc_addrs, &requires_inline_qos, 0, &blob_bytes_read);
    link_->update_locators(remote, uc_addrs, mc_addrs, requires_inline_qos, false);
  }
}

void
RtpsUdpTransport::get_last_recv_locator(const GUID_t& remote,
                                        const GuidVendorId_t& vendor_id,
                                        TransportLocator& tl)
{
  if (is_shut_down()) {
    return;
  }

  GuardThreadType guard_links(links_lock_);

  bool expects_inline_qos = false;
  NetworkAddress addr;
  if (link_) {
    addr = link_->get_last_recv_address(remote);
    if (addr == NetworkAddress()) {
      return;
    }
    GUIDSeq_var guids(new GUIDSeq);
    GUIDSeq& ref = static_cast<GUIDSeq&>(guids);
    ref.length(1);
    ref[0] = remote;
    expects_inline_qos = link_->requires_inline_qos(guids);
  }

  LocatorSeq locators;
  locators.length(1);
  address_to_locator(locators[0], addr.to_addr());

  RTPS::VendorId_t vid;
  vid.vendorId[0] = vendor_id[0];
  vid.vendorId[1] = vendor_id[1];

  const Encoding& encoding = RTPS::get_locators_encoding();
  size_t size = serialized_size(encoding, locators);
  serialized_size(encoding, size, vid);
  primitive_serialized_size_boolean(encoding, size);

  ACE_Message_Block mb_locator(size);
  Serializer ser_loc(&mb_locator, encoding);
  ser_loc << locators;
  ser_loc << vid;
  ser_loc << ACE_OutputCDR::from_boolean(expects_inline_qos);

  tl.transport_type = "rtps_udp";
  RTPS::message_block_to_sequence(mb_locator, tl.data);
}

void
RtpsUdpTransport::append_transport_statistics(TransportStatisticsSequence& seq)
{
  core_.append_transport_statistics(seq);
}

bool RtpsUdpTransport::open_socket(
  const RtpsUdpInst_rch& config, ACE_SOCK_Dgram& sock, int protocol, ACE_INET_Addr& actual)
{
  const bool ipv4 = protocol == PF_INET;
  const DDS::UInt16 init_part_port_id =
#ifdef ACE_HAS_IPV6
    ipv4 ? config->init_participant_port_id() : config->ipv6_init_participant_port_id();
#else
    config->init_participant_port_id();
#endif

  NetworkAddress address;
  bool fixed_port;
  DDS::UInt16 part_port_id = init_part_port_id;
  while (true) {
#ifdef ACE_HAS_IPV6
    if (ipv4) {
#endif
      if (!config->unicast_address(address, fixed_port, domain_, part_port_id)) {
        return false;
      }
#ifdef ACE_HAS_IPV6
    } else if (!config->ipv6_unicast_address(address, fixed_port, domain_, part_port_id)) {
      return false;
    }
#endif

    if (sock.open(address.to_addr(), protocol) == 0) {
      break;
    }

    if (fixed_port) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RtpsUdpTransport::open_socket: "
          "failed to open unicast %C socket for %C: %m\n",
          ipv4 ? "IPv4" : "IPv6", LogAddr(address).c_str()));
      }
      return false;

    } else {
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpTransport::open_socket: "
          "failed to open unicast %C socket for %C: %m, trying next participant ID...\n",
          ipv4 ? "IPv4" : "IPv6", LogAddr(address).c_str()));
      }

      ++part_port_id;

      if (part_port_id == init_part_port_id) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RtpsUdpTransport::open_socket: "
            "could not find a free %C unicast port\n",
            ipv4 ? "IPv4" : "IPv6"));
        }
        return false;
      }
    }
  }

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) RtpsUdpTransport::open_socket: "
               "opened %C unicast socket %d for %C\n",
               ipv4 ? "IPv4" : "IPv6", sock.get_handle(), LogAddr(address).c_str()));
  }

#ifdef ACE_WIN32
  // By default Winsock will cause reads to fail with "connection reset"
  // when UDP sends result in ICMP "port unreachable" messages.
  // The transport framework is not set up for this since returning <= 0
  // from our receive_bytes causes the framework to close down the datalink
  // which in this case is used to receive from multiple peers.
  {
    BOOL recv_udp_connreset = FALSE;
    sock.control(SIO_UDP_CONNRESET, &recv_udp_connreset);
  }
#endif

  if (sock.get_local_addr(actual) != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RtpsUdpTransport::open_socket: "
        "failed to get actual address from %C socket for %C: %m\n",
        ipv4 ? "IPv4" : "IPv6", LogAddr(address).c_str()));
    }
    return false;
  }

  if (!set_recvpktinfo(sock, ipv4)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RtpsUdpTransport::open_socket: "
        "failed to set RECVPKTINFO on %C socket for %C\n",
        ipv4 ? "IPv4" : "IPv6", LogAddr(address).c_str()));
    }
    return false;
  }

  return true;
}

bool
RtpsUdpTransport::configure_i(const RtpsUdpInst_rch& config)
{
  if (!config) {
    return false;
  }

  // Open the socket here so that any addresses/ports left
  // unspecified in the RtpsUdpInst are known by the time we get to
  // connection_info_i().  Opening the sockets here also allows us to
  // detect and report errors during DataReader/Writer setup instead
  // of during association.

  ACE_INET_Addr actual4;
  if (!open_socket(config, unicast_socket_, PF_INET, actual4)) {
    return false;
  }
  config->actual_local_address_ = actual4;

#ifdef ACE_HAS_IPV6
  ACE_INET_Addr actual6;
  if (!open_socket(config, ipv6_unicast_socket_, PF_INET6, actual6)) {
    return false;
  }
  NetworkAddress temp(actual6);
  if (actual6.is_ipv4_mapped_ipv6() && temp.is_any()) {
    temp = NetworkAddress(actual6.get_port_number(), "::");
  }
  config->ipv6_actual_local_address_ = temp;
#endif

  create_reactor_task(false, "RtpsUdpTransport" + config->name());

  ACE_Reactor* reactor = reactor_task()->get_reactor();
  job_queue_ = DCPS::make_rch<DCPS::JobQueue>(reactor);

#if OPENDDS_CONFIG_SECURITY
  if (core_.use_ice()) {
    start_ice();
  }

  relay_stun_task_= make_rch<Sporadic>(TheServiceParticipant->time_source(), reactor_task(), rchandle_from(this), &RtpsUdpTransport::relay_stun_task);
#endif

  if (config->opendds_discovery_default_listener_) {
    GuardThreadType guard_links(links_lock_);
    link_ = make_datalink(config->opendds_discovery_guid_.guidPrefix);
    link_->default_listener(*config->opendds_discovery_default_listener_);
  }

#if OPENDDS_CONFIG_SECURITY
  core_.reset_relay_stun_task_falloff();
  relay_stun_task_->schedule(TimeDuration::zero_value);
#endif

  // Start listening for config events after everything is initialized.
  ConfigListener::job_queue(job_queue_);
  config_reader_ = make_rch<ConfigReader>(ConfigStoreImpl::datareader_qos(), rchandle_from(this));
  TheServiceParticipant->config_topic()->connect(config_reader_);

  return true;
}

void RtpsUdpTransport::client_stop(const GUID_t& localId)
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
#if OPENDDS_CONFIG_SECURITY
  if (core_.use_ice()) {
    stop_ice();
  }

  relay_stun_task_->cancel();
#endif

  if (config_reader_) {
    TheServiceParticipant->config_topic()->disconnect(config_reader_);
    config_reader_.reset();
  }

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

void
RtpsUdpTransport::on_data_available(ConfigReader_rch)
{
  const RtpsUdpInst_rch cfg = config();
  OPENDDS_ASSERT(cfg);
  RcHandle<ConfigStoreImpl> config_store = TheServiceParticipant->config_store();
  const String& config_prefix = cfg->config_prefix();
  bool has_prefix = false;

  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->take(samples, infos, DDS::LENGTH_UNLIMITED,
                       DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];

    if (sample.key_has_prefix(config_prefix)) {
      has_prefix = true;

#if OPENDDS_CONFIG_SECURITY
      if (sample.key() == cfg->config_key("RTPS_RELAY_ONLY")) {
        core_.rtps_relay_only(cfg->rtps_relay_only());
        if (core_.rtps_relay_only()) {
          core_.reset_relay_stun_task_falloff();
          relay_stun_task_->schedule(TimeDuration::zero_value);
        } else {
          if (!core_.use_rtps_relay()) {
            disable_relay_stun_task();
          }
        }
      } else if (sample.key() == cfg->config_key("USE_RTPS_RELAY")) {
        core_.use_rtps_relay(cfg->use_rtps_relay());
        if (core_.use_rtps_relay()) {
          core_.reset_relay_stun_task_falloff();
          relay_stun_task_->schedule(TimeDuration::zero_value);
        } else {
          if (!core_.rtps_relay_only()) {
            disable_relay_stun_task();
          }
        }
      } else if (sample.key() == cfg->config_key("DATA_RTPS_RELAY_ADDRESS")) {
        core_.rtps_relay_address(cfg->rtps_relay_address());
        relay_stun_task_->cancel();
        core_.reset_relay_stun_task_falloff();
        relay_stun_task_->schedule(TimeDuration::zero_value);
      } else if (sample.key() == cfg->config_key("USE_ICE")) {
        const bool before = core_.use_ice();
        const bool after = cfg->use_ice();
        core_.use_ice(after);

        if (before && !after) {
          stop_ice();
        } else if (!before && after) {
          start_ice();
        }
      }
#endif
    }
  }

  if (has_prefix) {
    core_.reload(config_prefix);
  }
}

#if OPENDDS_CONFIG_SECURITY

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
  ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

  struct iovec iov[1];
  char buffer[0x10000];
  iov[0].iov_base = buffer;
  iov[0].iov_len = sizeof buffer;
  ACE_INET_Addr remote_address;

  bool stop;
  RtpsUdpReceiveStrategy::receive_bytes_helper(iov, 1, choose_recv_socket(fd), remote_address,
#if OPENDDS_CONFIG_SECURITY
                                               transport.get_ice_agent(), transport.get_ice_endpoint(),
#endif
                                               transport, stop);

  return 0;
}

namespace {
  bool shouldWarn(int code) {
    return code == EPERM || code == EACCES || code == EINTR || code == ENOBUFS || code == ENOMEM
      || code == EADDRNOTAVAIL || code == ENETUNREACH;
  }

  ssize_t
  send_single_i(const RtpsUdpTransport& transport,
                ACE_SOCK_Dgram& socket,
                const iovec iov[],
                int n,
                const ACE_INET_Addr& addr,
                bool& network_is_unreachable)
  {
    ACE_UNUSED_ARG(transport);

    OPENDDS_ASSERT(addr != ACE_INET_Addr());

#ifdef OPENDDS_TESTING_FEATURES
    ssize_t total_length;
    if (transport.core().should_drop(iov, n, total_length)) {
      return total_length;
    }
#endif

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
RtpsUdpTransport::IceEndpoint::host_addresses() const
{
  ICE::AddressListType addresses;

  RtpsUdpInst_rch cfg = transport.config();

  if (!cfg) {
    return addresses;
  }

  ACE_INET_Addr addr = cfg->actual_local_address_.to_addr();
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
  addr = cfg->ipv6_actual_local_address_.to_addr();
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
  const NetworkAddress da(destination);
  ACE_SOCK_Dgram& socket = choose_send_socket(destination);

  ACE_Message_Block block(20 + message.length());
  DCPS::Serializer serializer(&block, STUN::encoding);
  const_cast<STUN::Message&>(message).block = &block;
  serializer << message;

  iovec iov[MAX_SEND_BLOCKS];
  const int num_blocks = RtpsUdpSendStrategy::mb_to_iov(block, iov);
  const ssize_t result = send_single_i(transport, socket, iov, num_blocks, destination, network_is_unreachable_);

  if (result < 0) {
    transport.core().send_fail(da, MCK_STUN, iov, num_blocks);
    if (!network_is_unreachable_) {
      const ACE_Log_Priority prio = shouldWarn(errno) ? LM_WARNING : LM_ERROR;
      ACE_ERROR((prio, "(%P|%t) RtpsUdpTransport::send() - "
                 "failed to send STUN message\n"));
    }
  } else {
    transport.core().send(da, MCK_STUN, result);
  }
}

ACE_INET_Addr
RtpsUdpTransport::IceEndpoint::stun_server_address() const {
  return transport.core().stun_server_address().to_addr();
}

void
RtpsUdpTransport::start_ice()
{
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, "(%P|%t) RtpsUdpTransport::start_ice\n"));
  }

  ice_agent_->add_endpoint(static_rchandle_cast<ICE::Endpoint>(ice_endpoint_));

  GuardThreadType guard_links(links_lock_);

  if (!link_) {
    ReactorTask_rch ri = reactor_task();
    ri->execute_or_enqueue(make_rch<RegisterHandler>(unicast_socket_.get_handle(), ice_endpoint_.get(), static_cast<ACE_Reactor_Mask>(ACE_Event_Handler::READ_MASK)));
#ifdef ACE_HAS_IPV6
    ri->execute_or_enqueue(make_rch<RegisterHandler>(ipv6_unicast_socket_.get_handle(), ice_endpoint_.get(), static_cast<ACE_Reactor_Mask>(ACE_Event_Handler::READ_MASK)));
#endif
  }
}

void
RtpsUdpTransport::stop_ice()
{
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, "(%P|%t) RtpsUdpTransport::stop_ice\n"));
  }

  GuardThreadType guard_links(links_lock_);

  if (!link_) {
    ReactorTask_rch ri = reactor_task();
    ri->execute_or_enqueue(make_rch<RemoveHandler>(unicast_socket_.get_handle(), static_cast<ACE_Reactor_Mask>(ACE_Event_Handler::READ_MASK)));
#ifdef ACE_HAS_IPV6
    ri->execute_or_enqueue(make_rch<RemoveHandler>(ipv6_unicast_socket_.get_handle(), static_cast<ACE_Reactor_Mask>(ACE_Event_Handler::READ_MASK)));
#endif
  }

  ice_agent_->remove_endpoint(static_rchandle_cast<ICE::Endpoint>(ice_endpoint_));
}

void
RtpsUdpTransport::relay_stun_task(const DCPS::MonotonicTimePoint& /*now*/)
{
  GuardThreadType guard_links(links_lock_);

  const ACE_INET_Addr relay_address = core_.rtps_relay_address().to_addr();

  if ((core_.use_rtps_relay() || core_.rtps_relay_only()) &&
      relay_address != ACE_INET_Addr() &&
      !equal_guid_prefixes(local_prefix_, GUIDPREFIX_UNKNOWN)) {
    process_relay_sra(relay_srsm_.send(relay_address, ICE::Configuration::instance()->server_reflexive_indication_count(), local_prefix_));
    ice_endpoint_->send(relay_address, relay_srsm_.message());
    relay_stun_task_->schedule(core_.advance_relay_stun_task_falloff());
  }
}

void
RtpsUdpTransport::process_relay_sra(ICE::ServerReflexiveStateMachine::StateChange sc)
{
#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::ConnectionRecord connection_record;
  std::memset(connection_record.guid, 0, sizeof(connection_record.guid));
  connection_record.protocol = RTPS_RELAY_STUN_PROTOCOL;
  connection_record.latency = TimeDuration::zero_value.to_dds_duration();

  switch (sc) {
  case ICE::ServerReflexiveStateMachine::SRSM_None:
    if (relay_srsm_.latency_available()) {
      connection_record.address = DCPS::LogAddr(relay_srsm_.stun_server_address()).c_str();
      connection_record.latency = relay_srsm_.latency().to_dds_duration();
      relay_srsm_.latency_available(false);
      deferred_connection_records_.push_back(std::make_pair(true, connection_record));
    }
    break;
  case ICE::ServerReflexiveStateMachine::SRSM_Set:
  case ICE::ServerReflexiveStateMachine::SRSM_Change:
    // Lengthen to normal period.
    core_.set_relay_stun_task_falloff();
    connection_record.address = DCPS::LogAddr(relay_srsm_.stun_server_address()).c_str();
    connection_record.latency = relay_srsm_.latency().to_dds_duration();
    relay_srsm_.latency_available(false);
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

  if (!deferred_connection_records_.empty()) {
    job_queue_->enqueue(DCPS::make_rch<WriteConnectionRecords>(bit_sub_, deferred_connection_records_));
    deferred_connection_records_.clear();
  }

#else
  ACE_UNUSED_ARG(sc);
#endif
}

void
RtpsUdpTransport::disable_relay_stun_task()
{
#ifndef DDS_HAS_MINIMUM_BIT
  relay_stun_task_->cancel();

  DCPS::ConnectionRecord connection_record;
  std::memset(connection_record.guid, 0, sizeof(connection_record.guid));
  connection_record.protocol = RTPS_RELAY_STUN_PROTOCOL;
  connection_record.latency = TimeDuration::zero_value.to_dds_duration();

  if (relay_srsm_.stun_server_address() != ACE_INET_Addr()) {
    connection_record.address = DCPS::LogAddr(relay_srsm_.stun_server_address()).c_str();
    deferred_connection_records_.push_back(std::make_pair(false, connection_record));
  }

  if (!bit_sub_) {
    return;
  }

  if (!deferred_connection_records_.empty()) {
    job_queue_->enqueue(DCPS::make_rch<WriteConnectionRecords>(bit_sub_, deferred_connection_records_));
    deferred_connection_records_.clear();
  }

  relay_srsm_ = ICE::ServerReflexiveStateMachine();
#endif
}

#endif

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
