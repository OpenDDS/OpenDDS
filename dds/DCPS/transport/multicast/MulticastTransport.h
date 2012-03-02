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

  void passive_connection(MulticastPeer peer);

protected:
  virtual DataLink* find_datalink_i(const RepoId& local_id,
                                    const RepoId& remote_id,
                                    const TransportBLOB& remote_data,
                                    bool remote_reliable,
                                    const ConnectionAttribs& attribs,
                                    bool active);

  virtual DataLink* connect_datalink_i(const RepoId& local_id,
                                       const RepoId& remote_id,
                                       const TransportBLOB& remote_data,
                                       bool remote_reliable,
                                       const ConnectionAttribs& attribs);

  virtual DataLink* accept_datalink(ConnectionEvent& ce);
  virtual void stop_accepting(ConnectionEvent& ce);

  virtual bool configure_i(TransportInst* config);

  virtual void shutdown_i();

  virtual bool connection_info_i(TransportLocator& info) const;

  virtual void release_datalink(DataLink* link);

  virtual std::string transport_type() const { return "multicast"; }

private:
  MulticastDataLink* make_datalink(const RepoId& local_id,
                                   const RepoId& remote_id,
                                   CORBA::Long priority,
                                   bool active);

  MulticastSession* start_session(const MulticastDataLink_rch& link,
                                  MulticastPeer remote_peer, bool active);

  RcHandle<MulticastInst> config_i_;

  /// link for pubs.
  MulticastDataLink_rch client_link_;
  /// link for subs.
  MulticastDataLink_rch server_link_;

  // Used by the passive side to track the virtual "connections" to remote
  // peers: the pending_connections_ are potentential peers that the framework
  // has already informed us about (in accept_datalink) but have not yet sent
  // a SYN TRANSPORT_CONTROL message; the connections_ are remote peers that
  // have already sent the SYN message -- we can consider these "complete"
  // from the framework's point of view.
  ACE_SYNCH_MUTEX connections_lock_;
  std::multimap<ConnectionEvent*, MulticastPeer> pending_connections_;
  std::set<MulticastPeer> connections_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTTRANSPORT_H */
