/*
 * $Id$
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

#include <map>

namespace OpenDDS {
namespace DCPS {

class MulticastInst;
class MulticastSession;

class OpenDDS_Multicast_Export MulticastTransport : public TransportImpl {
public:
  explicit MulticastTransport(const TransportInst_rch& inst);
  ~MulticastTransport();

  void passive_connection(MulticastPeer local_peer, MulticastPeer remote_peer);

protected:
  virtual DataLink* find_datalink_i(const RepoId& local_id,
                                    const RepoId& remote_id,
                                    const TransportBLOB& remote_data,
                                    bool remote_reliable,
                                    bool remote_durable,
                                    const ConnectionAttribs& attribs,
                                    bool active);

  virtual DataLink* connect_datalink_i(const RepoId& local_id,
                                       const RepoId& remote_id,
                                       const TransportBLOB& remote_data,
                                       bool remote_reliable,
                                       bool remote_durable,
                                       const ConnectionAttribs& attribs);

  virtual DataLink* accept_datalink(ConnectionEvent& ce);
  virtual void stop_accepting(ConnectionEvent& ce);

  virtual bool configure_i(TransportInst* config);

  virtual void shutdown_i();

  virtual bool connection_info_i(TransportLocator& info) const;

  virtual void release_datalink(DataLink* link);

  virtual std::string transport_type() const { return "multicast"; }

private:
  typedef ACE_Thread_Mutex         ThreadLockType;
  typedef ACE_Guard<ThreadLockType>     GuardThreadType;

  MulticastDataLink* make_datalink(const RepoId& local_id,
                                   CORBA::Long priority,
                                   bool active);

  MulticastSession* start_session(const MulticastDataLink_rch& link,
                                  MulticastPeer remote_peer, bool active);

  RcHandle<MulticastInst> config_i_;

  ThreadLockType links_lock_;
  /// link for pubs.
  typedef std::map<MulticastPeer, MulticastDataLink_rch> Links;
  Links client_links_;
  /// link for subs.
  Links server_links_;

  // Used by the passive side to track the virtual "connections" to remote
  // peers: the pending_connections_ are potentential peers that the framework
  // has already informed us about (in accept_datalink) but have not yet sent
  // a SYN TRANSPORT_CONTROL message; the connections_ are remote peers that
  // have already sent the SYN message -- we can consider these "complete"
  // from the framework's point of view.
  ThreadLockType connections_lock_;
  std::multimap<ConnectionEvent*, MulticastPeer> pending_connections_;
  // remote peer to local peer
  typedef std::pair<MulticastPeer, MulticastPeer> Peers;
  std::set<Peers> connections_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTTRANSPORT_H */
