/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/TransportImpl.h"

#include "Multicast_Export.h"

#include <map>

#ifndef DCPS_MULTICASTTRANSPORT_H
#define DCPS_MULTICASTTRANSPORT_H

namespace OpenDDS {
namespace DCPS {

class MulticastConfiguration;

class OpenDDS_Multicast_Export MulticastTransport
  : public TransportImpl {
public:
  ~MulticastTransport();

protected:
  virtual DataLink* find_or_create_datalink(
    RepoId local_id,
    const AssociationData* remote_association,
    CORBA::Long priority,
    bool active);

  virtual int configure_i(TransportConfiguration* config);

  virtual void shutdown_i();

  virtual int connection_info_i(TransportInterfaceInfo& local_info) const;

  virtual void release_datalink_i(DataLink* link, bool release_pending);

private:
  MulticastConfiguration* config_i_;

  typedef std::map<long, DataLink*> DataLinkMap;
  DataLinkMap links_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTTRANSPORT_H */
