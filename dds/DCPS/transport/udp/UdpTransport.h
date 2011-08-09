/*
 * $Id$
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

#include <map>

namespace OpenDDS {
namespace DCPS {

class UdpInst;

class OpenDDS_Udp_Export UdpTransport : public TransportImpl {
public:
  explicit UdpTransport(const TransportInst_rch& inst);

  void passive_connection(const ACE_INET_Addr& remote_address,
                          ACE_Message_Block* data);

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
  ACE_INET_Addr get_connection_addr(const TransportBLOB& data) const;

  virtual bool acked(RepoId local_id, RepoId remote_id);
  virtual void remove_ack(RepoId local_id, RepoId remote_id);

  virtual void release_datalink_i(DataLink* link, bool release_pending);

  virtual std::string transport_type() const { return "udp"; }

private:
  UdpDataLink* make_datalink(const ACE_INET_Addr& remote_address, bool active);

  RcHandle<UdpInst> config_i_;

  UdpDataLink_rch server_link_;

  typedef std::map<PriorityKey, UdpDataLink_rch> UdpDataLinkMap;
  UdpDataLinkMap client_links_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_UDPTRANSPORT_H */
