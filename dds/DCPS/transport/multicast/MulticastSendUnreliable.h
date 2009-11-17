/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTSENDUNRELIABLE_H
#define DCPS_MULTICASTSENDUNRELIABLE_H

#include "Multicast_Export.h"

#include "MulticastSendStrategy.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastSendUnreliable
  : public MulticastSendStrategy {
public:
  explicit MulticastSendUnreliable(MulticastDataLink* link);

  virtual void stop_i();

protected:
  virtual ssize_t send_bytes_i(const iovec iov[], int n);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTSENDUNRELIABLE_H */
