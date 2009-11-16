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

#ifndef DCPS_MULTICASTRECEIVEUNRELIABLE_H
#define DCPS_MULTICASTRECEIVEUNRELIABLE_H

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastReceiveUnreliable
  : public MulticastReceiveStrategy {
public:
  explicit MulticastReceiveUnreliable(MulticastDataLink* link);

protected:
  virtual ssize_t receive_bytes(iovec iov[],
                                int n,
                                ACE_INET_Addr& remote_address);

  virtual void deliver_sample(ReceivedDataSample& sample,
                              const ACE_INET_Addr& remote_address);

  virtual int start_i();

  virtual void stop_i();
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTRECEIVEUNRELIABLE_H */
