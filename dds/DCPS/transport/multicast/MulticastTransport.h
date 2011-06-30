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
  MulticastTransport();
  ~MulticastTransport();

protected:
  virtual DataLink* find_or_create_datalink(
    RepoId local_id,
    const AssociationData* remote_association,
    CORBA::Long priority,
    bool active);

  virtual int configure_i(TransportInst* config);

  virtual void shutdown_i();

  virtual int connection_info_i(TransportInterfaceInfo& info) const;

  virtual bool acked(RepoId local_id, RepoId remote_id);
  virtual void remove_ack(RepoId local_id, RepoId remote_id);

  virtual void release_datalink_i(DataLink* link,
                                  bool release_pending);
private:
  MulticastInst* config_i_;

  /// link for pubs.
  MulticastDataLink_rch client_link_;
  /// link for subs.
  MulticastDataLink_rch server_link_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTTRANSPORT_H */
