// -*- C++ -*-
//
// $Id$
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleMcastConfiguration::SimpleMcastConfiguration(const TransportIdType& id)
  : multicast_group_address_(ACE_DEFAULT_MULTICAST_PORT, ACE_DEFAULT_MULTICAST_ADDR),
    receiver_(false)
{
  DBG_ENTRY_LVL("SimpleMcastConfiguration","SimpleMcastConfiguration",5);
  this->transport_type_ = "SimpleMcast";

  if (id == TAO::DCPS::DEFAULT_SIMPLE_MCAST_SUB_ID)
  {
    this->receiver_ = true;
  }

  this->transport_id_ = id;
}

