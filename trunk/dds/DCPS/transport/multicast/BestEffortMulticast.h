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
                      MulticastPeer local_peer,
                      MulticastPeer remote_peer,
                      bool active);

  virtual bool acked();

  virtual bool header_received(const TransportHeader& header);
  virtual void sample_received(ReceivedDataSample& sample);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_BESTEFFORTMULTICAST_H */
