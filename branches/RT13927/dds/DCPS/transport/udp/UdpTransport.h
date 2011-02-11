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

#include "UdpConfiguration.h"
#include "UdpDataLink.h"
#include "UdpDataLink_rch.h"

#include "dds/DCPS/transport/framework/PriorityKey.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"

#include <map>

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Udp_Export UdpTransport
  : public TransportImpl {
protected:
  virtual DataLink* find_or_create_datalink(
    RepoId local_id,
    const AssociationData* remote_association,
    CORBA::Long priority,
    bool active);

  virtual int configure_i(TransportConfiguration* config);

  virtual void shutdown_i();

  virtual int connection_info_i(TransportInterfaceInfo& info) const;
  ACE_INET_Addr connection_info_i(const TransportInterfaceInfo& info) const;

  virtual bool acked(RepoId local_id, RepoId remote_id);
  virtual void remove_ack(RepoId local_id, RepoId remote_id);

  virtual void release_datalink_i(DataLink* link, bool release_pending);

private:
  UdpConfiguration* config_i_;

  UdpDataLink_rch server_link_;

  typedef std::map<PriorityKey, UdpDataLink_rch> UdpDataLinkMap;
  UdpDataLinkMap client_links_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_UDPTRANSPORT_H */
