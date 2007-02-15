// -*- C++ -*-
//
// $Id$

#include "detail/ReactivePacketReceiver.h"
#include "detail/ReactivePacketSender.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::ReliableMulticastDataLink::~ReliableMulticastDataLink()
{
  stop_i();
}
