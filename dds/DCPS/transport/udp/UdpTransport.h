/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_UDPTRANSPORT_H
#define DCPS_UDPTRANSPORT_H

#include "Udp_Export.h"

#include "UdpDataLink.h"
#include "UdpDataLink_rch.h"

#include "dds/DCPS/transport/framework/PriorityKey.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/PoolAllocator.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class UdpInst;

class OpenDDS_Udp_Export UdpTransport : public TransportImpl {
public:
  explicit UdpTransport(UdpInst& inst);

  void passive_connection(const ACE_INET_Addr& remote_address,
                          const Message_Block_Ptr& data);
  UdpInst& config() const;
protected:
  virtual AcceptConnectResult connect_datalink(const RemoteTransport& remote,
                                               const ConnectionAttribs& attribs,
                                               const TransportClient_rch& client);

  virtual AcceptConnectResult accept_datalink(const RemoteTransport& remote,
                                              const ConnectionAttribs& attribs,
                                              const TransportClient_rch& client);

  virtual void stop_accepting_or_connecting(const TransportClient_wrch& client,
                                            const RepoId& remote_id);

  bool configure_i(UdpInst& config);

  virtual void shutdown_i();

  virtual bool connection_info_i(TransportLocator& info, ConnectionInfoFlags flags) const;
  ACE_INET_Addr get_connection_addr(const TransportBLOB& data) const;

  virtual void release_datalink(DataLink* link);

  virtual std::string transport_type() const { return "udp"; }

private:
  UdpDataLink_rch make_datalink(const ACE_INET_Addr& remote_address,
                             Priority priority, bool active);

  PriorityKey blob_to_key(const TransportBLOB& remote,
                          Priority priority, ACE_INET_Addr local_addr, bool active);

  typedef ACE_SYNCH_MUTEX         LockType;
  typedef ACE_Guard<LockType>     GuardType;
  typedef ACE_Condition<LockType> ConditionType;

  /// This lock is used to protect the client_links_ data member.
  LockType client_links_lock_;

  /// Map of fully associated DataLinks for this transport.  Protected
  // by client_links_lock_.
  typedef OPENDDS_MAP(PriorityKey, UdpDataLink_rch) UdpDataLinkMap;
  UdpDataLinkMap client_links_;

  /// The single datalink for the passive side.  No locking required.
  UdpDataLink_rch server_link_;

  /// This protects the pending_connections_, pending_server_link_keys_,
  /// and server_link_keys_ data members.
  //LockType connections_lock_;
  ACE_Recursive_Thread_Mutex connections_lock_;

  /// Locked by connections_lock_.
  /// These are passive-side PriorityKeys that have been fully associated
  /// (processed by accept_datalink() and finished handshaking).  They are
  /// ready for use and reuse via server_link_.
  std::set<PriorityKey> server_link_keys_;

  typedef std::vector<DataLink::OnStartCallback> Callbacks;
  typedef OPENDDS_MAP(PriorityKey, Callbacks) PendConnMap;
  /// Locked by connections_lock_.  Tracks expected connections
  /// that we have learned about in accept_datalink() but have
  /// not yet performed the handshake.
  PendConnMap pending_connections_;

  /// Locked by connections_lock_.
  /// These are passive-side PriorityKeys that have finished handshaking,
  /// but have not been processed by accept_datalink()
  std::set<PriorityKey> pending_server_link_keys_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_UDPTRANSPORT_H */
