// -*- C++ -*-
//
// $Id$
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleUdpConfiguration::SimpleUdpConfiguration()
{
  DBG_ENTRY("SimpleUdpConfiguration","SimpleUdpConfiguration");
  this->max_packet_size_ = 62501;
  this->transport_type_ = "SimpleUdp";
}

