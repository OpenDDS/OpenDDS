/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTTRANSPORT_H
#define DCPS_MULTICASTTRANSPORT_H

#include "Multicast_Export.h"

#include "MulticastConfiguration.h"
#include "MulticastDataLink.h"
#include "MulticastDataLink_rch.h"

#include "dds/DCPS/transport/framework/TransportImpl.h"

#include <map>

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastTransport
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

  virtual void release_datalink_i(DataLink* link,
                                  bool release_pending);

  virtual void reliability_lost_i(DataLink* link,
                                  const InterfaceListType& interfaces);
private:
  MulticastConfiguration* config_i_;

  typedef std::map<MulticastPeer, MulticastDataLink_rch> MulticastDataLinkMap;
  MulticastDataLinkMap links_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTTRANSPORT_H */
