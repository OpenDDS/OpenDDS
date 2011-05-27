// -*- C++ -*-
//
// $Id$

#include "ReliableMulticastTransportConfiguration.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::ReliableMulticastTransportConfiguration::ReliableMulticastTransportConfiguration(
  const TransportIdType& id)
  : receiver_(false)
  , sender_history_size_(1024)
  , receiver_buffer_size_(256)
{
  transport_type_ = "ReliableMulticast";
  if (id == DEFAULT_RELIABLE_MULTICAST_SUB_ID)
    this->receiver_ = true;
  transport_id_ = id;
}

ACE_INLINE
TAO::DCPS::ReliableMulticastTransportConfiguration::~ReliableMulticastTransportConfiguration()
{
}
