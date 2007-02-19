// -*- C++ -*-
//
// $Id$

#include "ReliableMulticastDataLink.h"
#include "detail/ReactivePacketReceiver.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::ReliableMulticastTransportReceiveStrategy::ReliableMulticastTransportReceiveStrategy(
  ReliableMulticastDataLink& data_link
  )
  : data_link_(data_link)
{
}

ACE_INLINE
TAO::DCPS::ReliableMulticastTransportReceiveStrategy::~ReliableMulticastTransportReceiveStrategy()
{
}
