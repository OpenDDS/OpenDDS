/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastReceiveStrategy.h"

namespace OpenDDS {
namespace DCPS {

MulticastReceiveStrategy::MulticastReceiveStrategy(MulticastDataLink* link)
  : link_(link)
{
  if (this->link_ != 0) {
    this->link_->_add_ref();
  }
}

MulticastReceiveStrategy::~MulticastReceiveStrategy()
{
  if (this->link_ != 0) {
    this->link_->_remove_ref();
  }
}

} // namespace DCPS
} // namespace OpenDDS
