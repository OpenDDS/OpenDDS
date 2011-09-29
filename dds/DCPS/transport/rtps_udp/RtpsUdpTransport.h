/*
 * $Id$
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

#include "dds/DCPS/transport/framework/PriorityKey.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"

#include <map>

namespace OpenDDS {
namespace DCPS {

class RtpsUdpInst;

class OpenDDS_Rtps_Udp_Export RtpsUdpTransport : public TransportImpl {
public:
  explicit RtpsUdpTransport(const TransportInst_rch& inst);

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

  virtual void release_datalink_i(DataLink* link, bool release_pending);

  virtual std::string transport_type() const { return "rtps_udp"; }

private:
  RtpsUdpDataLink* make_datalink(const RepoId& local_id);

  RcHandle<RtpsUdpInst> config_i_;

  /// In the initial implementation there will only be one link per transport.
  /// This link can be safely reused by any clients that belong to the same
  /// domain participant (same GUID prefix).  This implementation could be
  /// extended to automatically create new links when the 'local_id' passed to
  /// find/create/accept differs from the initial local_id.
  RtpsUdpDataLink_rch link_;

  virtual PriorityKey blob_to_key(const TransportBLOB& remote,
                                  CORBA::Long priority,
                                  bool active);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RTPSUDPTRANSPORT_H */
