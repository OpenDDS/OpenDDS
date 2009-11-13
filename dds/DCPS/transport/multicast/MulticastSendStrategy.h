/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/TransportSendStrategy.h"

#include "Multicast_Export.h"

#ifndef DCPS_MULTICASTSENDSTRATEGY_H
#define DCPS_MULTICASTSENDSTRATEGY_H

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastSendStrategy
  : public TransportSendStrategy {
public:
  MulticastSendStrategy(TransportConfiguration* config,
                        CORBA::Long priority);

  virtual void stop_i();

protected:
  virtual ACE_HANDLE get_handle();

  virtual ssize_t send_bytes_i(const iovec iov[], int n);
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* DCPS_MULTICASTSENDSTRATEGY_H */
