// -*- C++ -*-
//
// $Id$
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleUdpConfiguration::SimpleUdpConfiguration(const TransportIdType& id)
{
  DBG_ENTRY_LVL("SimpleUdpConfiguration","SimpleUdpConfiguration",5);
  this->transport_type_ = "SimpleUdp";
  this->transport_id_ = id;
}

