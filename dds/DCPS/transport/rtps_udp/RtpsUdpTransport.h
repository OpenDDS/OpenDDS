/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPTRANSPORT_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPTRANSPORT_H

#include "Rtps_Udp_Export.h"
#include "RtpsUdpDataLink_rch.h"
#include "RtpsUdpDataLink.h"

#include <dds/DCPS/ConnectionRecords.h>
#include <dds/DCPS/FibonacciSequence.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/SporadicTask.h>

#include <dds/DCPS/RTPS/ICE/Ice.h>
#include <dds/DCPS/RTPS/RtpsCoreC.h>

#include <dds/DCPS/transport/framework/TransportClient.h>
#include <dds/DCPS/transport/framework/TransportImpl.h>
#include <dds/DCPS/transport/framework/TransportStatistics.h>
#include <dds/DCPS/transport/framework/MessageDropper.h>

#include <dds/OpenDDSConfigWrapper.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class RtpsUdpInst;

class OpenDDS_Rtps_Udp_Export RtpsUdpCore {
public:
  RtpsUdpCore(const RtpsUdpInst_rch& inst);

  TimeDuration send_delay() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return send_delay_;
  }

  TimeDuration heartbeat_period() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return heartbeat_period_;
  }

  TimeDuration nak_response_delay() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return nak_response_delay_;
  }

  TimeDuration receive_address_duration() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return receive_address_duration_;
  }

  void rtps_relay_only(bool flag)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    rtps_relay_only_ = flag;
  }

  bool rtps_relay_only() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return rtps_relay_only_;
  }

  void use_rtps_relay(bool flag)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    use_rtps_relay_ = flag;
  }

  bool use_rtps_relay() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return use_rtps_relay_;
  }

  void rtps_relay_address(const NetworkAddress& address)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    rtps_relay_address_ = address;
  }

  NetworkAddress rtps_relay_address() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return rtps_relay_address_;
  }

  void use_ice(bool flag)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    use_ice_ = flag;
  }

  bool use_ice() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return use_ice_;
  }

  void stun_server_address(const NetworkAddress& address)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    stun_server_address_ = address;
  }

  NetworkAddress stun_server_address() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return stun_server_address_;
  }

  void send(const NetworkAddress& remote_address,
            CORBA::Long key_kind,
            ssize_t bytes)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (transport_statistics_.count_messages()) {
      const InternalMessageCountKey key(remote_address, key_kind, remote_address == rtps_relay_address_);
      transport_statistics_.message_count[key].send(bytes);
    }
  }

  void send_fail(const NetworkAddress& remote_address,
                 CORBA::Long key_kind,
                 ssize_t bytes)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (transport_statistics_.count_messages()) {
      const InternalMessageCountKey key(remote_address, key_kind, remote_address == rtps_relay_address_);
      transport_statistics_.message_count[key].send_fail(bytes);
    }
  }

  void send_fail(const NetworkAddress& remote_address,
                 CORBA::Long key_kind,
                 iovec iov[],
                 int num_blocks)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (transport_statistics_.count_messages()) {
      ssize_t bytes = 0;
      for (int i = 0; i < num_blocks; ++i) {
        bytes += iov[i].iov_len;
      }
      const InternalMessageCountKey key(remote_address, key_kind, remote_address == rtps_relay_address_);
      transport_statistics_.message_count[key].send_fail(bytes);
    }
  }

  void recv(const NetworkAddress& remote_address,
            CORBA::Long key_kind,
            ssize_t bytes)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (transport_statistics_.count_messages()) {
      const InternalMessageCountKey key(remote_address, key_kind, remote_address == rtps_relay_address_);
      transport_statistics_.message_count[key].recv(bytes);
    }
  }

  void reader_nack_count(const GUID_t& guid,
                         ACE_CDR::ULong count)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (transport_statistics_.count_messages()) {
      transport_statistics_.reader_nack_count[guid] += count;
    }
  }

  void writer_resend_count(const GUID_t& guid,
                           ACE_CDR::ULong count)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (transport_statistics_.count_messages()) {
      transport_statistics_.writer_resend_count[guid] += count;
    }
  }

  void append_transport_statistics(TransportStatisticsSequence& seq)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    append(seq, transport_statistics_);
    transport_statistics_.clear();
  }

  bool should_drop(ssize_t length) const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return message_dropper_.should_drop(length);
  }

  bool should_drop(const iovec iov[],
                   int n,
                   ssize_t& length) const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return message_dropper_.should_drop(iov, n, length);
  }

  void reload(const String& config_prefix)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    message_dropper_.reload(TheServiceParticipant->config_store(), config_prefix);
    transport_statistics_.reload(TheServiceParticipant->config_store(), config_prefix);
  }

