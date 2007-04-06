// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::ReliableMulticastDataLink::~ReliableMulticastDataLink()
{
  stop_i();
}

ACE_INLINE
TAO::DCPS::ReliableMulticastTransportImpl_rch&
TAO::DCPS::ReliableMulticastDataLink::get_transport_impl()
{
  return transport_impl_;
}
