/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPSUDPTRANSPORT_H
#define DCPS_RTPSUDPTRANSPORT_H

#include "Rtps_Udp_Export.h"

#include "RtpsUdpDataLink.h"
#include "RtpsUdpDataLink_rch.h"

#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/framework/TransportClient.h"

#include "dds/DCPS/PoolAllocator.h"

#include "dds/DCPS/RTPS/RtpsCoreC.h"

#include "dds/DCPS/RTPS/ICE/Ice.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class RtpsUdpInst;

class OpenDDS_Rtps_Udp_Export RtpsUdpTransport : public TransportImpl {
public:
  RtpsUdpTransport(RtpsUdpInst& inst);
  RtpsUdpInst& config() const;
  virtual ICE::Endpoint* get_ice_endpoint();

  virtual void update_locators(const RepoId& /*remote*/,
                               const TransportLocatorSeq& /*locators*/);

private:
  virtual AcceptConnectResult connect_datalink(const RemoteTransport& remote,
                                               const ConnectionAttribs& attribs,
                                               const TransportClient_rch& client);

  virtual AcceptConnectResult accept_datalink(const RemoteTransport& remote,
                                              const ConnectionAttribs& attribs,
                                              const TransportClient_rch& client);

  virtual void stop_accepting_or_connecting(const TransportClient_wrch& client,
                                            const RepoId& remote_id);

  bool configure_i(RtpsUdpInst& config);

  virtual void shutdown_i();

  virtual void register_for_reader(const RepoId& participant,
                                   const RepoId& writerid,
                                   const RepoId& readerid,
                                   const TransportLocatorSeq& locators,
                                   OpenDDS::DCPS::DiscoveryListener* listener);

  virtual void unregister_for_reader(const RepoId& participant,
                                     const RepoId& writerid,
                                     const RepoId& readerid);

  virtual void register_for_writer(const RepoId& /*participant*/,
                                   const RepoId& /*readerid*/,
                                   const RepoId& /*writerid*/,
                                   const TransportLocatorSeq& /*locators*/,
                                   DiscoveryListener* /*listener*/);

  virtual void unregister_for_writer(const RepoId& /*participant*/,
                                     const RepoId& /*readerid*/,
                                     const RepoId& /*writerid*/);

  virtual bool connection_info_i(TransportLocator& info, ConnectionInfoFlags flags) const;
  ACE_INET_Addr get_connection_addr(const TransportBLOB& data,
                                    bool* requires_inline_qos = 0,
                                    unsigned int* blob_bytes_read = 0) const;

  virtual void release_datalink(DataLink* link);

  virtual OPENDDS_STRING transport_type() const { return "rtps_udp"; }

  RtpsUdpDataLink_rch make_datalink(const GuidPrefix_t& local_prefix);

  void use_datalink(const RepoId& local_id,
                    const RepoId& remote_id,
                    const TransportBLOB& remote_data,
                    bool local_reliable, bool remote_reliable,
                    bool local_durable, bool remote_durable);

#if defined(OPENDDS_SECURITY)
  void local_crypto_handle(DDS::Security::ParticipantCryptoHandle pch)
  {
    local_crypto_handle_ = pch;
    if (link_) {
      link_->local_crypto_handle(pch);
    }
  }
#endif

  //protects access to link_ for duration of make_datalink
  typedef ACE_Thread_Mutex          ThreadLockType;
  typedef ACE_Guard<ThreadLockType> GuardThreadType;
  ThreadLockType links_lock_;

  /// This protects the connections_ data member
  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;
  LockType connections_lock_;

  /// RTPS uses only one link per transport.
  /// This link can be safely reused by any clients that belong to the same
  /// domain participant (same GUID prefix).  Use by a second participant
  /// is not possible because the network location returned by
  /// connection_info_i() can't be shared among participants.
  RtpsUdpDataLink_rch link_;
  bool map_ipv4_to_ipv6() const;

  ACE_SOCK_Dgram unicast_socket_;

  TransportClient_wrch default_listener_;

#if defined(OPENDDS_SECURITY)
  DDS::Security::ParticipantCryptoHandle local_crypto_handle_;
#endif

#ifdef OPENDDS_SECURITY
  struct IceEndpoint : public ACE_Event_Handler, public ICE::Endpoint {
    RtpsUdpTransport& transport;

    IceEndpoint(RtpsUdpTransport& a_transport)
      : transport(a_transport) {}

    virtual int handle_input(ACE_HANDLE fd);
    virtual ICE::AddressListType host_addresses() const;
    virtual void send(const ACE_INET_Addr& address, const STUN::Message& message);
    virtual ACE_INET_Addr stun_server_address() const;
  };
  IceEndpoint ice_endpoint_;
#endif
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_RTPSUDPTRANSPORT_H */
