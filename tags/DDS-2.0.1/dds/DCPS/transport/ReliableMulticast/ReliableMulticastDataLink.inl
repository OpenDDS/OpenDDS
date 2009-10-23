/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::ReliableMulticastDataLink::~ReliableMulticastDataLink()
{
  stop_i();
}

ACE_INLINE
OpenDDS::DCPS::ReliableMulticastTransportImpl_rch&
OpenDDS::DCPS::ReliableMulticastDataLink::get_transport_impl()
{
  return transport_impl_;
}

ACE_INLINE
ACE_SOCK&
OpenDDS::DCPS::ReliableMulticastDataLink::socket()
{
  return this->send_strategy_.socket();
}
