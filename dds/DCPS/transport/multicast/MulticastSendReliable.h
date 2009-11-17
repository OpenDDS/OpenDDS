/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTSENDRELIABLE_H
#define DCPS_MULTICASTSENDRELIABLE_H

#include "Multicast_Export.h"

#include "MulticastSendStrategy.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastSendReliable
  : public MulticastSendStrategy {
public:
  explicit MulticastSendReliable(MulticastDataLink* link);

  virtual void stop_i();

protected:
  virtual ACE_HANDLE get_handle();

  virtual ssize_t send_bytes_i(const iovec iov[], int n);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTSENDRELIABLE_H */
