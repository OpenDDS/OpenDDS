// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"
#include "ReliableMulticastTransportConfiguration.h"
#include "ReliableMulticastThreadSynchResource.h"
#include "detail/ReactivePacketSender.h"

ACE_INLINE
OpenDDS::DCPS::ReliableMulticastTransportSendStrategy::ReliableMulticastTransportSendStrategy(
  OpenDDS::DCPS::ReliableMulticastTransportConfiguration& configuration,
  OpenDDS::DCPS::ReliableMulticastThreadSynchResource* synch_resource,
  CORBA::Long priority
  )
  : OpenDDS::DCPS::TransportSendStrategy(&configuration, synch_resource, priority)
{
}

ACE_INLINE
OpenDDS::DCPS::ReliableMulticastTransportSendStrategy::~ReliableMulticastTransportSendStrategy()
{
}
