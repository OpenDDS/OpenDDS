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

#include "dds/DCPS/RTPS/RtpsBaseMessageTypesC.h"

#include <map>

namespace OpenDDS {
namespace DCPS {

class RtpsUdpInst;

class OpenDDS_Rtps_Udp_Export RtpsUdpTransport : public TransportImpl {
public:
  explicit RtpsUdpTransport(const TransportInst_rch& inst);

private:
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
  ACE_INET_Addr get_connection_addr(const TransportBLOB& data,
                                    bool& requires_inline_qos) const;

  virtual void release_datalink(DataLink* link);

  virtual std::string transport_type() const { return "rtps_udp"; }

  RtpsUdpDataLink* make_datalink(const GuidPrefix_t& local_prefix);

  void use_datalink(const RepoId& local_id,
                    const RepoId& remote_id,
                    const TransportBLOB& remote_data,
                    bool local_reliable,
                    bool remote_reliable);

  RcHandle<RtpsUdpInst> config_i_;

  /// RTPS uses only one link per transport.
  /// This link can be safely reused by any clients that belong to the same
  /// domain participant (same GUID prefix).  Use by a second participant
  /// is not possible because the network location returned by
  /// connection_info_i() can't be shared among participants.
  RtpsUdpDataLink_rch link_;

  ACE_SOCK_Dgram unicast_socket_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RTPSUDPTRANSPORT_H */
