/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/TransportReceiveStrategy.h"

#include "Multicast_Export.h"

#ifndef DCPS_MULTICASTRECEIVESTRATEGY_H
#define DCPS_MULTICASTRECEIVESTRATEGY_H

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastReceiveStrategy
  : public TransportReceiveStrategy {
protected:
  virtual int start_i();

  virtual void stop_i();

  virtual ssize_t receive_bytes(iovec iov[],
                                int n,
                                ACE_INET_Addr& remote_address);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTRECEIVESTRATEGY_H */
