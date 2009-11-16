/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastTransport.h"

#include "ace/SOCK_Dgram_Mcast.h"

#include "dds/DCPS/transport/framework/DataLink.h"

#include "Multicast_Export.h"

#ifndef DCPS_MULTICASTDATALINK_H
#define DCPS_MULTICASTDATALINK_H

namespace OpenDDS {
namespace DCPS {

class AssociationData;
class MulticastConfiguration;

class OpenDDS_Multicast_Export MulticastDataLink
  : public DataLink {
public:
  MulticastDataLink(MulticastTransport* impl,
                    CORBA::Long priority,
                    long local_peer,
                    long remote_peer);

  bool open(const ACE_INET_Addr& group_address, bool active);
  void close();

protected:
  virtual void stop_i();

private:
  MulticastTransport* impl_i_;

  long local_peer_;
  long remote_peer_;

  ACE_SOCK_Dgram_Mcast socket_;

  friend class MulticastSendStrategy;
  friend class MulticastReceiveStrategy;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTDATALINK_H */
