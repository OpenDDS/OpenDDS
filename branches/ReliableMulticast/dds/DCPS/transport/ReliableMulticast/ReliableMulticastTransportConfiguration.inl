// -*- C++ -*-
//
// $Id$

#include "ReliableMulticastTransportConfiguration.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::ReliableMulticastTransportConfiguration::ReliableMulticastTransportConfiguration()
  : receiver_(false)
  , sender_history_size_(1024)
  , receiver_buffer_size_(256)
{
  transport_type_ = "ReliableMulticast";
}

ACE_INLINE
TAO::DCPS::ReliableMulticastTransportConfiguration::~ReliableMulticastTransportConfiguration()
{
}