#if OPENDDS_CONFIG_SECURITY
  void reset_relay_stun_task_falloff()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    relay_stun_task_falloff_.set(heartbeat_period_);
  }

  TimeDuration advance_relay_stun_task_falloff()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    relay_stun_task_falloff_.advance(ICE::Configuration::instance()->server_reflexive_address_period());
    return relay_stun_task_falloff_.get();
  }

  void set_relay_stun_task_falloff()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    relay_stun_task_falloff_.set(ICE::Configuration::instance()->server_reflexive_address_period());
  }
#endif

private:
  mutable ACE_Thread_Mutex mutex_;
  const TimeDuration send_delay_;
  const TimeDuration heartbeat_period_;
  const TimeDuration nak_response_delay_;
  const TimeDuration receive_address_duration_;
  bool rtps_relay_only_;
  bool use_rtps_relay_;
  NetworkAddress rtps_relay_address_;
  bool use_ice_;
  NetworkAddress stun_server_address_;
  MessageDropper message_dropper_;
  InternalTransportStatistics transport_statistics_;
  FibonacciSequence<TimeDuration> relay_stun_task_falloff_;
};

class OpenDDS_Rtps_Udp_Export RtpsUdpTransport : public TransportImpl, public ConfigListener {
public:
  RtpsUdpTransport(const RtpsUdpInst_rch& inst,
                   DDS::DomainId_t domain);
  RtpsUdpInst_rch config() const;
#if OPENDDS_CONFIG_SECURITY
  DCPS::RcHandle<ICE::Agent> get_ice_agent() const;
#endif
  virtual DCPS::WeakRcHandle<ICE::Endpoint> get_ice_endpoint();
#if OPENDDS_CONFIG_SECURITY
  ICE::ServerReflexiveStateMachine& relay_srsm() { return relay_srsm_; }
  void process_relay_sra(ICE::ServerReflexiveStateMachine::StateChange);
  void disable_relay_stun_task();
#endif

  virtual void update_locators(const GUID_t& /*remote*/,
                               const TransportLocatorSeq& /*locators*/);

  virtual void get_last_recv_locator(const GUID_t& /*remote_id*/,
                                     const GuidVendorId_t& /*vendor_id*/,
                                     TransportLocator& /*locators*/);

  void append_transport_statistics(TransportStatisticsSequence& seq);

  RtpsUdpCore& core() { return core_; }
  const RtpsUdpCore& core() const { return core_; }

private:
  virtual AcceptConnectResult connect_datalink(const RemoteTransport& remote,
                                               const ConnectionAttribs& attribs,
                                               const TransportClient_rch& client);

  virtual AcceptConnectResult accept_datalink(const RemoteTransport& remote,
                                              const ConnectionAttribs& attribs,
                                              const TransportClient_rch& client);

  virtual void stop_accepting_or_connecting(const TransportClient_wrch& client,
                                            const GUID_t& remote_id,
                                            bool disassociate,
                                            bool association_failed);

  bool open_socket(
    const RtpsUdpInst_rch& config, ACE_SOCK_Dgram& sock, int protocol, ACE_INET_Addr& actual);

  bool configure_i(const RtpsUdpInst_rch& config);

  void client_stop(const GUID_t& localId);

  virtual void shutdown_i();

  virtual void register_for_reader(const GUID_t& participant,
                                   const GUID_t& writerid,
                                   const GUID_t& readerid,
                                   const TransportLocatorSeq& locators,
                                   DiscoveryListener* listener);

  virtual void unregister_for_reader(const GUID_t& participant,
                                     const GUID_t& writerid,
                                     const GUID_t& readerid);

