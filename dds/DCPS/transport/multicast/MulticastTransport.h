/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/TransportImpl.h"

#include <map>

#include "Multicast_Export.h"

#ifndef DCPS_MULTICASTTRANSPORT_H
#define DCPS_MULTICASTTRANSPORT_H

namespace OpenDDS {
namespace DCPS {

class MulticastConfiguration;
class MulticastDataLink;

class OpenDDS_Multicast_Export MulticastTransport
  : public TransportImpl {
public:
  MulticastConfiguration* get_configuration();

protected:
  virtual DataLink* find_or_create_datalink(
    RepoId local_id,
    const AssociationData* remote_association,
    CORBA::Long priority,
    bool active);

  virtual int configure_i(TransportConfiguration* config);

  virtual void shutdown_i();

  virtual int connection_info_i(TransportInterfaceInfo& info) const;

  virtual void release_datalink_i(DataLink* link, bool release_pending);

private:
  MulticastConfiguration* config_i_;

  typedef std::map<long, MulticastDataLink*> MulticastDataLinkMap;
  MulticastDataLinkMap links_;

  ACE_INET_Addr get_connection_info(const TransportInterfaceInfo& info) const;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "MulticastTransport.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_MULTICASTTRANSPORT_H */
