/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastReceiveStrategy.h"
#include "Multicast_Export.h"

#ifndef DCPS_MULTICASTRECEIVERELIABLE_H
#define DCPS_MULTICASTRECEIVERELIABLE_H

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastReceiveReliable
  : public MulticastReceiveStrategy {
public:
  explicit MulticastReceiveReliable(MulticastDataLink* link);

protected:
  virtual ssize_t receive_bytes(iovec iov[],
                                int n,
                                ACE_INET_Addr& remote_address);

  virtual TransportHeaderStatus check_transport_header(const TransportHeader& header);

  virtual void deliver_sample(ReceivedDataSample& sample,
                              const ACE_INET_Addr& remote_address);

  virtual int start_i();

  virtual void stop_i();
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTRECEIVERELIABLE_H */