  virtual void register_for_writer(const GUID_t& /*participant*/,
                                   const GUID_t& /*readerid*/,
                                   const GUID_t& /*writerid*/,
                                   const TransportLocatorSeq& /*locators*/,
                                   DiscoveryListener* /*listener*/);

  virtual void unregister_for_writer(const GUID_t& /*participant*/,
                                     const GUID_t& /*readerid*/,
                                     const GUID_t& /*writerid*/);

  virtual bool connection_info_i(TransportLocator& info, ConnectionInfoFlags flags) const;

  void get_connection_addrs(const TransportBLOB& data,
                            NetworkAddressSet* uc_addrs,
                            NetworkAddressSet* mc_addrs = 0,
                            bool* requires_inline_qos = 0,
                            RTPS::VendorId_t* vendor_id = 0,
                            unsigned int* blob_bytes_read = 0) const;

  virtual void release_datalink(DataLink* link);

  virtual OPENDDS_STRING transport_type() const { return "rtps_udp"; }

  RtpsUdpDataLink_rch make_datalink(const GuidPrefix_t& local_prefix);

  bool use_datalink(const GUID_t& local_id,
                    const GUID_t& remote_id,
                    const TransportBLOB& remote_data,
                    const TransportBLOB& discovery_locator,
                    const MonotonicTime_t& participant_discovered_at,
                    ACE_CDR::ULong participant_flags,
                    bool local_reliable, bool remote_reliable,
                    bool local_durable, bool remote_durable,
                    SequenceNumber max_sn,
                    const TransportClient_rch& client);

#if OPENDDS_CONFIG_SECURITY
  void local_crypto_handle(DDS::Security::ParticipantCryptoHandle pch);
#endif

  //protects access to link_ for duration of make_datalink
  typedef ACE_Thread_Mutex          ThreadLockType;
  typedef ACE_Guard<ThreadLockType> GuardThreadType;
  ThreadLockType links_lock_;

  RcHandle<BitSubscriber> bit_sub_;
  GuidPrefix_t local_prefix_;

  /// RTPS uses only one link per transport.
  /// This link can be safely reused by any clients that belong to the same
  /// domain participant (same GUID prefix).  Use by a second participant
  /// is not possible because the network location returned by
  /// connection_info_i() can't be shared among participants.
  RtpsUdpDataLink_rch link_;

  ACE_SOCK_Dgram unicast_socket_;
#ifdef ACE_HAS_IPV6
  ACE_SOCK_Dgram ipv6_unicast_socket_;
#endif
  TransportClient_wrch default_listener_;

  JobQueue_rch job_queue_;

#if OPENDDS_CONFIG_SECURITY

  DDS::Security::ParticipantCryptoHandle local_crypto_handle_;

#ifndef DDS_HAS_MINIMUM_BIT
  ConnectionRecords deferred_connection_records_;
#endif

  struct IceEndpoint : public virtual ACE_Event_Handler, public virtual ICE::Endpoint {
    RtpsUdpTransport& transport;

    IceEndpoint(RtpsUdpTransport& a_transport)
      : transport(a_transport)
      , network_is_unreachable_(false)
    {}

    const ACE_SOCK_Dgram& choose_recv_socket(ACE_HANDLE fd) const;
    virtual int handle_input(ACE_HANDLE fd);
    virtual ICE::AddressListType host_addresses() const;
    ACE_SOCK_Dgram& choose_send_socket(const ACE_INET_Addr& address) const;
    virtual void send(const ACE_INET_Addr& address, const STUN::Message& message);
    virtual ACE_INET_Addr stun_server_address() const;

    bool network_is_unreachable_;
  };
  RcHandle<IceEndpoint> ice_endpoint_;

  typedef PmfSporadicTask<RtpsUdpTransport> Sporadic;
  void relay_stun_task(const MonotonicTimePoint& now);
  RcHandle<Sporadic> relay_stun_task_;
  ICE::ServerReflexiveStateMachine relay_srsm_;

  void start_ice();
  void stop_ice();

  RcHandle<ICE::Agent> ice_agent_;
#endif

  ConfigReader_rch config_reader_;
  void on_data_available(ConfigReader_rch reader);

  friend class RtpsUdpSendStrategy;
  friend class RtpsUdpReceiveStrategy;

  RtpsUdpCore core_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPTRANSPORT_H */
