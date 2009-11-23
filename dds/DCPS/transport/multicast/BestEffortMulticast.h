/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_BESTEFFORTMULTICAST_H
#define DCPS_BESTEFFORTMULTICAST_H

#include "Multicast_Export.h"

#include "MulticastDataLink.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export BestEffortMulticast
  : public MulticastDataLink {
public:
  BestEffortMulticast(MulticastTransport* transport,
                      ACE_INT32 local_peer,
                      ACE_INT32 remote_peer);

  virtual bool header_received(const TransportHeader& header);
  virtual void sample_received(ReceivedDataSample& sample);

  virtual bool acked();

protected:
  virtual bool join_i(const ACE_INET_Addr& group_address, bool active);
  virtual void leave_i();
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_BESTEFFORTMULTICAST_H */
