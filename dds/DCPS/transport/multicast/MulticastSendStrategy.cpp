/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSendStrategy.h"

namespace OpenDDS {
namespace DCPS {

MulticastSendStrategy::MulticastSendStrategy(MulticastDataLink* link)
  : TransportSendStrategy(link->get_configuration(),
                          0,  // ThreadSynchResource
                          link->transport_priority()),
    link_(link)
{
  if (this->link_ != 0) {
    this->link_->_add_ref();
  }
}

MulticastSendStrategy::~MulticastSendStrategy()
{
  if (this->link_ != 0) {
    this->link_->_remove_ref();
  }
}

} // namespace DCPS
} // namespace OpenDDS
