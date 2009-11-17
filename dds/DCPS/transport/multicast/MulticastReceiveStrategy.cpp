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
}

MulticastReceiveStrategy::~MulticastReceiveStrategy()
{
}

} // namespace DCPS
} // namespace OpenDDS
