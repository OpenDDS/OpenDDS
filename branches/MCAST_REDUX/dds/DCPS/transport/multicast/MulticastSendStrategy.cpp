/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSendStrategy.h"
#include "MulticastDataLink.h"
#include "MulticastConfiguration.h"

namespace OpenDDS {
namespace DCPS {

MulticastSendStrategy::MulticastSendStrategy(MulticastDataLink* link)
  : TransportSendStrategy(link->get_configuration(),
                          0,  // ThreadSynchResource
                          link->transport_priority()),
    link_(link)
{
}

MulticastSendStrategy::~MulticastSendStrategy()
{
}

} // namespace DCPS
} // namespace OpenDDS
