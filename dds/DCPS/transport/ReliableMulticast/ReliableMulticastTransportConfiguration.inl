// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"
#include "ReliableMulticastTransportConfiguration.h"

ACE_INLINE
TAO::DCPS::ReliableMulticastTransportConfiguration::ReliableMulticastTransportConfiguration()
  : receiver_(false)
{
}

ACE_INLINE
TAO::DCPS::ReliableMulticastTransportConfiguration::~ReliableMulticastTransportConfiguration()
{
}
