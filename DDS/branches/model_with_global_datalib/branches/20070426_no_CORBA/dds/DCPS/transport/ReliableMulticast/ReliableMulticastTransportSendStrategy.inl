// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"
#include "ReliableMulticastTransportConfiguration.h"
#include "ReliableMulticastThreadSynchResource.h"
#include "detail/ReactivePacketSender.h"

ACE_INLINE
TAO::DCPS::ReliableMulticastTransportSendStrategy::ReliableMulticastTransportSendStrategy(
  TAO::DCPS::ReliableMulticastTransportConfiguration& configuration,
  TAO::DCPS::ReliableMulticastThreadSynchResource* synch_resource
  )
  : TAO::DCPS::TransportSendStrategy(&configuration, synch_resource)
{
}

ACE_INLINE
TAO::DCPS::ReliableMulticastTransportSendStrategy::~ReliableMulticastTransportSendStrategy()
{
}
