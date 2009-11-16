/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastDataLink.h"

#include "dds/DCPS/transport/framework/TransportSendStrategy.h"

#include "Multicast_Export.h"

#ifndef DCPS_MULTICASTSENDSTRATEGY_H
#define DCPS_MULTICASTSENDSTRATEGY_H

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastSendStrategy
  : public TransportSendStrategy {
public:
  explicit MulticastSendStrategy(MulticastDataLink* link);
  virtual ~MulticastSendStrategy();

protected:
  MulticastDataLink* link_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* DCPS_MULTICASTSENDSTRATEGY_H */
