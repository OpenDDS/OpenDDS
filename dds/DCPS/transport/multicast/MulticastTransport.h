/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTTRANSPORT_H
#define DCPS_MULTICASTTRANSPORT_H

#include "Multicast_Export.h"

#include "MulticastDataLink_rch.h"
#include "MulticastTypes.h"

#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/PoolAllocator.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MulticastInst;

class MulticastSession;
typedef RcHandle<MulticastSession> MulticastSession_rch;

class OpenDDS_Multicast_Export MulticastTransport : public TransportImpl {
public:
  explicit MulticastTransport(MulticastInst& inst);
  ~MulticastTransport();

  void passive_connection(MulticastPeer local_peer, MulticastPeer remote_peer);

  MulticastInst& config() const;

protected:
  virtual AcceptConnectResult connect_datalink(const RemoteTransport& remote,
                                               const ConnectionAttribs& attribs,
                                               const TransportClient_rch& client);

  virtual AcceptConnectResult accept_datalink(const RemoteTransport& remote,
                                              const ConnectionAttribs& attribs,
                                              const TransportClient_rch& client);

  virtual void stop_accepting_or_connecting(const TransportClient_wrch& client,
                                            const RepoId& remote_id);

  bool configure_i(MulticastInst& config);

  virtual void shutdown_i();

  virtual bool connection_info_i(TransportLocator& info, ConnectionInfoFlags flags) const;

  virtual void release_datalink(DataLink* link);

  virtual std::string transport_type() const { return "multicast"; }

private:

  typedef ACE_SYNCH_MUTEX         LockType;
  typedef ACE_Guard<LockType>     GuardType;

  typedef ACE_Thread_Mutex         ThreadLockType;
  typedef ACE_Guard<ThreadLockType>     GuardThreadType;

  MulticastDataLink_rch make_datalink(const RepoId& local_id,
                                      Priority priority,
                                      bool active);

  MulticastSession_rch start_session(const MulticastDataLink_rch& link,
                                     MulticastPeer remote_peer, bool active);

  //RcHandle<MulticastInst> config_i_;

  ThreadLockType links_lock_;
  /// link for pubs.
  typedef OPENDDS_MAP(MulticastPeer, MulticastDataLink_rch) Links;
  Links client_links_;
  /// link for subs.
  Links server_links_;

  // Used by the passive side to track the virtual "connections" to remote
  // peers: the pending_connections_ are potential peers that the framework
  // has already informed us about (in accept_datalink) but have not yet sent
  // a SYN TRANSPORT_CONTROL message; the connections_ are remote peers that
  // have already sent the SYN message -- we can consider these "complete"
  // from the framework's point of view.
  ThreadLockType connections_lock_;
  typedef std::vector<DataLink::OnStartCallback> Callbacks;
  typedef std::pair<MulticastPeer, MulticastPeer> Peers;
  typedef OPENDDS_MAP(Peers, Callbacks) PendConnMap;
  PendConnMap pending_connections_;
  // remote peer to local peer
  OPENDDS_SET(Peers) connections_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_MULTICASTTRANSPORT_H */
