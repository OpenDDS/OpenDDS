/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTSENDSTRATEGY_H
#define DCPS_MULTICASTSENDSTRATEGY_H

#include "Multicast_Export.h"

#include "MulticastDataLink_rch.h"

#include "dds/DCPS/transport/framework/TransportSendStrategy.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastSendStrategy
  : public TransportSendStrategy {
public:
  explicit MulticastSendStrategy(MulticastDataLink* link);

  virtual void stop_i();

protected:
  virtual void prepare_header_i();
  
  virtual ssize_t send_bytes_i(const iovec iov[], int n);

private:
  MulticastDataLink_rch link_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* DCPS_MULTICASTSENDSTRATEGY_H */
