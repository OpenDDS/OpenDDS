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

ACE_HANDLE
MulticastReceiveStrategy::get_handle() const
{
  ACE_SOCK_Dgram_Mcast& socket = this->link_->socket();
  return socket.get_handle();
}

int
MulticastReceiveStrategy::handle_input(ACE_HANDLE /*fd*/)
{
  return TransportReceiveStrategy::handle_input();
}

} // namespace DCPS
} // namespace OpenDDS
