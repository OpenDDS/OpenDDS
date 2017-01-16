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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class RtpsUdpInst;
typedef RcHandle<RtpsUdpInst> RtpsUdpInst_rch;

class OpenDDS_Rtps_Udp_Export RtpsUdpTransport : public TransportImpl {
public:
  RtpsUdpTransport(const TransportInst_rch& inst);
  RtpsUdpInst_rch config() const;
private:
  virtual AcceptConnectResult connect_datalink(const RemoteTransport& remote,
                                               const ConnectionAttribs& attribs,
                                               const TransportClient_rch& client);

  virtual AcceptConnectResult accept_datalink(const RemoteTransport& remote,
                                              const ConnectionAttribs& attribs,
                                              const TransportClient_rch& client);

  virtual void stop_accepting_or_connecting(const TransportClient_rch& client,
                                            const RepoId& remote_id);

  virtual bool configure_i(TransportInst* config);

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

  virtual bool connection_info_i(TransportLocator& info) const;
  ACE_INET_Addr get_connection_addr(const TransportBLOB& data,
                                    bool& requires_inline_qos) const;

  virtual void release_datalink(DataLink* link);
  void pre_detach(const TransportClient_rch& client);

  virtual OPENDDS_STRING transport_type() const { return "rtps_udp"; }

  RtpsUdpDataLink_rch make_datalink(const GuidPrefix_t& local_prefix);

  void use_datalink(const RepoId& local_id,
                    const RepoId& remote_id,
                    const TransportBLOB& remote_data,
                    bool local_reliable, bool remote_reliable,
                    bool local_durable, bool remote_durable);

  //protects access to link_ for duration of make_datalink
  typedef ACE_Thread_Mutex         ThreadLockType;
  typedef ACE_Guard<ThreadLockType>     GuardThreadType;
  ThreadLockType links_lock_;

  /// This protects the connections_ and the pending_connections_
  /// data members.
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

  TransportClient_rch default_listener_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_RTPSUDPTRANSPORT_H */
