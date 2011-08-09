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

class OpenDDS_Multicast_Export MulticastTransport : public TransportImpl {
public:
  explicit MulticastTransport(const TransportInst_rch& inst);
  ~MulticastTransport();

protected:
  virtual DataLink* find_datalink_i(const RepoId& local_id,
                                    const RepoId& remote_id,
                                    const TransportBLOB& remote_data,
                                    CORBA::Long priority,
                                    bool active);

  virtual DataLink* connect_datalink_i(const RepoId& local_id,
                                       const RepoId& remote_id,
                                       const TransportBLOB& remote_data,
                                       CORBA::Long priority);

  virtual DataLink* accept_datalink(ConnectionEvent& ce);
  virtual void stop_accepting(ConnectionEvent& ce);

  virtual bool configure_i(TransportInst* config);

  virtual void shutdown_i();

  virtual bool connection_info_i(TransportLocator& info) const;

  virtual void release_datalink_i(DataLink* link,
                                  bool release_pending);

  virtual std::string transport_type() const { return "multicast"; }

private:
  MulticastDataLink* make_datalink(const RepoId& local_id,
                                   const RepoId& remote_id,
                                   CORBA::Long priority,
                                   bool active);

  bool start_session(const MulticastDataLink_rch& link,
                     MulticastPeer remote_peer, bool active);

  RcHandle<MulticastInst> config_i_;

  /// link for pubs.
  MulticastDataLink_rch client_link_;
  /// link for subs.
  MulticastDataLink_rch server_link_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTTRANSPORT_H */
